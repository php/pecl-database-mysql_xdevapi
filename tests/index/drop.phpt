--TEST--
mysqlx drop index
--SKIPIF--
--INI--
error_reporting=0
--FILE--
<?php
require_once(__DIR__."/../connect.inc");
require_once(__DIR__."/index_utils.inc");

$session = create_test_db();
$schema = $session->getSchema($db);
$coll = $schema->getCollection($test_collection_name);

drop_index(false);

create_index('{"fields": [{"field": "$.myField", "type": "TEXT(13)"}]}');

drop_index(true);
drop_index(false);
drop_index(false);

create_index('{"fields": [{"field": "$.myField", "type": "TEXT(13)"}, {"field": "$.myField2", "type": "TEXT(10)", "required": true}, {"field": "$.myField3", "type": "INT UNSIGNED", "required": false}], "unique": true}');

drop_index(true);
drop_index(false);
drop_index(false);

verify_expectations();
print "done!\n";
?>
--CLEAN--
<?php
	require_once(__DIR__."/../connect.inc");
	clean_test_db();
?>
--EXPECTF--
done!%A
