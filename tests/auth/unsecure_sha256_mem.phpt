--TEST--
mysqlx authentication mechanisms - unsecure caching_sha2_password
--SKIPIF--
--INI--
error_reporting=1
default_socket_timeout=4
--FILE--
<?php
require_once(__DIR__."/../connect.inc");
require_once(__DIR__."/auth_utils.inc");

// setup
$test_user = DEVAPI_EXT_NAME.'_test_user_sha2';
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
	require_once(__DIR__."/../connect.inc");
	clean_test_db();
?>
--EXPECTF--
done!%A
