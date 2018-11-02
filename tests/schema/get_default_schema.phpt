--TEST--
mysqlx get default schema
--SKIPIF--
--INI--
error_reporting=E_ALL
--FILE--
<?php
require_once("schema_utils.inc");
clean_test_db();

$default_schema_name = "";
$uri = prepare_uri_with_schema($default_schema_name);
$session = mysql_xdevapi\getSession($uri);

$default_schema_name = $test_schema_name;
create_schema($session, $default_schema_name);

$uri = prepare_uri_with_schema($default_schema_name);
$session = mysql_xdevapi\getSession($uri);
$schema = $session->getDefaultSchema();
expect_eq($schema->getName(), $default_schema_name);

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
