--TEST--
mysqlx incorrect TLS versions
--SKIPIF--
--FILE--
<?php

require(__DIR__."/tls_utils.inc");

test_tls_connection('tls-versions=TLSv1.x', false);
test_tls_connection('tls-versions=TLS1.0', false);
test_tls_connection('tls-versions=tlsv0.1', false);
test_tls_connection('tls-versions=TLSv12', false);
test_tls_connection('tls-versions=incorrect', false);

test_tls_connection('tls-versions=[TLSv1.2,TLSv1.1,TLSv1', false);
test_tls_connection('tls-versions=TLSv1,TLSv1.2]', false);
test_tls_connection('tls-versions=[TLSx1.0,TLSv1.0,TLSv1]', false);
test_tls_connection('tls-versions=[]', false);

test_tls_connection('tls-version=[TLSv]', false);
test_tls_connection('tls-version=[TLSv1.01]', false);
test_tls_connection('tls-version=[TLSv11]', false);
test_tls_connection('tls-version=[foo]', false);
test_tls_connection('tls-version=', false);

test_tls_connection('tls-version=[ ]', false);

global $disable_ssl_opt;
//test_tls_connection('tls-versions=TLSv1&'.$disable_ssl_opt, false);
test_tls_connection('tls-versions=[TLSv1,TLSv1.1,TLSv1.2]&'.$disable_ssl_opt, false);
//test_tls_connection($disable_ssl_opt.'&tls-versions=[TLSv1.1]', false);
test_tls_connection($disable_ssl_opt.'&tls-versions=[TLSv1,TLSv1.2]', false);

test_tls_connection('tls-version=TLSv1.2&'.$disable_ssl_opt, false);
//test_tls_connection('tls-version=[TLSv1.0,TLSv1.1]&'.$disable_ssl_opt, false);
test_tls_connection($disable_ssl_opt.'&tls-version=[TLSv1.2,TLSv1.1,TLSv1.0]', false);
test_tls_connection($disable_ssl_opt.'&tls-version=[TLSv1.2,TLSv1]', false);

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
[10065][HY000] Unknown TLS version: No valid TLS version specified, the only valid versions are TLSv1.2%a
[10065][HY000] Unknown TLS version: No valid TLS version specified, the only valid versions are TLSv1.2%a
[10065][HY000] Unknown TLS version: No valid TLS version specified, the only valid versions are TLSv1.2%a
[10065][HY000] Unknown TLS version: No valid TLS version specified, the only valid versions are TLSv1.2%a
[10065][HY000] Unknown TLS version: No valid TLS version specified, the only valid versions are TLSv1.2%a
[10065][HY000] Unknown TLS version: No valid TLS version specified, the only valid versions are TLSv1.2%a
[10065][HY000] Unknown TLS version: No valid TLS version specified, the only valid versions are TLSv1.2%a
[10065][HY000] Unknown TLS version: No valid TLS version specified, the only valid versions are TLSv1.2%a
[10065][HY000] Unknown TLS version: No valid TLS version specified, the only valid versions are TLSv1.2%a
[10052][HY000] Invalid argument. The argument to tls-version cannot be empty.
[10065][HY000] Unknown TLS version: No valid TLS version specified, the only valid versions are TLSv1.2%a

Warning: mysql_xdevapi\getSession(): TLSv1 and TLSv1.1 are not supported starting from MySQL 8.0.28 and should not be used.%a
[10045][HY000] Inconsistent ssl options cannot disable SSL connections when secure options are used

Warning: mysql_xdevapi\getSession(): TLSv1 and TLSv1.1 are not supported starting from MySQL 8.0.28 and should not be used.%a
[10045][HY000] Inconsistent ssl options secure option 'tls-versions' can not be specified when SSL connections are disabled
[10045][HY000] Inconsistent ssl options cannot disable SSL connections when secure options are used

Warning: mysql_xdevapi\getSession(): TLSv1 and TLSv1.1 are not supported starting from MySQL 8.0.28 and should not be used.%a
[10045][HY000] Inconsistent ssl options secure option 'tls-version' can not be specified when SSL connections are disabled

Warning: mysql_xdevapi\getSession(): TLSv1 and TLSv1.1 are not supported starting from MySQL 8.0.28 and should not be used.%a
[10045][HY000] Inconsistent ssl options secure option 'tls-version' can not be specified when SSL connections are disabled
done!%A
