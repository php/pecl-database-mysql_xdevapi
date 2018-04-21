/*
  +----------------------------------------------------------------------+
  | PHP Version 7                                                        |
  +----------------------------------------------------------------------+
  | Copyright (c) 2006-2018 The PHP Group                                |
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
#include <boost/algorithm/string/classification.hpp>
#include <boost/algorithm/string/split.hpp>

namespace mysqlx {

namespace util {

namespace zend {

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
	zend_bool persistent)
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

	util::string args;
	Variadic variadic;
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
	std::size_t calc_min_args_count(const Type_spec& type_spec);
	void raise_error(const util::string& reason);

private:
	const bool is_method;
	const uint32_t call_args_count;
	const uint32_t arglist_required_args_count;
	const util::string type_spec;
};

// ------------

Verify_call_parameters::Verify_call_parameters(
	bool is_method,
	zend_execute_data* execute_data, 
	const char* type_spec)
	: is_method( is_method )
	, call_args_count( execute_data->func->common.num_args )
	, arglist_required_args_count( execute_data->func->common.required_num_args )
	, type_spec( type_spec )
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
		raise_error("only one optional args block is allowed");
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
		raise_error("variadic specificator is always last char or it is invalid");
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

void Verify_call_parameters::validate_type_spec(const Type_spec& type_spec)
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

	const std::size_t disallowed_type_idx{ type_spec.args.find_first_not_of(allowed_types) };
	if (disallowed_type_idx != util::string::npos) {
		raise_error("unknown type in type_spec");
	}
}

void Verify_call_parameters::verify_variadic(
	const Type_spec& type_spec_required_args,
	const Type_spec& type_spec_optional_args)
{
	if (type_spec_required_args.is_variadic() && !type_spec_optional_args.empty()) {
		raise_error("variadic specificator is always last char in whole type_spec");
	}
}

void Verify_call_parameters::verify_required_args_count(
	const Type_spec& type_spec_required_args)
{
	const std::size_t type_spec_required_args_count{ calc_min_args_count(type_spec_required_args) };
	if (arglist_required_args_count != type_spec_required_args_count) {
		raise_error("required number of args in type_spec and arglist are different");
	}
}

std::size_t Verify_call_parameters::calc_min_args_count(const Type_spec& type_spec)
{
	std::size_t args_count{ type_spec.args.size() };

	if (is_method) {
		if (args_count == 0) {
			raise_error("method call needs at least one argument - object");
		}
		--args_count;
	}

	if (type_spec.variadic == Type_spec::Variadic::One_or_more) {
		++args_count;
	}

	return args_count;
}

void Verify_call_parameters::raise_error(const util::string& reason)
{
	std::ostringstream os;
	os << "verification of call params failed: " << reason;
	throw std::invalid_argument(os.str());
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

} // namespace zend

} // namespace util

} // namespace mysqlx

/*
 * Local variables:
 * tab-width: 4
 * c-basic-offset: 4
 * End:
 * vim600: noet sw=4 ts=4 fdm=marker
 * vim<600: noet sw=4 ts=4
 */
