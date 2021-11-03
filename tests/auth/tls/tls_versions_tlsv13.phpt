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

$tls_uri = prepare_tls_uri('');
$session = mysql_xdevapi\getSession($tls_uri);
expect_not_null($session);

$result = $session->sql("SELECT @@tls_version")->execute();
$data = $result->fetchOne();
file_put_contents('temp', $data['@@tls_version']);

$session->sql("SET GLOBAL tls_version = 'TLSv1.2,TLSv1.3'")->execute();

test_version('', true, 'TLSv1.3');
test_version('tls-versions=[TLSv1.1,TLSv1.2]', true, 'TLSv1.2');
test_version('tls-version=[foo,TLSv1.3]', true, 'TLSv1.3');
test_version('tls-versions=[TLSv1.1,TLSv1.3]', true, 'TLSv1.3');

$session->sql("SET GLOBAL tls_version = 'TLSv1.2'")->execute();

test_version('', true, 'TLSv1.2');
test_version('tls-version=[foo,TLSv1.3]', false, '');
test_version('tls-versions=[TLSv1.1,TLSv1.3]', false, '');

verify_expectations();
print "done!\n";
?>
--CLEAN--
<?php
require(__DIR__."/tls_utils.inc");
clean_test_db();
$tls_uri = prepare_tls_uri('');
$session = mysql_xdevapi\getSession($tls_uri);
if($session) {
	$version = file_get_contents('temp');
	if(!empty($version)) {
		$session->sql("SET GLOBAL tls_version = '" . $version . "'")->execute();
	}
	unlink('temp');
}
?>
--EXPECTF--

Warning: mysql_xdevapi\getSession(): TLSv1 and TLSv1.1 are not supported starting from MySQL 8.0.28 and should not be used.%a

Warning: mysql_xdevapi\getSession(): TLSv1 and TLSv1.1 are not supported starting from MySQL 8.0.28 and should not be used.%a
done!%A
