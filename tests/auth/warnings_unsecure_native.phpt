--TEST--
mysqlx authentication mechanisms - unsecure mysql_native_password warnings
--SKIPIF--
--INI--
error_reporting=E_ALL
default_socket_timeout=4
--FILE--
<?php
require_once(__DIR__."/auth_utils.inc");

// setup
$test_user = $Test_user_native;
reset_test_user($test_user, 'mysql_native_password');

test_unsecure_connection($test_user, 'sha256_memory', false, true);
test_unsecure_connection($test_user, 'plain', false, true);
test_unsecure_connection($test_user, 'external', false, true);
test_unsecure_connection($test_user, 'unsupported', false, true);

test_unsecure_connection($Test_user_unknown, null, false, true);
test_unsecure_connection($Test_user_unknown, 'mysql41', false, true);

verify_expectations();
print "done!\n";
?>
--CLEAN--
<?php
	require_once(__DIR__."/auth_utils.inc");
	clean_test_db();
?>
--EXPECTF--
mysqlx://mysql_xdevapi_test_user_native:mysql_xdevapi_test_user_native_password@localhost:%s/?ssl-mode=disabled&auth=sha256_memory

Warning: mysql_xdevapi\getSession(): [1045][HY000] Access denied for user 'mysql_xdevapi_test_user_native'@'localhost' (using password: YES) in %s on line %d
[1045][HY000] Access denied for user 'mysql_xdevapi_test_user_native'@'localhost' (using password: YES)
----------------------
mysqlx://mysql_xdevapi_test_user_native:mysql_xdevapi_test_user_native_password@localhost:%s/?ssl-mode=disabled&auth=plain

Warning: mysql_xdevapi\getSession(): [1251][HY000] Invalid authentication method PLAIN in %s on line %d
[1251][HY000] Invalid authentication method PLAIN
----------------------
mysqlx://mysql_xdevapi_test_user_native:mysql_xdevapi_test_user_native_password@localhost:%s/?ssl-mode=disabled&auth=external

Warning: mysql_xdevapi\getSession(): [1251][HY000] Invalid authentication method EXTERNAL in %s on line %d
[1251][HY000] Invalid authentication method EXTERNAL
----------------------
mysqlx://mysql_xdevapi_test_user_native:mysql_xdevapi_test_user_native_password@localhost:%s/?ssl-mode=disabled&auth=unsupported
[10046][HY000] Invalid authentication mechanism unsupported
----------------------
mysqlx://mysql_xdevapi_test_user_unknown:mysql_xdevapi_test_user_unknown_password@localhost:%s/?ssl-mode=disabled

Warning: mysql_xdevapi\getSession(): [10054][HY000] Authentication failure. Authentication failed using MYSQL41, SHA256_MEMORY. Check username and password or try a secure connection in %s on line %d
[10054][HY000] Authentication failure. Authentication failed using MYSQL41, SHA256_MEMORY. Check username and password or try a secure connection
----------------------
mysqlx://mysql_xdevapi_test_user_unknown:mysql_xdevapi_test_user_unknown_password@localhost:%s/?ssl-mode=disabled&auth=mysql41

Warning: mysql_xdevapi\getSession(): [1045][HY000] Access denied for user 'mysql_xdevapi_test_user_unknown'@'localhost' (using password: YES) in %s on line %d
[1045][HY000] Access denied for user 'mysql_xdevapi_test_user_unknown'@'localhost' (using password: YES)
----------------------
done!%A
