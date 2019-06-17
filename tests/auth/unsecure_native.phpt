--TEST--
mysqlx authentication mechanisms - unsecure mysql_native_password
--SKIPIF--
--INI--
error_reporting=1
default_socket_timeout=4
--FILE--
<?php
require_once(__DIR__."/auth_utils.inc");

// setup
$test_user = $Test_user_native;
reset_test_user($test_user, 'mysql_native_password');

test_unsecure_connection($test_user, null);
test_unsecure_connection($test_user, 'MYSQL41');
test_unsecure_connection($test_user, 'PLAIN', false);
test_unsecure_connection($test_user, 'SHA256_MEMORY', false);
test_unsecure_connection($test_user, 'EXTERNAL', false);
test_unsecure_connection($test_user, 'WRONG', false);

test_unsecure_connection($test_user, 'mysql41');
test_unsecure_connection($test_user, 'plain', false);
test_unsecure_connection($test_user, 'sha256_memory', false);
test_unsecure_connection($test_user, 'external', false);
test_unsecure_connection($test_user, 'non-existent', false);

test_unsecure_connection($test_user, 'mySql41');
test_unsecure_connection($test_user, 'PLain', false);
test_unsecure_connection($test_user, 'sHa256_mEmOrY', false);
test_unsecure_connection($test_user, 'ExternaL', false);
test_unsecure_connection($test_user, 'InCorrect', false);

/*
	it will store cached password for SHA256_MEMORY, so this time mentioned auth
	mechanism will succeed, in opposition to previous trials
*/
$ssl_query = prepare_ssl_query();
test_secure_connection($test_user, 'PLAIN');
test_unsecure_connection($test_user, 'sha256_MEMORY');

verify_expectations();
print "done!\n";
?>
--CLEAN--
<?php
	require_once(__DIR__."/auth_utils.inc");
	clean_test_db();
?>
--EXPECTF--
[1251][HY000] Invalid authentication method PLAIN
[1045][HY000] Access denied for user 'mysql_xdevapi_test_user_native'@'localhost' (using password: YES)
[1251][HY000] Invalid authentication method EXTERNAL
[10046][HY000] Invalid authentication mechanism WRONG
[1251][HY000] Invalid authentication method PLAIN
[1045][HY000] Access denied for user 'mysql_xdevapi_test_user_native'@'localhost' (using password: YES)
[1251][HY000] Invalid authentication method EXTERNAL
[10046][HY000] Invalid authentication mechanism non-existent
[1251][HY000] Invalid authentication method PLAIN
[1045][HY000] Access denied for user 'mysql_xdevapi_test_user_native'@'localhost' (using password: YES)
[1251][HY000] Invalid authentication method EXTERNAL
[10046][HY000] Invalid authentication mechanism InCorrect
done!%A
