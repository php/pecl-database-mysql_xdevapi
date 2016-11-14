--TEST--
mysqlx connection (success/fail)
--SKIPIF--
--INI--
error_reporting=0
--FILE--
<?php
	require_once("connect.inc");

	$test = "10000";
	$nodeSession = false;

	try {
		$nodeSession = mysql_xdevapi\getNodeSession($host, $user, $passwd);
	} catch(Exception $e) {
		test_step_failed();
	}

	if (false == $nodeSession)
		test_step_failed();

	try {
		$nodeSession = mysql_xdevapi\getNodeSession("bad_host", $user, $passwd);
		test_step_failed();
	} catch(Exception $e) {
		expect_eq($e->getMessage(),
			'[HY000] php_network_getaddresses: getaddrinfo failed: Name or service not known');
		expect_eq($e->getCode(), 2002);
		test_step_ok();
	}

	try {
		$nodeSession = mysql_xdevapi\getNodeSession($host, "bad_user", $passwd);
	} catch(Exception $e) {
		expect_eq($e->getMessage(),
			'[HY000] Invalid user or password');
		expect_eq($e->getCode(), 1045);
		test_step_ok();
	}

	try {
		$nodeSession = mysql_xdevapi\getNodeSession($host, $user, "some_password");
	} catch(Exception $e) {
		expect_eq($e->getMessage(),
			'[HY000] Invalid user or password');
		expect_eq($e->getCode(), 1045);
		test_step_ok();
	}

	$nodeSession = mysql_xdevapi\getNodeSession("", $user, $passwd);
	expect_false($nodeSession);

	verify_expectations();
	print "done!\n";
?>
--EXPECTF--
done!%A
