--TEST--
mysqlx create/drop index - common cases
--SKIPIF--
--FILE--
<?php
require_once(__DIR__."/../connect.inc");
require_once(__DIR__."/index_utils.inc");

$session = create_test_db();
$schema = $session->getSchema($db);
$coll = $schema->getCollection($test_collection_name);

expect_create_index('{"fields": [{"field": "$.myField", "type": "TEXT(5)", "required": true}], "unique": true}');

expect_create_index_with_name("name_index", '{"fields": [{"field": "$.name", "type": "TEXT(15)", "required": true}], "unique": false}');

expect_create_index_with_name("age_index", '{"unique": false, "fields": [{"field": "$.age", "type": "INTEGER", "required": false}]}');

expect_create_index_with_name("name_age_index", '{"fields": [{"field": "$.name", "type": "TEXT(20)", "required": true}, {"field": "$.age", "type": "INTEGER", "required": false}], "unique": true}');

expect_create_index_with_name("age_job_index", '{"unique": false, "fields": [{"field": "$.age", "type": "INTEGER", "required": true}, {"field": "$.job", "type": "TEXT(30)", "required": false}]}');

expect_create_index_with_name("name_age_job_index", '{"fields": [{"field": "$.name", "type": "TEXT(20)", "required": true}, {"field": "$.age", "type": "INTEGER", "required": true}, {"field": "$.job", "type": "TEXT(30)", "required": false}], "unique": true}');

expect_create_index_with_name("zip", '{"fields": [{"field": "$.zip", "type": "TEXT(10)"}]}');

expect_create_index_with_name("count", '{"fields": [{"field": "$.count", "type": "INT UNSIGNED"}]}');

expect_create_index_with_name("loc", '{"type": "SPATIAL", "fields": [{"field": "$.coords", "type": "GEOJSON", "srid": 31287}]}');

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
