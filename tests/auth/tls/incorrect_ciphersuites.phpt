--TEST--
mysqlx incorrect TLS ciphersuites
--SKIPIF--
--FILE--
<?php

require_once(__DIR__."/tls_utils.inc");

test_tls_connection('tls-versions=TLSv1&tls-ciphersuites=TLS_ECDH_ECDSA_WITH_AES_128_GCM_SHA256', false);
test_tls_connection('tls-versions=[TLSv1.2]&tls-ciphersuites=[TLS_DH_anon_WITH_AES_128_CBC_SHA]', false);
test_tls_connection(
	'tls-ciphersuites=[TLS_DH_DSS_WITH_CAMELLIA_256_CBC_SHA,TLS_DHE_RSA_WITH_SEED_CBC_SHA]'
	. '&tls-versions=TLSv1.1'
	, false);
test_tls_connection(
	'tls-versions=TLSv1.0&'.
	'tls-ciphersuites=[TLS_DH_DSS_WITH_CAMELLIA_256_CBC_SHA,TLS_DHE_RSA_WITH_SEED_CBC_SHA,TLS_ECDHE_ECDSA_WITH_AES_128_CBC_SHA]'
	, false);

test_tls_connection('tls-version=TLSv1.2&tls-ciphersuites=[TLS_ECDHE_ECDSA_WITH_RC4_128_SHA]', false);
test_tls_connection(
	'tls-version=[TLSv1.0,TLSv1.2]&'.
	'tls-ciphersuites=[TLS_DH_DSS_WITH_AES_256_CBC_SHA256,TLS_ECDHE_RSA_WITH_AES_256_GCM_SHA384]'
	, false);


test_tls_connection('tls-version=TLSv1.0&tls-ciphersuites=TLS_RSA_WITH_NULL_SHA256', false);
test_tls_connection('tls-version=TLSv1.2&tls-ciphersuites=[TLS_RSA_WITH_NULL_MD5]', false);
test_tls_connection(
	'tls-ciphersuites=[SSL_RSA_WITH_NULL_MD5,TLS_DHE_DSS_WITH_AES_128_CBC_SHA256]'
	. '&tls-version=[TLSv1.0,TLSv1.2]'
	, false);
test_tls_connection(
	'tls-ciphersuites=[TLS_RSA_EXPORT_WITH_DES40_CBC_SHA,SSL_DHE_RSA_WITH_DES_CBC_SHA,TLS_DH_DSS_WITH_CAMELLIA_128_CBC_SHA]'
	. '&tls-version=[TLSv1.2]'
	, false);


if (is_tls_v13_supported()) {
	test_tls_connection(
		'tls-versions=TLSv1.3&tls-ciphersuites=TLS_DH_anon_WITH_3DES_EDE_CBC_SHA'
		, false);
	test_tls_connection(
		'tls-versions=[TLSv1.3]&tls-ciphersuites=[TLS_DH_DSS_WITH_CAMELLIA_128_CBC_SHA]'
		, false);
	test_tls_connection(
		'tls-version=TLSv1.3&'.
		'tls-ciphersuites=[SSL_DH_DSS_WITH_3DES_EDE_CBC_SHA,TLS_RSA_WITH_RC4_128_SHA,TLS_DH_anon_WITH_DES_CBC_SHA]'
		, false);
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
Warning: mysql_xdevapi\getSession(): SSL operation failed with code 1. OpenSSL Error messages:
error:140830B5:SSL routines:ssl3_client_hello:no ciphers available in %s
[10068][HY000] Cannot connect to MySQL by using SSL

Warning: mysql_xdevapi\getSession(): SSL operation failed with code 1. OpenSSL Error messages:
error:14094410:SSL routines:ssl3_read_bytes:sslv3 alert handshake failure in %s
[10068][HY000] Cannot connect to MySQL by using SSL

Warning: mysql_xdevapi\getSession(): SSL operation failed with code 1. OpenSSL Error messages:
error:14094410:SSL routines:ssl3_read_bytes:sslv3 alert handshake failure in %s
[10068][HY000] Cannot connect to MySQL by using SSL

Warning: mysql_xdevapi\getSession(): SSL operation failed with code 1. OpenSSL Error messages:
error:14094410:SSL routines:ssl3_read_bytes:sslv3 alert handshake failure in %s
[10068][HY000] Cannot connect to MySQL by using SSL

Warning: mysql_xdevapi\getSession(): SSL operation failed with code 1. OpenSSL Error messages:
error:14094410:SSL routines:ssl3_read_bytes:sslv3 alert handshake failure in %s
[10068][HY000] Cannot connect to MySQL by using SSL

Warning: mysql_xdevapi\getSession(): SSL operation failed with code 1. OpenSSL Error messages:
error:140740B5:SSL routines:SSL23_CLIENT_HELLO:no ciphers available in %s
[10068][HY000] Cannot connect to MySQL by using SSL

Warning: mysql_xdevapi\getSession(): SSL operation failed with code 1. OpenSSL Error messages:
error:140830B5:SSL routines:ssl3_client_hello:no ciphers available in %s
[10068][HY000] Cannot connect to MySQL by using SSL

Warning: mysql_xdevapi\getSession(): SSL operation failed with code 1. OpenSSL Error messages:
error:14094410:SSL routines:ssl3_read_bytes:sslv3 alert handshake failure in %s
[10068][HY000] Cannot connect to MySQL by using SSL

Warning: mysql_xdevapi\getSession(): SSL operation failed with code 1. OpenSSL Error messages:
error:14077410:SSL routines:SSL23_GET_SERVER_HELLO:sslv3 alert handshake failure in %s
[10068][HY000] Cannot connect to MySQL by using SSL

Warning: mysql_xdevapi\getSession(): SSL operation failed with code 1. OpenSSL Error messages:
error:14094410:SSL routines:ssl3_read_bytes:sslv3 alert handshake failure in %s
[10068][HY000] Cannot connect to MySQL by using SSL
done!%A
