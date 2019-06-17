--TEST--
mysqlx authentication mechanisms - unsecure caching_sha2_password warnings
--SKIPIF--
--INI--
error_reporting=E_ALL
default_socket_timeout=4
--FILE--
<?php
require_once(__DIR__."/auth_utils.inc");

// setup
$test_user = $Test_user_sha2;
$ssl_query = prepare_ssl_query();
reset_test_user($test_user, 'caching_sha2_password');

test_unsecure_connection($test_user, null, false, true);
test_unsecure_connection($test_user, 'MYSQL41', false, true);
test_unsecure_connection($test_user, 'SHA256_MEMORY', false, true);
test_unsecure_connection($test_user, 'PLAIN', false, true);
test_unsecure_connection($test_user, 'EXTERNAL', false, true);
test_unsecure_connection($test_user, 'UNSUPPORTED', false, true);

test_secure_connection($test_user, 'PLAIN', true, false);
test_unsecure_connection($test_user, 'SHA256_MEMORY', true, true);
test_unsecure_connection($Test_user_unknown, 'SHA256_MEMORY', false, true);

reset_test_user($test_user, 'caching_sha2_password');
test_secure_connection($test_user, null, true, false);
test_unsecure_connection($test_user, 'sha256_memory', true, true);
test_unsecure_connection($Test_user_unknown, 'sha256_memory', false, true);

verify_expectations();
print "done!\n";
?>
--CLEAN--
<?php
	require_once(__DIR__."/auth_utils.inc");
	clean_test_db();
?>
--EXPECTF--
mysqlx://mysql_xdevapi_test_user_sha2:mysql_xdevapi_test_user_sha2_password@localhost:%s/?ssl-mode=disabled

Warning: mysql_xdevapi\getSession(): [10054][HY000] Authentication failure. Authentication failed using MYSQL41, SHA256_MEMORY. Check username and password or try a secure connection in %s on line %d
[10054][HY000] Authentication failure. Authentication failed using MYSQL41, SHA256_MEMORY. Check username and password or try a secure connection
----------------------
mysqlx://mysql_xdevapi_test_user_sha2:mysql_xdevapi_test_user_sha2_password@localhost:%s/?ssl-mode=disabled&auth=MYSQL41

Warning: mysql_xdevapi\getSession(): [1045][HY000] Access denied for user 'mysql_xdevapi_test_user_sha2'@'localhost' (using password: YES) in %s on line %d
[1045][HY000] Access denied for user 'mysql_xdevapi_test_user_sha2'@'localhost' (using password: YES)
----------------------
mysqlx://mysql_xdevapi_test_user_sha2:mysql_xdevapi_test_user_sha2_password@localhost:%s/?ssl-mode=disabled&auth=SHA256_MEMORY

Warning: mysql_xdevapi\getSession(): [1045][HY000] Access denied for user 'mysql_xdevapi_test_user_sha2'@'localhost' (using password: YES) in %s on line %d
[1045][HY000] Access denied for user 'mysql_xdevapi_test_user_sha2'@'localhost' (using password: YES)
----------------------
mysqlx://mysql_xdevapi_test_user_sha2:mysql_xdevapi_test_user_sha2_password@localhost:%s/?ssl-mode=disabled&auth=PLAIN

Warning: mysql_xdevapi\getSession(): [1251][HY000] Invalid authentication method PLAIN in %s on line %d
[1251][HY000] Invalid authentication method PLAIN
----------------------
mysqlx://mysql_xdevapi_test_user_sha2:mysql_xdevapi_test_user_sha2_password@localhost:%s/?ssl-mode=disabled&auth=EXTERNAL

Warning: mysql_xdevapi\getSession(): [1251][HY000] Invalid authentication method EXTERNAL in %s on line %d
[1251][HY000] Invalid authentication method EXTERNAL
----------------------
mysqlx://mysql_xdevapi_test_user_sha2:mysql_xdevapi_test_user_sha2_password@localhost:%s/?ssl-mode=disabled&auth=UNSUPPORTED
[10046][HY000] Invalid authentication mechanism UNSUPPORTED
----------------------
mysqlx://mysql_xdevapi_test_user_sha2:mysql_xdevapi_test_user_sha2_password@localhost:%s/?ssl-mode=disabled&auth=SHA256_MEMORY
----------------------
mysqlx://mysql_xdevapi_test_user_unknown:mysql_xdevapi_test_user_unknown_password@localhost:%s/?ssl-mode=disabled&auth=SHA256_MEMORY

Warning: mysql_xdevapi\getSession(): [1045][HY000] Access denied for user 'mysql_xdevapi_test_user_unknown'@'localhost' (using password: YES) in %s on line %d
[1045][HY000] Access denied for user 'mysql_xdevapi_test_user_unknown'@'localhost' (using password: YES)
----------------------
mysqlx://mysql_xdevapi_test_user_sha2:mysql_xdevapi_test_user_sha2_password@localhost:%s/?ssl-mode=disabled&auth=sha256_memory
----------------------
mysqlx://mysql_xdevapi_test_user_unknown:mysql_xdevapi_test_user_unknown_password@localhost:%s/?ssl-mode=disabled&auth=sha256_memory

Warning: mysql_xdevapi\getSession(): [1045][HY000] Access denied for user 'mysql_xdevapi_test_user_unknown'@'localhost' (using password: YES) in %s on line %d
[1045][HY000] Access denied for user 'mysql_xdevapi_test_user_unknown'@'localhost' (using password: YES)
----------------------
done!%A
