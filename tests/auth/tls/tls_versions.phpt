--TEST--
mysqlx TLS versions
--SKIPIF--
--FILE--
<?php

require(__DIR__."/tls_utils.inc");

global $disable_ssl_opt;

test_tls_connection('tls-versions=TLSv1.2', true);

test_tls_connection('tls-versions=[TLSv1,TLSv1.1,TLSv1.2]', true);
test_tls_connection('tls-versions=[TLSv1.2,TLSv1.1,TLSv1]', true);
test_tls_connection('tls-versions=[TLSv1,TLSv1.2]', true);

test_tls_connection('tls-version=[TLSv1.2]', true);

test_tls_connection('tls-version=[TLSv1.1,TLSv1.2]', true);
test_tls_connection('tls-version=[TLSv1.2,TLSv1.1,TLSv1]', true);
test_tls_connection('tls-version=[TLSv1.2,TLSv1,TLSv1.2]', true);
test_tls_connection('tls-version=[TLSv1.0,TLSv1.1,TLSv1.0,TLSv1.2,TLSv1.1]', true);

verify_expectations();
print "done!\n";
?>
--CLEAN--
<?php
require(__DIR__."/tls_utils.inc");
clean_test_db();
?>
--EXPECTF--
done!%A
