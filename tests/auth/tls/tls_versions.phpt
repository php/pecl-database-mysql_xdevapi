--TEST--
mysqlx TLS versions
--SKIPIF--
<?php
require(__DIR__."/tls_utils.inc");
if (!is_tls_v13_supported()) return;
	die('skip TLSv1.3 is supported');
?>
--FILE--
<?php

require(__DIR__."/tls_utils.inc");

global $disable_ssl_opt;

test_tls_version('', true, 'TLSv1.2');
test_tls_version('tls-versions=[TLSv1.1,TLSv1.2]', true, 'TLSv1.2');
test_tls_version('tls-version=[foo,TLSv1.3]', false, '');
test_tls_version('tls-versions=[TLSv1.1,TLSv1.3]', false, '');

verify_expectations();
print "done!\n";
?>
--CLEAN--
<?php
require(__DIR__."/tls_utils.inc");
clean_test_db();
?>
--EXPECTF--

Warning: mysql_xdevapi\getSession(): TLSv1 and TLSv1.1 are not supported starting from MySQL 8.0.28 and should not be used.%a
[10065][HY000] Unknown TLS version: No valid TLS version specified, the only valid versions are%a
[10065][HY000] Unknown TLS version: No valid TLS version specified, the only valid versions are%a
done!%A
