--TEST--
mysqlx TLS versions for TLSv1.3
--SKIPIF--
<?php
require(__DIR__."/tls_utils.inc");
skip_if_tls_v13_not_supported();
?>
--FILE--
<?php

require(__DIR__."/tls_utils.inc");

test_tls_connection('tls-versions=TLSv1.3', true);
test_tls_connection('tls-versions=[TLSv1,TLSv1.2,TLSv1.3]', true);
test_tls_connection('tls-version=[TLSv1.3]', true);
test_tls_connection('tls-version=[TLSv1.3,TLSv1.1,TLSv1.3,TLSv1.1]', true);

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

Warning: mysql_xdevapi\getSession(): TLSv1 and TLSv1.1 are not supported starting from MySQL 8.0.28 and should not be used.%a
done!%A
