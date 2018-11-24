--TEST--
mysqlx create/drop index - failure cases
--SKIPIF--
--FILE--
<?php
require_once(__DIR__."/../connect.inc");
require_once(__DIR__."/index_utils.inc");

$session = create_test_db();
$schema = $session->getSchema($db);
$coll = $schema->getCollection($test_collection_name);

// FR5_1 Create an Index with an invalid name.
// An exception must be thrown in all the cases
// TODO: it should fail in future version of server
expect_create_index_with_name("01myIndex", '{"fields": [{"field": "$.myField", "type": "TEXT(13)"}]}');
expect_create_index_with_name("!myIndex", '{"fields": [{"field": "$.myField", "type": "TEXT(13)"}]}');
expect_create_index_with_name("-myIndex", '{"fields": [{"field": "$.myField", "type": "TEXT(13)"}]}');

// FR5_2 Create an index with the name of an index that already exists.
try {
	// Create an "index":
	$coll->createIndex("myUniqueIndex", '{"fields": [{"field": "$.myField", "type": "TEXT(13)"}]}');
	// Create an index with the same "name":
	$coll->createIndex("myUniqueIndex", '{"fields": [{"field": "$.myField", "type": "TEXT(13)"}]}');
	// An exception must be thrown
	test_step_failed("myUniqueIndex successfully created");
} catch(Exception $e) {
	test_step_ok();
}

// FR5_3 Create an index with a not valid JSON document definition.
// An exception must be thrown in all the cases
expect_fail_index('{"fields": [{field = "$.myField", type = "TEXT(13)"}]}');
expect_fail_index('{"fields": [{"field": "$.myField", "type": "TEXT(13)"]}');
expect_fail_index('{"fields": [{"field": "$.myField", "type": "TEXT(13)"}}');

// FR5_4 Create an index where its definition is a JSON document but its structure is not valid.
// Create the index with invalid JSON "structure": coll.createIndex
// An exception must be thrown in all the cases
expect_fail_index('{"fields": [{"field": "$.myField", "type": "TEXT(13)", "myCustomField":"myCustomValue"}]}');
expect_fail_index('{"fields": [{"field": "$.myField", "mytype": "TEXT(13)"}]}');
expect_fail_index('{"fields": [{"myfield": "$.myField", "type": "TEXT(13)"}]}');

// FR5_5 Create an index with the index type not "INDEX" or "SPATIAL" (case sensitive).
// Create the index with invalid index "type"
// An exception must be thrown in all the cases
expect_fail_index('{"fields": [{"field": "$.myField", "type": "TEXT(13)"}], "type":"index"}');
expect_fail_index('{"fields": [{"field": "$.myField", "type": "TEXT(13)"}], "type":"Index"}');
expect_fail_index('{"fields": [{"field": "$.myField", "type": "TEXT(13)"}], "type":"InDeX"}');
expect_fail_index('{"fields": [{"field": "$.myGeoJsonField", "type": "GEOJSON", "required": true}], "unique": true, "type":"Spatial"}');
expect_fail_index('{"fields": [{"field": "$.myGeoJsonField", "type": "GEOJSON", "required": true}], "unique": true, "type":"spatial"}');
expect_fail_index('{"fields": [{"field": "$.myGeoJsonField", "type": "GEOJSON", "required": true}], "unique": true, "type":"aPaTiAl"}');

// FR5_6 Create a "SPATIAL" index with "required" flag set to false.
// Create the index with text data type and geojson "options":
// Error should be thrown
expect_fail_index('{"fields": [{"field": "$.myField", "type": "GEOJSON", "required": false, "options": 2, "srid": 4326}], "type":"SPATIAL"}');

// FR5_7 Create an index with an invalid "type" specified (type names are case insensitive).
// Create the index with invalid "type":
// TODO: it should fail in future version of server
expect_create_index('{"fields": [{"field": "$.myField", "type": "TEXT(13)"}]}');
expect_create_index('{"fields": [{"field": "$.myField", "type": "Datetime"}]}');
expect_create_index('{"fields": [{"field": "$.myField", "type": "Timestamp"}]}');
expect_create_index('{"fields": [{"field": "$.myField", "type": "Time"}]}');
expect_create_index('{"fields": [{"field": "$.myField", "type": "Date"}]}');
expect_fail_index('{"fields": [{"field": "$.myField", "type": "incorrect-type"}]}');

// FR5_8 Create an index specifiying geojson options for non geojson data type.
// Create the index with text data type and geojson "options":
expect_fail_index('{"fields": [{"field": "$.myField", "type": "TEXT(13)", "options": 2, "srid": 4326}]}');

// FR5_9 Create an index specifiying the "collation" option for non TEXT data type.
// TODO: it should fail in future version of server
expect_create_index('{"fields": [{"field": "$.myField", "type": "DATETIME", "collation": "utf8_general_ci"}]}');

// creation of index should fail with required field, which is not available in filled collection
fill_db_collection($coll);
expect_fail_index(
	'{"fields":'.
	'[{"field": "$.myField", "type": "TEXT(13)", "required": true},'.
	' {"field": "$.myField2", "type": "INTEGER", "required": false}]}'
);

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
