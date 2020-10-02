--TEST--
mysqlx incorrect TLS ciphersuites
--SKIPIF--
--FILE--
<?php

require_once(__DIR__."/tls_utils.inc");

test_tls_connection('tls-versions=TLSv1&tls-ciphersuites=TLS_ECDH_anon_WITH_AES_256_CBC_SHA', false);
test_tls_connection('tls-versions=[TLSv1.2]&tls-ciphersuites=[TLS_DH_anon_WITH_AES_128_CBC_SHA]', false);
test_tls_connection(
	'tls-ciphersuites=[TLS_DH_DSS_EXPORT_WITH_DES40_CBC_SHA,TLS_DHE_DSS_EXPORT_WITH_DES40_CBC_SHA]'
	. '&tls-versions=TLSv1.1'
	, false);

test_tls_connection('tls-version=TLSv1.2&tls-ciphersuites=[TLS_ECDHE_ECDSA_WITH_RC4_128_SHA]', false);


test_tls_connection('tls-version=TLSv1.0&tls-ciphersuites=TLS_RSA_WITH_NULL_SHA256', false);
test_tls_connection('tls-version=TLSv1.2&tls-ciphersuites=[TLS_RSA_WITH_NULL_MD5]', false);
test_tls_connection(
	'tls-ciphersuites=[SSL_RSA_WITH_NULL_MD5,TLS_ECDH_ECDSA_WITH_RC4_128_SHA]'
	. '&tls-version=[TLSv1.0,TLSv1.2]'
	, false);
test_tls_connection(
	'tls-ciphersuites=[TLS_RSA_EXPORT_WITH_DES40_CBC_SHA,SSL_DHE_RSA_WITH_DES_CBC_SHA,TLS_DH_DSS_WITH_CAMELLIA_128_CBC_SHA]'
	. '&tls-version=[TLSv1.2]'
	, false);

global $disable_ssl_opt;
test_tls_connection('tls-ciphersuites=TLS_RSA_WITH_AES_128_GCM_SHA256&'.$disable_ssl_opt, false);
test_tls_connection('tls-ciphersuites=[TLS_RSA_WITH_AES_128_GCM_SHA256,TLS_RSA_WITH_AES_128_CBC_SHA]&'.$disable_ssl_opt, false);
test_tls_connection($disable_ssl_opt.'&tls-ciphersuites=[TLS_ECDH_RSA_WITH_AES_128_CBC_SHA256]', false);
test_tls_connection($disable_ssl_opt.'&tls-ciphersuites=[TLS_DH_anon_WITH_AES_256_CBC_SHA256,TLS_ECDHE_RSA_WITH_AES_128_CBC_SHA256]', false);

test_tls_connection('tls-ciphersuite=TLS_DH_anon_WITH_AES_128_GCM_SHA256&'.$disable_ssl_opt, false);
test_tls_connection('tls-ciphersuite=[TLS_DHE_RSA_WITH_AES_128_CBC_SHA256,TLS_ECDHE_ECDSA_WITH_AES_128_GCM_SHA256]&'.$disable_ssl_opt, false);
test_tls_connection($disable_ssl_opt.'&tls-ciphersuite=[TLS_ECDHE_ECDSA_WITH_AES_128_GCM_SHA256,TLS_ECDH_RSA_WITH_AES_128_CBC_SHA,TLS_DHE_RSA_WITH_SEED_CBC_SHA]', false);
test_tls_connection($disable_ssl_opt.'&tls-ciphersuite=TLS_DH_DSS_WITH_AES_128_CBC_SHA256', false);

verify_expectations();
print "done!\n";
?>
--CLEAN--
<?php
require_once(__DIR__."/tls_utils.inc");
clean_test_db();
?>
--EXPECTF--
%A[10071][HY000] No valid cipher suite found in the tls ciphersuites list.
%A[10071][HY000] No valid cipher suite found in the tls ciphersuites list.
%A[10071][HY000] No valid cipher suite found in the tls ciphersuites list.
%A[10071][HY000] No valid cipher suite found in the tls ciphersuites list.
%A[10071][HY000] No valid cipher suite found in the tls ciphersuites list.
%A[10071][HY000] No valid cipher suite found in the tls ciphersuites list.
%A[10071][HY000] No valid cipher suite found in the tls ciphersuites list.
%A[10071][HY000] No valid cipher suite found in the tls ciphersuites list.
[10045][HY000] Inconsistent ssl options cannot disable SSL connections when secure options are used
[10045][HY000] Inconsistent ssl options cannot disable SSL connections when secure options are used
[10045][HY000] Inconsistent ssl options secure option 'tls-ciphersuites' can not be specified when SSL connections are disabled
[10045][HY000] Inconsistent ssl options secure option 'tls-ciphersuites' can not be specified when SSL connections are disabled
[10045][HY000] Inconsistent ssl options cannot disable SSL connections when secure options are used
[10045][HY000] Inconsistent ssl options cannot disable SSL connections when secure options are used
[10045][HY000] Inconsistent ssl options secure option 'tls-ciphersuite' can not be specified when SSL connections are disabled
[10045][HY000] Inconsistent ssl options secure option 'tls-ciphersuite' can not be specified when SSL connections are disabled
done!%A
