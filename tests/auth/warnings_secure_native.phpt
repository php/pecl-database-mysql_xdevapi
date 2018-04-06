--TEST--
mysqlx authentication mechanisms - secure mysql_native_password warnings
--SKIPIF--
--INI--
error_reporting=E_ALL
default_socket_timeout=4
--FILE--
<?php
require_once(__DIR__."/../connect.inc");
require_once(__DIR__."/auth_utils.inc");

// setup
$test_user = $Test_user_native;
$ssl_query = prepare_ssl_query();
reset_test_user($test_user, 'mysql_native_password');

test_secure_connection($test_user, 'sha256_memory', false, true);
test_secure_connection($test_user, 'external', false, true);
test_secure_connection($test_user, 'unknown', false, true);

test_secure_connection($Test_user_unknown, null, false, true);
test_secure_connection($Test_user_unknown, 'mysql41', false, true);
test_secure_connection($Test_user_unknown, 'plain', false, true);

verify_expectations();
print "done!\n";
?>
--CLEAN--
<?php
	require_once(__DIR__."/../connect.inc");
	clean_test_db();
?>
--EXPECTF--
mysqlx://mysql_xdevapi_test_user_native:mysql_xdevapi_test_user_native_password@%s/?ssl-%s&auth=sha256_memory

Warning: mysql_xdevapi\getSession(): [1045][HY000] Invalid user or password in %s
----------------------
mysqlx://mysql_xdevapi_test_user_native:mysql_xdevapi_test_user_native_password@%s/?ssl-%s&auth=external

Warning: mysql_xdevapi\getSession(): [1251][HY000] Invalid authentication method EXTERNAL in %s

Warning: mysql_xdevapi\getSession(): SSL: An established connection was aborted by the software in your host machine.
 in %s
----------------------
mysqlx://mysql_xdevapi_test_user_native:mysql_xdevapi_test_user_native_password@%s/?ssl-%s&auth=unknown
----------------------
mysqlx://mysql_xdevapi_test_user_unknown:mysql_xdevapi_test_user_unknown_password@%s/?ssl-%s

Warning: mysql_xdevapi\getSession(): [1045][HY000] Invalid user or password in %s
----------------------
mysqlx://mysql_xdevapi_test_user_unknown:mysql_xdevapi_test_user_unknown_password@%s/?ssl-%s&auth=mysql41

Warning: mysql_xdevapi\getSession(): [1045][HY000] Invalid user or password in %s
----------------------
mysqlx://mysql_xdevapi_test_user_unknown:mysql_xdevapi_test_user_unknown_password@%s/?ssl-%s&auth=plain

Warning: mysql_xdevapi\getSession(): [1045][HY000] Invalid user or password in %s
----------------------
done!%A
