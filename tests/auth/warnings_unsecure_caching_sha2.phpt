--TEST--
mysqlx authentication mechanisms - warnings
--SKIPIF--
--INI--
error_reporting=E_ALL
default_socket_timeout=4
--FILE--
<?php
require_once(__DIR__."/../connect.inc");
require_once(__DIR__."/auth_utils.inc");

// setup
$test_user = DEVAPI_EXT_NAME.'_test_user_sha2';
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
test_unsecure_connection($Unknown_user, 'SHA256_MEMORY', false, true);

verify_expectations();
print "done!\n";
?>
--CLEAN--
<?php
	require_once(__DIR__."/../connect.inc");
	clean_test_db();
?>
--EXPECTF--
mysqlx://mysql_xdevapi_test_user_sha2:mysql_xdevapi_test_user_sha2_password@localhost:33160/?ssl-mode=disabled

Warning: mysql_xdevapi\getSession(): [HY000] Authentication failed using MYSQL41, SHA256_MEMORY. Check username and password or try a secure connection in %s
----------------------
mysqlx://mysql_xdevapi_test_user_sha2:mysql_xdevapi_test_user_sha2_password@localhost:33160/?ssl-mode=disabled&auth=MYSQL41

Warning: mysql_xdevapi\getSession(): [1045][HY000] Invalid user or password in %s
----------------------
mysqlx://mysql_xdevapi_test_user_sha2:mysql_xdevapi_test_user_sha2_password@localhost:33160/?ssl-mode=disabled&auth=SHA256_MEMORY

Warning: mysql_xdevapi\getSession(): [1045][HY000] Invalid user or password in %s
----------------------
mysqlx://mysql_xdevapi_test_user_sha2:mysql_xdevapi_test_user_sha2_password@localhost:33160/?ssl-mode=disabled&auth=PLAIN

Warning: mysql_xdevapi\getSession(): [1251][HY000] Invalid authentication method PLAIN in %s
----------------------
mysqlx://mysql_xdevapi_test_user_sha2:mysql_xdevapi_test_user_sha2_password@localhost:33160/?ssl-mode=disabled&auth=EXTERNAL

Warning: mysql_xdevapi\getSession(): [1251][HY000] Invalid authentication method EXTERNAL in %s
----------------------
mysqlx://mysql_xdevapi_test_user_sha2:mysql_xdevapi_test_user_sha2_password@localhost:33160/?ssl-mode=disabled&auth=UNSUPPORTED
----------------------
mysqlx://mysql_xdevapi_test_user_sha2:mysql_xdevapi_test_user_sha2_password@localhost:33160/?ssl-mode=disabled&auth=SHA256_MEMORY
----------------------
mysqlx://mysql_xdevapi_test_user_unknown:mysql_xdevapi_test_user_unknown_password@localhost:33160/?ssl-mode=disabled&auth=SHA256_MEMORY

Warning: mysql_xdevapi\getSession(): [1045][HY000] Invalid user or password in %s
----------------------
done!%A
