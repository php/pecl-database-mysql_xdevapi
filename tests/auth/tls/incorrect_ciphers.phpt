--TEST--
mysqlx incorrect ciphers
--SKIPIF--
--FILE--
<?php

require(__DIR__."/tls_utils.inc");

test_tls_connection('ssl-ciphers=INCORRECT-CIPHER', false);
test_tls_connection('ssl-ciphers=DHE-RSA-AES127-GCM-SHA259', false);
test_tls_connection('ssl-ciphers=foo', false);
test_tls_connection('ssl-ciphers=[WRONG-CIPHER,UNKNOWN-CIPHER]', false);
test_tls_connection('ssl-ciphers=[ADH-AES257-SHAA,AES256-SHHA,EDDH-DSS-CBC-SHA259]', false);

test_tls_connection('ssl-ciphers=[DES-CBC-SHA]&tls-version=TLSv1.2', false);
test_tls_connection('tls-versions=[TLSv1.1,TLSv1.2]&ssl-ciphers=[DHE-DSS-RC4-SHA]', false);

test_tls_connection('ssl-cipher=bad-cipher', false);
test_tls_connection('ssl-cipher=ECDH-RSAA-RCC4-SHA', false);
test_tls_connection('ssl-cipher=FOO', false);
test_tls_connection('ssl-cipher=[FALSE-CIPHER,NON-EXISTING-CIPHER]', false);
test_tls_connection('ssl-ciphers=[AES129-SHA256,ECDH-RSA-AES129-SHAA256,ADH-AES253-GCM-SHA385]', false);

test_tls_connection('ssl-cipher=[EDH-RSA-DES-CBC-SHA]&tls-version=[TLSv1,TLSv1.2]', false);
test_tls_connection('tls-versions=TLSv1.2&ssl-cipher=[EXP-EDH-RSA-DES-CBC-SHA]', false);

global $disable_ssl_opt;
test_tls_connection('ssl-ciphers=DHE-RSA-AES128-GCM-SHA256&'.$disable_ssl_opt, true);
test_tls_connection('ssl-ciphers=[ECDHE-RSA-AES128-SHA256,AES128-SHA256]&'.$disable_ssl_opt, true);
test_tls_connection($disable_ssl_opt.'&ssl-ciphers=[ECDH-RSA-AES128-SHA256]', true);
test_tls_connection($disable_ssl_opt.'&ssl-ciphers=[ADH-AES256-SHA256,ECDHE-RSA-AES128-SHA256]', true);

test_tls_connection('ssl-cipher=ADH-AES128-GCM-SHA256&'.$disable_ssl_opt, true);
test_tls_connection('ssl-cipher=[DHE-RSA-AES128-SHA256,ECDHE-ECDSA-AES128-GCM-SHA256]&'.$disable_ssl_opt, true);
test_tls_connection($disable_ssl_opt.'&ssl-cipher=[AES128-GCM-SHA256,ECDH-RSA-AES128-SHA,DHE-RSA-SEED-SHA]', true);
test_tls_connection($disable_ssl_opt.'&ssl-cipher=DH-DSS-AES128-SHA256', true);

verify_expectations();
print "done!\n";
?>
--CLEAN--
<?php
require(__DIR__."/tls_utils.inc");
clean_test_db();
?>
--EXPECTF--
[10070][HY000] No valid cipher found in the ssl ciphers list.
[10070][HY000] No valid cipher found in the ssl ciphers list.
[10070][HY000] No valid cipher found in the ssl ciphers list.
[10070][HY000] No valid cipher found in the ssl ciphers list.
[10070][HY000] No valid cipher found in the ssl ciphers list.
[10070][HY000] No valid cipher found in the ssl ciphers list.

Warning: mysql_xdevapi\getSession(): TLSv1 and TLSv1.1 are not supported starting from MySQL 8.0.28 and should not be used.%a
[10070][HY000] No valid cipher found in the ssl ciphers list.
[10070][HY000] No valid cipher found in the ssl ciphers list.
[10070][HY000] No valid cipher found in the ssl ciphers list.
[10070][HY000] No valid cipher found in the ssl ciphers list.
[10070][HY000] No valid cipher found in the ssl ciphers list.
[10070][HY000] No valid cipher found in the ssl ciphers list.

Warning: mysql_xdevapi\getSession(): TLSv1 and TLSv1.1 are not supported starting from MySQL 8.0.28 and should not be used.%a
[10070][HY000] No valid cipher found in the ssl ciphers list.
[10070][HY000] No valid cipher found in the ssl ciphers list.
done!%A
