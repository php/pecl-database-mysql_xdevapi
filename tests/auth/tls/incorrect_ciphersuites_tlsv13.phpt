--TEST--
mysqlx incorrect ciphersuites for TLSv1.3
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
	'tls-versions=TLSv1.3&tls-ciphersuites=TLS_DH_anon_WITH_3DES_EDE_CBC_SHA'
	, false);
test_tls_connection(
	'tls-versions=[TLSv1.3]&tls-ciphersuites=[TLS_DH_DSS_WITH_CAMELLIA_128_CBC_SHA]'
	, false);
test_tls_connection(
	'tls-version=TLSv1.3&'.
	'tls-ciphersuites=[SSL_DH_DSS_WITH_3DES_EDE_CBC_SHA,TLS_RSA_WITH_RC4_128_SHA,TLS_DH_anon_WITH_DES_CBC_SHA]'
	, false);

verify_expectations();
print "done!\n";
?>
--CLEAN--
<?php
require_once(__DIR__."/tls_utils.inc");
clean_test_db();
?>
--EXPECTF--
[10071][HY000] No valid cipher suite found in the tls ciphersuites list.
[10071][HY000] No valid cipher suite found in the tls ciphersuites list.
[10071][HY000] No valid cipher suite found in the tls ciphersuites list.
done!%A
