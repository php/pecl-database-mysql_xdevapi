--TEST--
mysqlx incorrect TLS versions
--SKIPIF--
--FILE--
<?php

require(__DIR__."/tls_utils.inc");

test_tls_connection('tls-version=[TLSv1.0,TLSv1.1]', false);
test_tls_connection('tls-version=[foo,TLSv1.1]', false);
test_tls_connection('tls-version=[foo,bar]', false);
test_tls_connection('tls-version=[]', false);
test_tls_connection('tls-version=', false);

global $disable_ssl_opt;
test_tls_connection('tls-versions=[TLSv1,TLSv1.1,TLSv1.2]&'.$disable_ssl_opt, true);
test_tls_connection($disable_ssl_opt.'&tls-versions=[TLSv1,TLSv1.2]', true);

test_tls_connection('tls-version=TLSv1.2&'.$disable_ssl_opt, true);
test_tls_connection($disable_ssl_opt.'&tls-version=[TLSv1.2,TLSv1.1,TLSv1.0]', true);
test_tls_connection($disable_ssl_opt.'&tls-version=[TLSv1.2,TLSv1]', true);

verify_expectations();
print "done!\n";
?>
--CLEAN--
<?php
require(__DIR__."/tls_utils.inc");
clean_test_db();
?>
--EXPECTF--
[10065][HY000] Unknown TLS version: No valid TLS version specified, the only valid versions are TLSv1.2%a
[10065][HY000] Unknown TLS version: No valid TLS version specified, the only valid versions are TLSv1.2%a
[10065][HY000] Unknown TLS version: No valid TLS version specified, the only valid versions are TLSv1.2%a
[10065][HY000] Unknown TLS version: No valid TLS version specified, the only valid versions are TLSv1.2%a
[10052][HY000] Invalid argument. The argument to tls-version cannot be empty.

Warning: mysql_xdevapi\getSession(): TLSv1 and TLSv1.1 are not supported starting from MySQL 8.0.28 and should not be used.%a

Warning: mysql_xdevapi\getSession(): TLSv1 and TLSv1.1 are not supported starting from MySQL 8.0.28 and should not be used.%a

Warning: mysql_xdevapi\getSession(): TLSv1 and TLSv1.1 are not supported starting from MySQL 8.0.28 and should not be used.%a

Warning: mysql_xdevapi\getSession(): TLSv1 and TLSv1.1 are not supported starting from MySQL 8.0.28 and should not be used.%a
done!%A

