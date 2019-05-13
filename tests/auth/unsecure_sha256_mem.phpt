--TEST--
mysqlx authentication mechanisms - unsecure caching_sha2_password
--SKIPIF--
--INI--
error_reporting=1
default_socket_timeout=4
--FILE--
<?php
require_once(__DIR__."/auth_utils.inc");

// setup
$test_user = $Test_user_sha2;
reset_test_user($test_user, 'caching_sha2_password');


test_unsecure_connection($test_user, null, false);

test_unsecure_connection($test_user, 'MYSQL41', false);
test_unsecure_connection($test_user, 'PLAIN', false);
test_unsecure_connection($test_user, 'SHA256_MEMORY', false);
test_unsecure_connection($test_user, 'EXTERNAL', false);
test_unsecure_connection($test_user, 'UNSUPPORTED', false);

test_unsecure_connection($test_user, 'mysql41', false);
test_unsecure_connection($test_user, 'plain', false);
test_unsecure_connection($test_user, 'sha256_memory', false);
test_unsecure_connection($test_user, 'external', false);
test_unsecure_connection($test_user, 'nonworking', false);

test_unsecure_connection($test_user, 'MyqQl41', false);
test_unsecure_connection($test_user, 'PlaiN', false);
test_unsecure_connection($test_user, 'Sha256_memoRy', false);
test_unsecure_connection($test_user, 'EXTernal', false);
test_unsecure_connection($test_user, 'imProper', false);

/*
	it will store cached password for SHA256_MEMORY, so this time mentioned auth
	mechanism will succeed, in opposition to previous trials
*/
$ssl_query = prepare_ssl_query();
test_secure_connection($test_user, 'PLAIN');

test_unsecure_connection($test_user, null);
test_unsecure_connection($test_user, 'SHA256_memory');

// ----------
reset_test_user($test_user, 'caching_sha2_password');
test_unsecure_connection($test_user, null, false);
test_unsecure_connection($test_user, 'SHA256_memory', false);
// will login with PLAIN and cache passwd as previously
test_secure_connection($test_user, null);
test_unsecure_connection($test_user, null);
test_unsecure_connection($test_user, 'SHA256_memory');


verify_expectations();
print "done!\n";
?>
--CLEAN--
<?php
	require_once(__DIR__."/auth_utils.inc");
	clean_test_db();
?>
--EXPECTF--
[10054][HY000] Authentication failure. Authentication failed using MYSQL41, SHA256_MEMORY. Check username and password or try a secure connection
[1045][HY000] Access denied for user 'mysql_xdevapi_test_user_sha2'@'localhost' (using password: YES)
[1251][HY000] Invalid authentication method PLAIN
[1045][HY000] Access denied for user 'mysql_xdevapi_test_user_sha2'@'localhost' (using password: YES)
[1251][HY000] Invalid authentication method EXTERNAL
[10046][HY000] Invalid authentication mechanism UNSUPPORTED
[1045][HY000] Access denied for user 'mysql_xdevapi_test_user_sha2'@'localhost' (using password: YES)
[1251][HY000] Invalid authentication method PLAIN
[1045][HY000] Access denied for user 'mysql_xdevapi_test_user_sha2'@'localhost' (using password: YES)
[1251][HY000] Invalid authentication method EXTERNAL
[10046][HY000] Invalid authentication mechanism nonworking
[10046][HY000] Invalid authentication mechanism MyqQl41
[1251][HY000] Invalid authentication method PLAIN
[1045][HY000] Access denied for user 'mysql_xdevapi_test_user_sha2'@'localhost' (using password: YES)
[1251][HY000] Invalid authentication method EXTERNAL
[10046][HY000] Invalid authentication mechanism imProper
[10054][HY000] Authentication failure. Authentication failed using MYSQL41, SHA256_MEMORY. Check username and password or try a secure connection
[1045][HY000] Access denied for user 'mysql_xdevapi_test_user_sha2'@'localhost' (using password: YES)
done!%A
