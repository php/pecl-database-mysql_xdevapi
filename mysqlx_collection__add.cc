/*
  +----------------------------------------------------------------------+
  | PHP Version 7                                                        |
  +----------------------------------------------------------------------+
  | Copyright (c) 2006-2020 The PHP Group                                |
  +----------------------------------------------------------------------+
  | This source file is subject to version 3.01 of the PHP license,      |
  | that is bundled with this package in the file LICENSE, and is        |
  | available through the world-wide-web at the following url:           |
  | http://www.php.net/license/3_01.txt                                  |
  | If you did not receive a copy of the PHP license and are unable to   |
  | obtain it through the world-wide-web, please send a note to          |
  | license@php.net so we can mail you a copy immediately.               |
  +----------------------------------------------------------------------+
  | Authors: Andrey Hristov <andrey@php.net>                             |
  |          Filip Janiszewski <fjanisze@php.net>                        |
  |          Darek Slusarczyk <marines@php.net>                          |
  +----------------------------------------------------------------------+
*/
#include "php_api.h"
#include "mysqlnd_api.h"
#include "json_api.h"
#include "xmysqlnd/xmysqlnd.h"
#include "xmysqlnd/xmysqlnd_session.h"
#include "xmysqlnd/xmysqlnd_schema.h"
#include "xmysqlnd/xmysqlnd_stmt.h"
#include "xmysqlnd/xmysqlnd_collection.h"
#include "xmysqlnd/xmysqlnd_crud_collection_commands.h"
#include "php_mysqlx.h"
#include "mysqlx_exception.h"
#include "mysqlx_class_properties.h"
#include "mysqlx_executable.h"
#include "mysqlx_sql_statement.h"
#include "mysqlx_collection__add.h"
#include "mysqlx_exception.h"
#include "util/allocator.h"
#include "util/json_utils.h"
#include "util/object.h"
#include "util/strings.h"
#include "util/string_utils.h"
#include "util/zend_utils.h"

