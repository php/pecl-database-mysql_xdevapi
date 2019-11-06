/*
  +----------------------------------------------------------------------+
  | PHP Version 7                                                        |
  +----------------------------------------------------------------------+
  | Copyright (c) 2006-2020 The PHP Group                                |
  +----------------------------------------------------------------------+
  | This source file is subject to version 3.01 of the PHP license,      |
  | rhs is bundled with this package in the file LICENSE, and is        |
  | available through the world-wide-web at the following url:           |
  | http://www.php.net/license/3_01.txt                                  |
  | If you did not receive a copy of the PHP license and are unable to   |
  | obtain it through the world-wide-web, please send a note to          |
  | license@php.net so we can mail you a copy immediately.               |
  +----------------------------------------------------------------------+
  | Authors: Darek Slusarczyk <marines@php.net>                          |
  +----------------------------------------------------------------------+
*/
#include "php_api.h"
#include "mysqlnd_api.h"
#include "zend_utils.h"
#include "strings.h"
#include "string_utils.h"
#include <boost/algorithm/string/classification.hpp>
#include <boost/algorithm/string/split.hpp>

namespace mysqlx {

namespace util {

namespace zend {

void ensure_is_array(zval* zv)
{
	if (Z_TYPE_P(zv) == IS_ARRAY) return;

	zval_ptr_dtor(zv);
	array_init_size(zv, 0);
}

// ----------------

namespace {

template<typename T>
void free_error_list(
	T& error_list,
	zend_bool persistent);

template<>
void free_error_list<zend_llist*>(
	zend_llist*& error_list,
	zend_bool persistent)
{
	if (error_list) {
		zend_llist_clean(error_list);
		mnd_pefree(error_list, persistent);
		error_list = nullptr;
	}
}

template<>
void free_error_list<zend_llist>(
	zend_llist& error_list,
	zend_bool /*persistent*/)
{
	zend_llist_clean(&error_list);
}

} // anonymous namespace

void free_error_info_list(
	MYSQLND_ERROR_INFO* error_info,
	zend_bool persistent)
{
	free_error_list(error_info->error_list, persistent);
}

// -----------------------------------------------------------------

namespace {

struct Arg_list
{
	Arg_list(const zend_execute_data* execute_data);

	std::size_t args_count() const;
	std::size_t required_args_count() const;
	bool is_variadic() const;

	const zend_execute_data* const execute_data;
};

// ------------

Arg_list::Arg_list(const zend_execute_data* execute_data)
	: execute_data(execute_data)
{
}

std::size_t Arg_list::args_count() const
{
	return execute_data->func->common.num_args;
}

std::size_t Arg_list::required_args_count() const
{
	return execute_data->func->common.required_num_args;
}

bool Arg_list::is_variadic() const
{
	return (execute_data->func->common.fn_flags & ZEND_ACC_VARIADIC) == ZEND_ACC_VARIADIC;
}

// ------------------------

struct Type_spec
{
	enum class Variadic {
		None,
		Zero_or_more,
		One_or_more
	};

	Type_spec(
		const util::string& args,
		Variadic variadic);

	bool empty() const;
	bool is_variadic() const;

	const util::string args;
	const Variadic variadic;
};

// ------------

Type_spec::Type_spec(
	const util::string& args,
	Variadic variadic)
	: args(args)
	, variadic(variadic)
{
}

bool Type_spec::empty() const
{
	return args.empty() && !is_variadic();
}

bool Type_spec::is_variadic() const
{
	return variadic != Variadic::None;
}

// ------------------------

class Verify_call_parameters
{
public:
	Verify_call_parameters(
		bool is_method,
		zend_execute_data* execute_data,
		const char* type_spec);

	void run();

private:
	void extract_type_spec_fragments(
		util::string* required_args,
		util::string* optional_args);
	Type_spec create_type_spec(const util::string& raw_args);
	Type_spec::Variadic resolve_variadic(const util::string& args);
	void validate_type_spec(const Type_spec& type_spec);

	void verify_variadic(
		const Type_spec& type_spec_required_args,
		const Type_spec& type_spec_optional_args);

	void verify_required_args_count(
		const Type_spec& type_spec_required_args);
	void verify_optional_args_count(
		const Type_spec& type_spec_optional_args);
	std::size_t calc_arglist_optional_args_count();

	std::size_t calc_min_args_count(
		const Type_spec& type_spec,
		bool required);

