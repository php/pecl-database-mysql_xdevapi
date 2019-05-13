--TEST--
mysqlx authentication mechanisms - secure caching_sha2_password warnings
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

test_secure_connection($test_user, 'MYSQL41', false, true);
test_secure_connection($test_user, 'SHA256_MEMORY', false, true);
test_secure_connection($test_user, 'EXTERNAL', false, true);
test_secure_connection($test_user, 'UNKNOWN', false, true);

test_secure_connection($test_user, 'PLAIN', true, false);
test_secure_connection($test_user, 'SHA256_MEMORY', true, true);
test_secure_connection($Test_user_unknown, 'SHA256_MEMORY', false, true);

reset_test_user($test_user, 'caching_sha2_password');
test_secure_connection($test_user, null, true, false);
test_secure_connection($test_user, 'sha256_memory', true, true);
test_secure_connection($Test_user_unknown, 'sha256_memory', false, true);

verify_expectations();
print "done!\n";
?>
--CLEAN--
<?php
	require_once(__DIR__."/auth_utils.inc");
	clean_test_db();
?>
--EXPECTF--
mysqlx://mysql_xdevapi_test_user_sha2:mysql_xdevapi_test_user_sha2_password@localhost:%s/?ssl-key=%s&auth=MYSQL41

Warning: mysql_xdevapi\getSession(): [1045][HY000] Access denied for user 'mysql_xdevapi_test_user_sha2'@'localhost' (using password: YES) in %s
[1045][HY000] Access denied for user 'mysql_xdevapi_test_user_sha2'@'localhost' (using password: YES)
----------------------
mysqlx://mysql_xdevapi_test_user_sha2:mysql_xdevapi_test_user_sha2_password@localhost:%s/?ssl-key=%s&auth=SHA256_MEMORY

Warning: mysql_xdevapi\getSession(): [1045][HY000] Access denied for user 'mysql_xdevapi_test_user_sha2'@'localhost' (using password: YES) in %s
[1045][HY000] Access denied for user 'mysql_xdevapi_test_user_sha2'@'localhost' (using password: YES)
----------------------
mysqlx://mysql_xdevapi_test_user_sha2:mysql_xdevapi_test_user_sha2_password@localhost:%s/?ssl-key=%s&auth=EXTERNAL

Warning: mysql_xdevapi\getSession(): [1251][HY000] Invalid authentication method EXTERNAL in %s %A
[1251][HY000] Invalid authentication method EXTERNAL
----------------------
mysqlx://mysql_xdevapi_test_user_sha2:mysql_xdevapi_test_user_sha2_password@localhost:%s/?ssl-key=%s&auth=UNKNOWN
[10046][HY000] Invalid authentication mechanism UNKNOWN
----------------------
mysqlx://mysql_xdevapi_test_user_sha2:mysql_xdevapi_test_user_sha2_password@localhost:%s/?ssl-key=%s&auth=SHA256_MEMORY
----------------------
mysqlx://mysql_xdevapi_test_user_unknown:mysql_xdevapi_test_user_unknown_password@localhost:%s/?ssl-key=%s&auth=SHA256_MEMORY

Warning: mysql_xdevapi\getSession(): [1045][HY000] Access denied for user 'mysql_xdevapi_test_user_unknown'@'localhost' (using password: YES) in %s
[1045][HY000] Access denied for user 'mysql_xdevapi_test_user_unknown'@'localhost' (using password: YES)
----------------------
mysqlx://mysql_xdevapi_test_user_sha2:mysql_xdevapi_test_user_sha2_password@localhost:%s/?ssl-key=%s&auth=sha256_memory
----------------------
mysqlx://mysql_xdevapi_test_user_unknown:mysql_xdevapi_test_user_unknown_password@localhost:%s/?ssl-key=%s&auth=sha256_memory

Warning: mysql_xdevapi\getSession(): [1045][HY000] Access denied for user 'mysql_xdevapi_test_user_unknown'@'localhost' (using password: YES) in %s
[1045][HY000] Access denied for user 'mysql_xdevapi_test_user_unknown'@'localhost' (using password: YES)
----------------------
done!%A
