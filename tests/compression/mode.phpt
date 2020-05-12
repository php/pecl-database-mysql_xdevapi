--TEST--
mysqlx compression mode
--SKIPIF--
--FILE--
<?php
require("compression_utils.inc");

// ---------------------------------------
// correct options lower-case

try {
	$session = mysql_xdevapi\getSession($base_uri.'/?compression=preferred');
	test_step_ok();
} catch(Exception $e) {
	test_step_failed();
}

try {
	$session = mysql_xdevapi\getSession($base_uri.'/?compression=disabled');
	test_step_ok();
} catch(Exception $e) {
	test_step_failed();
}

try {
	$session = mysql_xdevapi\getSession($base_uri.'/?compression=required');
	test_step_ok();
} catch(Exception $e) {
	if ($e->getMessage() == Compression_disabled_msg) {
		test_step_ok();
	} else {
		test_step_failed();
	}
}

// ---------------------------------------
// correct options various-case (allowed)

try {
	$session = mysql_xdevapi\getSession($base_uri.'/?compression=PREFERRED');
	test_step_ok();
} catch(Exception $e) {
	test_step_failed();
}

try {
	$session = mysql_xdevapi\getSession($base_uri.'/?compression=DiSaBlEd');
	test_step_ok();
} catch(Exception $e) {
	test_step_failed();
}

try {
	$session = mysql_xdevapi\getSession($base_uri.'/?compression=Required');
	test_step_ok();
} catch(Exception $e) {
	if ($e->getMessage() == Compression_disabled_msg) {
		test_step_ok();
	} else {
		test_step_failed();
	}
}

// ---------------------------------------
// incorrect options

try {
	$session = mysql_xdevapi\getSession($base_uri.'/?compression=On');
	test_step_failed();
} catch(Exception $e) {
	test_step_ok();
}

try {
	$session = mysql_xdevapi\getSession($base_uri.'/?compression=OFF');
	test_step_failed();
} catch(Exception $e) {
	test_step_ok();
}

try {
	$session = mysql_xdevapi\getSession($base_uri.'/?compression=True,other=5');
	test_step_failed();
} catch(Exception $e) {
	test_step_ok();
}

try {
	$session = mysql_xdevapi\getSession($base_uri.'/?compression=FalSe,param=AbCdE');
	test_step_failed();
} catch(Exception $e) {
	test_step_ok();
}

verify_expectations();
print "done!\n";
?>
--CLEAN--
<?php
require("connect.inc");
clean_test_db();
?>
--EXPECTF--
done!%A
