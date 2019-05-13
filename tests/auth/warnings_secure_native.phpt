--TEST--
mysqlx authentication mechanisms - secure mysql_native_password warnings
--SKIPIF--
--INI--
error_reporting=E_ALL
default_socket_timeout=4
--FILE--
<?php
require_once(__DIR__."/auth_utils.inc");

// setup
$test_user = $Test_user_native;
$ssl_query = prepare_ssl_query();
reset_test_user($test_user, 'mysql_native_password');

test_secure_connection($test_user, 'sha256_memory', false, true);
test_secure_connection($test_user, 'unknown', false, true);

test_secure_connection($Test_user_unknown, null, false, true);
test_secure_connection($Test_user_unknown, 'mysql41', false, true);
test_secure_connection($Test_user_unknown, 'plain', false, true);
test_secure_connection($test_user, 'external', false, true);

verify_expectations();
print "done!\n";
?>
--CLEAN--
<?php
	require_once(__DIR__."/auth_utils.inc");
	clean_test_db();
?>
--EXPECTF--
mysqlx://mysql_xdevapi_test_user_native:mysql_xdevapi_test_user_native_password@localhost:%s/?ssl-key=%s&auth=sha256_memory

Warning: mysql_xdevapi\getSession(): [1045][HY000] Access denied for user 'mysql_xdevapi_test_user_native'@'localhost' (using password: YES) in %s
[1045][HY000] Access denied for user 'mysql_xdevapi_test_user_native'@'localhost' (using password: YES)
----------------------
mysqlx://mysql_xdevapi_test_user_native:mysql_xdevapi_test_user_native_password@localhost:%s/?ssl-key=%s&auth=unknown
[10046][HY000] Invalid authentication mechanism unknown
----------------------
mysqlx://mysql_xdevapi_test_user_unknown:mysql_xdevapi_test_user_unknown_password@localhost:%s/?ssl-key=%s

Warning: mysql_xdevapi\getSession(): [1045][HY000] Access denied for user 'mysql_xdevapi_test_user_unknown'@'localhost' (using password: YES) in %s
[1045][HY000] Access denied for user 'mysql_xdevapi_test_user_unknown'@'localhost' (using password: YES)
----------------------
mysqlx://mysql_xdevapi_test_user_unknown:mysql_xdevapi_test_user_unknown_password@localhost:%s/?ssl-key=%s&auth=mysql41

Warning: mysql_xdevapi\getSession(): [1045][HY000] Access denied for user 'mysql_xdevapi_test_user_unknown'@'localhost' (using password: YES) in %s
[1045][HY000] Access denied for user 'mysql_xdevapi_test_user_unknown'@'localhost' (using password: YES)
----------------------
mysqlx://mysql_xdevapi_test_user_unknown:mysql_xdevapi_test_user_unknown_password@localhost:%s/?ssl-key=%s&auth=plain

Warning: mysql_xdevapi\getSession(): [1045][HY000] Access denied for user 'mysql_xdevapi_test_user_unknown'@'localhost' (using password: YES) in %s
[1045][HY000] Access denied for user 'mysql_xdevapi_test_user_unknown'@'localhost' (using password: YES)
----------------------
mysqlx://mysql_xdevapi_test_user_native:mysql_xdevapi_test_user_native_password@localhost:%s/?ssl-key=%s&auth=external

Warning: mysql_xdevapi\getSession(): [1251][HY000] Invalid authentication method EXTERNAL in %s %A
[1251][HY000] Invalid authentication method EXTERNAL
----------------------
done!%A
