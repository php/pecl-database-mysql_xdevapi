--TEST--
mysqlx incorrect TLS versions
--SKIPIF--
--FILE--
<?php

require(__DIR__."/tls_utils.inc");

test_tls_connection('tls-versions=TLSv1.x', false);
test_tls_connection('tls-versions=TLS1.0', false);
test_tls_connection('tls-versions=tlsv0.1', false);
test_tls_connection('tls-versions=TLSv12', false);
test_tls_connection('tls-versions=incorrect', false);

test_tls_connection('tls-versions=[TLSv1;TLSv1.0,TLSv1.2]', false);
test_tls_connection('tls-versions=[TLSv1.2,TLSv1.1,TLSv1', false);
test_tls_connection('tls-versions=TLSv1,TLSv1.2]', false);
test_tls_connection('tls-versions=[TLSx1.0,TLSv1.0,TLSv1]', false);
test_tls_connection('tls-versions=[TLSv1,TLSv1.1,TLSv1.2,]', false);
test_tls_connection('tls-versions=[]', false);

test_tls_connection('tls-version=[TLSv]', false);
test_tls_connection('tls-version=[TLSv1.01]', false);
test_tls_connection('tls-version=[TLSv11]', false);
test_tls_connection('tls-version=[foo]', false);
test_tls_connection('tls-version=', false);

test_tls_connection('tls-version=[TLSv1.1,TLSv1.2,unknown]', false);
test_tls_connection('tls-version=[TLSv1.2,xyz,TLSv1.1,TLSv1]', false);
test_tls_connection('tls-version=[TLSv1.2,TLS,TLSv1.2]', false);
test_tls_connection('tls-version=[ ]', false);

global $disable_ssl_opt;
test_tls_connection('tls-versions=TLSv1&'.$disable_ssl_opt, false);
test_tls_connection('tls-versions=[TLSv1,TLSv1.1,TLSv1.2]&'.$disable_ssl_opt, false);
test_tls_connection($disable_ssl_opt.'&tls-versions=[TLSv1.1]', false);
test_tls_connection($disable_ssl_opt.'&tls-versions=[TLSv1,TLSv1.2]', false);

test_tls_connection('tls-version=TLSv1.2&'.$disable_ssl_opt, false);
test_tls_connection('tls-version=[TLSv1.0,TLSv1.1]&'.$disable_ssl_opt, false);
test_tls_connection($disable_ssl_opt.'&tls-version=[TLSv1.2,TLSv1.1,TLSv1.0]', false);
test_tls_connection($disable_ssl_opt.'&tls-version=[TLSv1.2,TLSv1]', false);

verify_expectations();
print "done!\n";
?>
--CLEAN--
<?php
require(__DIR__."/tls_utils.inc");
clean_test_db();
?>
--EXPECTF--
[10065][HY000] Unknown TLS version: TLSv1.x not recognized as a valid TLS protocol version (should be one of TLS%a)
[10065][HY000] Unknown TLS version: TLS1.0 not recognized as a valid TLS protocol version (should be one of TLS%a)
[10065][HY000] Unknown TLS version: tlsv0.1 not recognized as a valid TLS protocol version (should be one of TLS%a)
[10065][HY000] Unknown TLS version: TLSv12 not recognized as a valid TLS protocol version (should be one of TLS%a)
[10065][HY000] Unknown TLS version: incorrect not recognized as a valid TLS protocol version (should be one of TLS%a)
[10065][HY000] Unknown TLS version: TLSv1;TLSv1.0 not recognized as a valid TLS protocol version (should be one of TLS%a)
[10065][HY000] Unknown TLS version: [TLSv1.2,TLSv1.1,TLSv1 not recognized as a valid TLS protocol version (should be one of TLS%a)
[10065][HY000] Unknown TLS version: TLSv1,TLSv1.2] not recognized as a valid TLS protocol version (should be one of TLS%a)
[10065][HY000] Unknown TLS version: TLSx1.0 not recognized as a valid TLS protocol version (should be one of TLS%a)
[10065][HY000] Unknown TLS version: '' not recognized as a valid TLS protocol version (should be one of TLS%a)
[10067][HY000] at least one TLS protocol version must be specified in tls-versions list
[10065][HY000] Unknown TLS version: TLSv not recognized as a valid TLS protocol version (should be one of TLS%a)
[10065][HY000] Unknown TLS version: TLSv1.01 not recognized as a valid TLS protocol version (should be one of TLS%a)
[10065][HY000] Unknown TLS version: TLSv11 not recognized as a valid TLS protocol version (should be one of TLS%a)
[10065][HY000] Unknown TLS version: foo not recognized as a valid TLS protocol version (should be one of TLS%a)
[10052][HY000] Invalid argument. The argument to tls-version cannot be empty.
[10065][HY000] Unknown TLS version: unknown not recognized as a valid TLS protocol version (should be one of TLS%a)
[10065][HY000] Unknown TLS version: xyz not recognized as a valid TLS protocol version (should be one of TLS%a)
[10065][HY000] Unknown TLS version: TLS not recognized as a valid TLS protocol version (should be one of TLS%a)
[10067][HY000] at least one TLS protocol version must be specified in tls-versions list
[10045][HY000] Inconsistent ssl options cannot disable SSL connections when secure options are used
[10045][HY000] Inconsistent ssl options cannot disable SSL connections when secure options are used
[10045][HY000] Inconsistent ssl options secure option 'tls-versions' can not be specified when SSL connections are disabled
[10045][HY000] Inconsistent ssl options secure option 'tls-versions' can not be specified when SSL connections are disabled
[10045][HY000] Inconsistent ssl options cannot disable SSL connections when secure options are used
[10045][HY000] Inconsistent ssl options cannot disable SSL connections when secure options are used
[10045][HY000] Inconsistent ssl options secure option 'tls-version' can not be specified when SSL connections are disabled
[10045][HY000] Inconsistent ssl options secure option 'tls-version' can not be specified when SSL connections are disabled
done!%A
