--TEST--
mysqlx TLS ciphersuites
--SKIPIF--
--FILE--
<?php

require_once(__DIR__."/tls_utils.inc");

test_tls_connection('tls-versions=TLSv1&tls-ciphersuites=TLS_RSA_WITH_AES_128_CBC_SHA', true);
test_tls_connection('tls-versions=TLSv1.2&tls-ciphersuites=[TLS_RSA_WITH_AES_128_GCM_SHA256]', true);
test_tls_connection('tls-versions=TLSv1.2&tls-ciphersuites=[]', true);
test_tls_connection(
	'tls-ciphersuites=[TLS_RSA_WITH_AES_128_CBC_SHA,TLS_DH_DSS_WITH_CAMELLIA_128_CBC_SHA]'
	. '&tls-versions=[TLSv1.0,TLSv1.2]'
	, true);
test_tls_connection(
	'tls-ciphersuites=[TLS_RSA_WITH_DES_CBC_SHA,TLS_DHE_RSA_WITH_AES_256_CBC_SHA,TLS_DHE_DSS_WITH_SEED_CBC_SHA]'
	. '&tls-versions=TLSv1.1'
	, true);

test_tls_connection('tls-version=TLSv1.0&tls-ciphersuites=TLS_DHE_RSA_WITH_AES_128_CBC_SHA', true);
test_tls_connection('tls-version=TLSv1.2&tls-ciphersuites=[TLS_DHE_RSA_WITH_AES_128_CBC_SHA256]', true);
test_tls_connection('tls-version=TLSv1.1&tls-ciphersuites=[]', true);
test_tls_connection(
	'tls-ciphersuites=[TLS_DHE_RSA_WITH_AES_128_CBC_SHA,TLS_DHE_DSS_WITH_AES_128_CBC_SHA256]'
	. '&tls-version=[TLSv1.0,TLSv1.2]'
	, true);
test_tls_connection(
	'tls-ciphersuites=[TLS_DHE_DSS_WITH_AES_256_CBC_SHA256,TLS_RSA_WITH_AES_128_CBC_SHA256,TLS_ECDHE_ECDSA_WITH_AES_256_GCM_SHA384]'
	. '&tls-version=[TLSv1.2]'
	, true);


if (are_tls_ciphersuites_supported()) {
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
}

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