namespace mysqlx {

namespace devapi {

using namespace drv;

namespace {

zend_class_entry* collection_add_class_entry;

ZEND_BEGIN_ARG_INFO_EX(arginfo_mysqlx_collection__add__execute, 0, ZEND_RETURN_VALUE, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_mysqlx_collection__add__add, 0, ZEND_RETURN_VALUE, 1)
	ZEND_ARG_INFO(no_pass_by_ref, json)
ZEND_END_ARG_INFO()


//------------------------------------------------------------------------------

enum_func_status
execute_statement(xmysqlnd_stmt& stmt, zval* resultset)
{
	util::zvalue stmt_zv;
	mysqlx_new_stmt(stmt_zv.ptr(), &stmt);
	if (stmt_zv.is_null()) {
		xmysqlnd_stmt_free(&stmt, nullptr, nullptr);
		return FAIL;
	}
	if (!stmt_zv.is_object()) {
		return FAIL;
	}
	zend_long flags{0};
	mysqlx_statement_execute_read_response(Z_MYSQLX_P(stmt_zv.ptr()),
						flags, MYSQLX_RESULT, resultset);
	return PASS;
}

enum class Add_op_status
{
	success,
	fail,
	noop
};

Add_op_status
collection_add_string(
	st_xmysqlnd_crud_collection_op__add* add_op,
	util::zvalue& doc)
{
	if( PASS == xmysqlnd_crud_collection_add__add_doc(add_op, doc.ptr()) ) {
		return Add_op_status::success;
	}
	return Add_op_status::fail;
}

Add_op_status
collection_add_object(
	st_xmysqlnd_crud_collection_op__add* add_op,
	util::zvalue& doc)
{
	Add_op_status ret = Add_op_status::fail;
	util::zvalue new_doc(util::json::encode_document(doc));
	if( PASS == xmysqlnd_crud_collection_add__add_doc(add_op, new_doc.ptr()) ) {
		ret = Add_op_status::success;
	}
	return ret;
}

Add_op_status
collection_add_array(
	st_xmysqlnd_crud_collection_op__add* add_op,
	util::zvalue& doc)
{
	Add_op_status ret = Add_op_status::fail;
	if( doc.empty() ) {
		ret = Add_op_status::noop;
	} else {
		ret = collection_add_object(add_op, doc);
	}
	return ret;
}

} // anonymous namespace

//------------------------------------------------------------------------------


bool Collection_add::add_docs(
	xmysqlnd_collection* coll,
	zval* documents,
	int num_of_documents)
{
	if (!documents || !num_of_documents) return false;

	for (int i{0}; i < num_of_documents; ++i) {
		auto doc_type{ Z_TYPE(documents[i]) };
		if (doc_type != IS_STRING &&
			doc_type != IS_OBJECT &&
			doc_type != IS_ARRAY) {
			php_error_docref(
				nullptr,
				E_WARNING,
				"Only strings, objects and arrays can be added. Type is %u",
				doc_type);
			return false;
		}
	}

	if( !add_op ) {
		if( !coll ) return false;
		collection = coll->get_reference();
		add_op = xmysqlnd_crud_collection_add__create(
			collection->get_schema()->get_name(),
			collection->get_name());
		if (!add_op) return false;
	}

	for (int i{0}; i < num_of_documents; ++i) {
		docs.push_back(util::zvalue::clone_from(&documents[i]));
	}

	return true;
}

bool Collection_add::add_docs(
	xmysqlnd_collection* coll,
	const util::string_view& /*doc_id*/,
	zval* doc)
{
	const int num_of_documents = 1;
	if (!add_docs(coll, doc, num_of_documents)) return false;
	return xmysqlnd_crud_collection_add__set_upsert(add_op) == PASS;
}

Collection_add::~Collection_add()
{
	if (add_op) {
		xmysqlnd_crud_collection_add__destroy(add_op);
	}

	if (collection) {
		xmysqlnd_collection_free(collection, nullptr, nullptr);
	}
}

void Collection_add::execute(zval* resultset)
{
	DBG_ENTER("Collection_add::execute");

	size_t noop_cnt{0};
	Add_op_status ret = Add_op_status::success;
	for (auto it{docs.begin()}; it != docs.end() && ret != Add_op_status::fail ; ++it) {
		ret = Add_op_status::fail;
		util::zvalue doc(*it);
		switch(doc.type()) {
		case util::zvalue::Type::String:
			ret = collection_add_string(add_op, doc);
			break;
		case util::zvalue::Type::Array:
			ret = collection_add_array(add_op, doc);
			break;
		case util::zvalue::Type::Object:
			ret = collection_add_object(add_op, doc);
			break;
		default:
			assert(!"should not happen!");
			ret = Add_op_status::fail;
		}
		if( ret == Add_op_status::noop ) {
			++noop_cnt;
		}
	}

	enum_func_status execute_ret_status{PASS};
	if ( docs.size() > noop_cnt ) {
		xmysqlnd_stmt* stmt = collection->add(add_op);
		if( nullptr != stmt ) {
			execute_ret_status = execute_statement(*stmt, resultset);
		} else {
			execute_ret_status = FAIL;
		}
	}
	if (FAIL == execute_ret_status && !EG(exception)) {
		RAISE_EXCEPTION(err_msg_add_doc);
	}
	DBG_VOID_RETURN;
}

//------------------------------------------------------------------------------


MYSQL_XDEVAPI_PHP_METHOD(mysqlx_collection__add, __construct)
{
	UNUSED_INTERNAL_FUNCTION_PARAMETERS();
}

MYSQL_XDEVAPI_PHP_METHOD(mysqlx_collection__add, execute)
{
	DBG_ENTER("mysqlx_collection__add::execute");

	zval* object_zv{nullptr};
	if (FAILURE == util::zend::parse_method_parameters(execute_data, getThis(), "O",
												&object_zv,
												collection_add_class_entry))
	{
		DBG_VOID_RETURN;
	}

	Collection_add& coll_add = util::fetch_data_object<Collection_add>(object_zv);
	coll_add.execute(return_value);

	DBG_VOID_RETURN;
}

MYSQL_XDEVAPI_PHP_METHOD(mysqlx_collection__add, add)
{
	zval* object_zv{nullptr};
	zval* docs{nullptr};
	int num_of_docs{0};

	DBG_ENTER("mysqlx_collection::add");

	if (FAILURE == util::zend::parse_method_parameters(execute_data, getThis(), "O+",
												&object_zv,
												collection_add_class_entry,
												&docs,
												&num_of_docs))
	{
		DBG_VOID_RETURN;
	}

	/*
	 * For subsequent calls to add_docs, the xmysqlnd_collection is set to NULL
	 */
	Collection_add& coll_add = util::fetch_data_object<Collection_add>(object_zv);
	if (coll_add.add_docs(nullptr, docs, num_of_docs)) {
		util::zvalue::copy_to(object_zv, return_value);
	}

	DBG_VOID_RETURN;
}

static const zend_function_entry mysqlx_collection__add_methods[] = {
	PHP_ME(mysqlx_collection__add, __construct,	nullptr,											ZEND_ACC_PRIVATE)

	PHP_ME(mysqlx_collection__add,	execute,		arginfo_mysqlx_collection__add__execute,	ZEND_ACC_PUBLIC)
	PHP_ME(mysqlx_collection__add,	add,			arginfo_mysqlx_collection__add__add,		ZEND_ACC_PUBLIC)

	{nullptr, nullptr, nullptr}
};

#if 0
static zval *
mysqlx_collection__add_property__name(const st_mysqlx_object* obj, zval* return_value)
{
	const Collection_add* object = (const Collection_add *) (obj->ptr);
	DBG_ENTER("mysqlx_collection__add_property__name");
	if (object->collection && !object->collection->get_name().empty()) {
		ZVAL_STRINGL(return_value, object->collection->get_name().data(), object->collection->get_name().length());
	} else {
		/*
		  This means EG(uninitialized_value). If we return just return_value, this is an UNDEF-ed value
		  and ISSET will say 'true' while for EG(unin) it is false.
		  In short:
		  return nullptr; -> isset()===false, value is nullptr
		  return return_value; (without doing ZVAL_XXX)-> isset()===true, value is nullptr
		*/
		return_value = nullptr;
	}
	DBG_RETURN(return_value);
}

#endif

static zend_object_handlers collection_add_handlers;
static HashTable collection_add_properties;

const st_mysqlx_property_entry collection_add_property_entries[] =
{
#if 0
	{std::string_view("name"), mysqlx_collection__add_property__name,	nullptr},
#endif
	{std::string_view{}, nullptr, nullptr}
};

static void
mysqlx_collection__add_free_storage(zend_object* object)
{
	util::free_object<Collection_add>(object);
}

static zend_object *
php_mysqlx_collection__add_object_allocator(zend_class_entry* class_type)
{
	DBG_ENTER("php_mysqlx_collection__add_object_allocator");
	st_mysqlx_object* mysqlx_object = util::alloc_object<Collection_add>(
		class_type,
		&collection_add_handlers,
		&collection_add_properties);
	DBG_RETURN(&mysqlx_object->zo);
}

void
mysqlx_register_collection__add_class(UNUSED_INIT_FUNC_ARGS, zend_object_handlers* mysqlx_std_object_handlers)
{
	MYSQL_XDEVAPI_REGISTER_CLASS(
		collection_add_class_entry,
		"CollectionAdd",
		mysqlx_std_object_handlers,
		collection_add_handlers,
		php_mysqlx_collection__add_object_allocator,
		mysqlx_collection__add_free_storage,
		mysqlx_collection__add_methods,
		collection_add_properties,
		collection_add_property_entries,
		mysqlx_executable_interface_entry);

#if 0
	/* The following is needed for the Reflection API */
	zend_declare_property_null(collection_add_class_entry, "name",	sizeof("name") - 1,	ZEND_ACC_PUBLIC);
#endif
}

void
mysqlx_unregister_collection__add_class(UNUSED_SHUTDOWN_FUNC_ARGS)
{
	zend_hash_destroy(&collection_add_properties);
}

void
mysqlx_new_collection__add(
	zval* return_value,
	xmysqlnd_collection* collection,
	zval* docs,
	int num_of_docs)
{
	DBG_ENTER("mysqlx_new_collection__add");

	Collection_add& coll_add{ util::init_object<Collection_add>(collection_add_class_entry, return_value) };
	if (!coll_add.add_docs(collection, docs, num_of_docs)) {
		zval_ptr_dtor(return_value);
		ZVAL_NULL(return_value);
		throw util::xdevapi_exception(util::xdevapi_exception::Code::add_doc);
	}

	DBG_VOID_RETURN;
}

} // namespace devapi

} // namespace mysqlx
