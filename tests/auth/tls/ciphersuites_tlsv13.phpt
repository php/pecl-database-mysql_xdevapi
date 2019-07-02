--TEST--
mysqlx TLS ciphersuites for TLSv1.3
--SKIPIF--
<?php
require(__DIR__."/tls_utils.inc");
skip_if_tls_ciphersuites_not_supported();
skip_if_tls_v13_not_supported();
?>
--FILE--
<?php

require_once(__DIR__."/tls_utils.inc");

test_tls_connection(
	'tls-versions=TLSv1.3&tls-ciphersuites=TLS_AES_128_GCM_SHA256'
	, true);
test_tls_connection(
	'tls-versions=[TLSv1.3]&tls-ciphersuites=[TLS_AES_256_GCM_SHA384]'
	, true);
test_tls_connection(
	'tls-version=TLSv1.3&'.
	'tls-ciphersuites=[TLS_CHACHA20_POLY1305_SHA256,TLS_AES_128_CCM_SHA256,TLS_AES_128_CCM_8_SHA256]'
	, true);

verify_expectations();
print "done!\n";
?>
--CLEAN--
<?php
require_once(__DIR__."/tls_utils.inc");
clean_test_db();
?>
--EXPECTF--
done!%A
