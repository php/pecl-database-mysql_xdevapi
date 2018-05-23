--TEST--
mysqlx create/drop index - extra cases
--SKIPIF--
--FILE--
<?php
require_once(__DIR__."/../connect.inc");
require_once(__DIR__."/index_utils.inc");

$session = create_test_db();
$schema = $session->getSchema($db);
$coll = $schema->getCollection($test_collection_name);

// ET_1 Create an index with mismatched data types
// Create the index with datetime data type on a number "field":
expect_create_index_with_name("myIntField", '{"fields": [{"field": "$.myField", "type": "DATETIME"}]}');

// ET_2 Create an index specifiying SPATIAL as the index type for a non spatial data type
// Create the index with text data type and geojson "options":
// Error should be thrown
expect_fail_index('{"fields": [{"field": "$.myField", "type": "TEXT(13)"}], "type":"SPATIAL"}');

// ET_3 Create an index specifiying INDEX as the index type for a spatial data type
// Create the index with text data type and geojson "options":
// Error should be thrown
//FILIP: expect_fail_index('{"fields": [{"field": "$.myField", "type": "GEOJSON", "required": true, "options": 2, "srid": 4326}], "type":"INDEX"}');

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
