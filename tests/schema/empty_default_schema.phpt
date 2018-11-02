--TEST--
mysqlx empty default schema
--SKIPIF--
--INI--
error_reporting=E_ALL
--FILE--
<?php
require_once("schema_utils.inc");

$default_schema = "";
$uri = prepare_uri_with_schema($default_schema);
$session = mysql_xdevapi\getSession($uri);
try {
	verify_default_schema($session, 0);
	test_step_failed('failure expected for '.$uri);
} catch(Exception $e) {
	test_step_ok();
}

verify_expectations();
print "done!\n";
?>
--CLEAN--
<?php
require_once("schema_utils.inc");
clean_test_db();
?>
--EXPECTF--
done!%A