	std::invalid_argument verify_error(const util::string& reason);

private:
	const bool is_method;
	const Arg_list arg_list;
	const util::string type_spec;
};

// ------------

Verify_call_parameters::Verify_call_parameters(
	bool is_method,
	zend_execute_data* execute_data,
	const char* type_spec)
	: is_method(is_method)
	, arg_list(execute_data)
	, type_spec(type_spec)
{
}

void Verify_call_parameters::run()
{
	util::string required_args;
	util::string optional_args;
	extract_type_spec_fragments(&required_args, &optional_args);

	Type_spec type_spec_required_args{ create_type_spec(required_args) };
	validate_type_spec(type_spec_required_args);

	Type_spec type_spec_optional_args{ create_type_spec(optional_args) };
	validate_type_spec(type_spec_optional_args);

	verify_variadic(type_spec_required_args, type_spec_optional_args);

	verify_required_args_count(type_spec_required_args);
	verify_optional_args_count(type_spec_optional_args);
}

void Verify_call_parameters::extract_type_spec_fragments(
	util::string* required_args,
	util::string* optional_args)
{
	util::strings type_spec_fragments;
	const char* Optional_args_separator{ "|" };
	boost::split(type_spec_fragments, type_spec, boost::is_any_of(Optional_args_separator));
	if (type_spec_fragments.size() < 2) {
		type_spec_fragments.resize(2);
	} else if (2 < type_spec_fragments.size()) {
		throw verify_error("only one optional args block is allowed");
	}

	*required_args = type_spec_fragments.front();
	*optional_args = type_spec_fragments.back();
}

Type_spec Verify_call_parameters::create_type_spec(const util::string& raw_args)
{
	util::string args(raw_args);

	const Type_spec::Variadic variadic{ resolve_variadic(args) };
	if (variadic != Type_spec::Variadic::None) {
		args.pop_back();
	}

	return Type_spec(args, variadic);
}

Type_spec::Variadic Verify_call_parameters::resolve_variadic(const util::string& args)
{
	const util::string variadic_types{ "*+" };

	const std::size_t idx{ args.find_first_of(variadic_types) };
	if (idx == util::string::npos) return Type_spec::Variadic::None;

	if (idx + 1 != args.length()) {
		throw verify_error("variadic specificator is always last char or it is invalid");
	}

	switch (type_spec[idx]) {
		case '*':
			return Type_spec::Variadic::Zero_or_more;

		case '+':
			return Type_spec::Variadic::One_or_more;

		default:
			assert(!"unexpected char!");
			return Type_spec::Variadic::None;
	}
}

void Verify_call_parameters::validate_type_spec(const Type_spec& type_specification)
{
	/*
		type_spec sscanf like typelist (though no %)
		l long -> long *
		d double -> double *
		b boolean -> zend_bool *
		a array -> zval **
		h HastTable -> HastTable*
		o object -> zval **
		O object -> zval **, zend_class_entry *
			Object must be derived from given class
		s string -> char **, int *
			You receive string and length
		r resource -> zval **
		z zval -> zval **
		Z zval-ref -> zval ***
		| right part is optional
		/ next param gets separated if not reference
		! Next param returns NULL if param type IS_NULL
	*/
	const util::string allowed_types{ "ldbahoOsz" };

	const std::size_t disallowed_type_idx{ type_specification.args.find_first_not_of(allowed_types) };
	if (disallowed_type_idx != util::string::npos) {
		throw verify_error("unknown type in type_specification");
	}
}

void Verify_call_parameters::verify_variadic(
	const Type_spec& type_spec_required_args,
	const Type_spec& type_spec_optional_args)
{
	if (type_spec_required_args.is_variadic() && !type_spec_optional_args.empty()) {
		throw verify_error("variadic specificator is always last char in whole type_spec");
	}
}

void Verify_call_parameters::verify_required_args_count(
	const Type_spec& type_spec_required_args)
{
	const std::size_t arglist_required_args_count{
		arg_list.required_args_count()
	};
	const std::size_t type_spec_required_args_count{
		calc_min_args_count(type_spec_required_args, true)
	};
	if (arglist_required_args_count != type_spec_required_args_count) {
		throw verify_error("required number of args in arglist and type_spec are different");
	}
}

void Verify_call_parameters::verify_optional_args_count(
	const Type_spec& type_spec_optional_args)
{
	const std::size_t arglist_optional_args_count{
		calc_arglist_optional_args_count()
	};
	const std::size_t type_spec_optional_args_count{
		calc_min_args_count(type_spec_optional_args, false)
	};
	if (arglist_optional_args_count != type_spec_optional_args_count) {
		throw verify_error("optional number of args in arglist and type_spec are different");
	}
}

std::size_t Verify_call_parameters::calc_arglist_optional_args_count()
{
	const std::size_t arglist_args_count{ arg_list.args_count() };
	const std::size_t arglist_required_args_count{ arg_list.required_args_count() };

	if (arglist_required_args_count <= arglist_args_count) {
		return arglist_args_count - arglist_required_args_count;
	}

	if (!arg_list.is_variadic()) {
		throw verify_error("arglist args count less than required args count, and no variadic arg");
	}

	if ((arglist_args_count + 1) == arglist_required_args_count) {
		return 0;
	}

	assert((arglist_args_count + 1) < arglist_required_args_count);
	throw verify_error("arglist args count less than required args count, despite variadic arg");
}

std::size_t Verify_call_parameters::calc_min_args_count(
	const Type_spec& type_specification,
	bool required)
{
	std::size_t args_count{ type_specification.args.size() };

	if (is_method && required) {
		if (args_count == 0) {
			throw verify_error("method call needs at least one argument - object");
		}
		// decrement due to obligatory specifier for object ('O' or 'o')
		--args_count;
	}

	if (type_specification.variadic == Type_spec::Variadic::One_or_more) {
		++args_count;
	}

	return args_count;
}

std::invalid_argument Verify_call_parameters::verify_error(const util::string& reason)
{
	std::ostringstream os;
	os << "verification of call params failed: " << reason;
	return std::invalid_argument(os.str());
}

} // anonymous namespace

void verify_call_parameters(
	bool is_method,
	zend_execute_data* execute_data,
	const char* type_spec)
{
	Verify_call_parameters verify_parameters(is_method, execute_data, type_spec);
	verify_parameters.run();
}

// ------------

bool is_module_loaded(const char* module_name)
{
	zend_string* module_zname{ to_zend_string(module_name) };
	const bool is_loaded{ zend_hash_exists(&module_registry, module_zname) ? true : false };
	zend_string_release(module_zname);
	return is_loaded;
}

bool is_openssl_loaded()
{
	const char* Openssl_module_name{ "openssl" };
	return is_module_loaded(Openssl_module_name);
}

} // namespace zend

} // namespace util

} // namespace mysqlx
