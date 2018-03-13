--TEST--
mysqlx SHA256_MEMORY authentication mechanism
--SKIPIF--
--INI--
error_reporting=1
default_socket_timeout=4
--FILE--
<?php
require_once(__DIR__."/../connect.inc");
require_once(__DIR__."/auth_utils.inc");

// setup
$test_user = 'user_sha256_mem';

$ssl_query = prepare_ssl_query();

// ----------
reset_test_user($test_user, 'caching_sha2_password');

test_unsecure_connection($test_user, null, false);
test_unsecure_connection($test_user, 'PLAIN', false);
test_unsecure_connection($test_user, 'SHA256_MEMORY', false);
test_secure_connection($test_user, 'SHA256_MEMORY', false);

// it will store cached passwd on server for next logins
test_secure_connection($test_user, 'PLAIN');
test_unsecure_connection($test_user, 'SHA256_MEMORY');
test_unsecure_connection($test_user, null);
test_secure_connection($test_user, 'SHA256_MEMORY');
test_secure_connection($test_user, 'MySQL41', false);

// ----------
reset_test_user($test_user, 'caching_sha2_password');

test_secure_connection($test_user, 'SHA256_MEMORY', false);
test_unsecure_connection($test_user, 'SHA256_MEMORY', false);
test_unsecure_connection($test_user, null, false);

// it will login with PLAIN and store cached passwd on server for next logins
test_secure_connection($test_user, null);
test_unsecure_connection($test_user, 'SHA256_MEMORY');
test_unsecure_connection($test_user, null);
test_secure_connection($test_user, 'SHA256_MEMORY');
test_secure_connection($test_user, 'MySQL41', false);

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
