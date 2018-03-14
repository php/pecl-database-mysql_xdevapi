--TEST--
mysqlx create/drop index - successful cases
--SKIPIF--
--FILE--
<?php
require_once(__DIR__."/../connect.inc");
require_once(__DIR__."/index_utils.inc");

$session = create_test_db();
$schema = $session->getSchema($db);
$coll = $schema->getCollection($test_collection_name);

// FR1_1 Create an index on a single field.
expect_create_index('{"fields": [{"field": "$.myField", "type": "TEXT(13)"}]}');

// FR1_2 Create an index on a single field with all the possibles options.
expect_create_index('{"fields": [{"field": "$.myField", "type": "TEXT(5)", "required": true}], "unique": true}');

// FR1_3 Create an index on multiple fields.
expect_create_index('{"fields": [{"field": "$.myField", "type": "TEXT(13)"}, {"field": "$.myField2", "type": "TEXT(10)"}, {"field": "$.myField3", "type": "INT"}]}');

// FR1_4 Create an index on multiple fields with all the possibles options.
expect_create_index('{"fields": [{"field": "$.myField", "type": "TEXT(13)"}, {"field": "$.myField2", "type": "TEXT(10)", "required": true}, {"field": "$.myField3", "type": "INT UNSIGNED", "required": false}], "unique": true}');

// FR1_5 Create an index using a geojson datatype field.
expect_create_index('{"fields": [{"field": "$.myGeoJsonField", "type": "GEOJSON", "required": true}], "unique": true, "type":"SPATIAL"}');

// FR1_6 Create an index using a geojson datatype field with all the possibles options.
expect_create_index('{"fields": [{"field": "$.myGeoJsonField", "type": "GEOJSON", "required": true, "options": 2, "srid": 4326}], "unique": true, "type":"SPATIAL"}');

// FR1_7 Create an index using a datetime field.
expect_create_index('{"fields": [{"field": "$.myField", "type": "DATETIME"}]}');

// FR1_8 Create an index using a timestamp field.
expect_create_index('{"fields": [{"field": "$.myField", "type": "TIMESTAMP"}]}');

// FR1_9 Create an index using a time field.
expect_create_index('{"fields": [{"field": "$.myField", "type": "TIME"}]}');

// FR1_10 Create an index using a date field.
expect_create_index('{"fields": [{"field": "$.myField", "type": "DATE"}]}');

// FR1_11 Create an index using a numeric field.
expect_create_index('{"fields": [{"field": "$.myField", "type": "NUMERIC UNSIGNED"}]}');

// FR1_12 Create an index using a decimal field.
expect_create_index('{"fields": [{"field": "$.myField", "type": "DECIMAL"}]}');

// FR1_13 Create an index using a double field.
expect_create_index('{"fields": [{"field": "$.myField", "type": "DOUBLE UNSIGNED"}]}');

// FR1_14 Create an index using a float field.
expect_create_index('{"fields": [{"field": "$.myField", "type": "FLOAT UNSIGNED"}]}');

// FR1_15 Create an index using a real field.
expect_create_index('{"fields": [{"field": "$.myField", "type": "REAL UNSIGNED"}]}');

// FR1_16 Create an index using a bigint field.
expect_create_index('{"fields": [{"field": "$.myField", "type": "BIGINT UNSIGNED"}]}');

// FR1_17 Create an index using a integer field.
expect_create_index('{"fields": [{"field": "$.myField", "type": "INTEGER UNSIGNED"}]}');

// FR1_18 Create an index using a mediumint field.
expect_create_index('{"fields": [{"field": "$.myField", "type": "MEDIUMINT UNSIGNED"}]}');

// FR1_19 Create an index using a smallint field.
expect_create_index('{"fields": [{"field": "$.myField", "type": "SMALLINT UNSIGNED"}]}');

// FR1_20 Create an index using a tinyint field.
expect_create_index('{"fields": [{"field": "$.myField", "type": "TINYINT UNSIGNED"}]}');

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
