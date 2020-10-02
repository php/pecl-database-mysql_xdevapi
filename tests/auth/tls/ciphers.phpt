--TEST--
mysqlx TLS ciphers
--SKIPIF--
--FILE--
<?php

require(__DIR__."/tls_utils.inc");

test_tls_connection('ssl-ciphers=AES256-SHA256', true);
test_tls_connection('ssl-ciphers=[]', true);
test_tls_connection('ssl-ciphers=[AES128-GCM-SHA256]', true);
test_tls_connection('ssl-ciphers=[ECDH-RSA-AES128-SHA256,AES128-GCM-SHA256]', true);
test_tls_connection('ssl-ciphers=[ECDHE-RSA-AES128-SHA256,AES256-SHA,ECDH-ECDSA-AES128-SHA256]', true);

test_tls_connection('ssl-ciphers=[AES256-GCM-SHA384]&tls-versions=TLSv1.2', true);
test_tls_connection('tls-versions=TLSv1.2&ssl-ciphers=[DHE-RSA-AES256-SHA]', true);

test_tls_connection('ssl-cipher=DHE-RSA-AES256-SHA', true);
test_tls_connection('ssl-cipher=[]', true);
test_tls_connection('ssl-cipher=[DHE-RSA-AES128-GCM-SHA256]', true);
test_tls_connection('ssl-cipher=[AES256-SHA,DHE-RSA-AES128-GCM-SHA256]', true);
test_tls_connection('ssl-cipher=[ECDHE-RSA-AES256-SHA,DHE-RSA-AES256-SHA256,PSK-AES128-CBC-SHA]', true);

test_tls_connection('ssl-cipher=[DHE-RSA-AES128-SHA256]&tls-versions=TLSv1.2', true);
test_tls_connection('tls-versions=TLSv1.2&ssl-cipher=[DHE-RSA-AES256-SHA]', true);

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
