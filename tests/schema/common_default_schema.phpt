--TEST--
mysqlx common test for default schema in uri
--SKIPIF--
--INI--
error_reporting=E_ALL
--FILE--
<?php
require_once("schema_utils.inc");

$default_schema = "";
$uri = prepare_uri_with_schema($default_schema);
$session = mysql_xdevapi\getSession($uri);

$default_schema = $test_schema_name;
$schema = create_schema($session, $default_schema);
create_table($session, $schema, $test_table_name);

$uri = prepare_uri_with_schema($default_schema);
$session = mysql_xdevapi\getSession($uri);
verify_default_schema($session, 1);

create_table($session, $schema, $test_table_name.'2');
verify_default_schema($session, 2);

create_table($session, $schema, $test_table_name.'3');
verify_default_schema($session, 3);

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
