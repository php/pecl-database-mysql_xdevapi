--TEST--
mysqlx create multi index
--SKIPIF--
--FILE--
<?php
require_once(__DIR__."/../connect.inc");
require_once(__DIR__."/index_utils.inc");

$session = create_test_db();
$schema = $session->getSchema($db);
$coll = $schema->getCollection($test_collection_name);

expect_create_multi_index(
	'{"fields": '.
	'[{"field": "$.myField", "type": "TEXT(13)", "required": true},'.
	' {"field": "$.myField2", "type": "TEXT(10)", "required": false},'.
	' {"field": "$.myField3", "type": "INT"}]}',
	array(
		array($Default_index_name, 1, ''),
		array($Default_index_name, 1, 'YES'),
		array($Default_index_name, 1, 'YES')
	)
);

expect_create_multi_index(
	'{"fields": '.
	'[{"field": "$.myField", "type": "TEXT(13)", "required": true},'.
	' {"field": "$.myField2", "type": "DATE", "required": false},'.
	' {"field": "$.myField3", "type": "DOUBLE UNSIGNED", "required": false}],'.
	'"unique": false}',
	array(
		array($Default_index_name, 1, ''),
		array($Default_index_name, 1, 'YES'),
		array($Default_index_name, 1, 'YES')
	)
);

fill_db_collection($coll);

$index_name = "name_age_job_index";
expect_create_multi_index_with_name($index_name,
	'{"fields": '.
	'[{"field": "$.name", "type": "TEXT(20)", "required": true},'.
	' {"field": "$.age", "type": "INTEGER"}, '.
	' {"field": "$.job", "type": "TEXT(30)", "required": false}], '.
	'"unique": true}',
	array(
		array($index_name, 1, ''),
		array($index_name, 1, 'YES'),
		array($index_name, 1, 'YES')
	)
);

$index_name = "ordinal_name_age_index";
expect_create_multi_index_with_name($index_name,
	'{"fields": '.
	'[{"field": "$.ordinal", "type": "INTEGER", "required": true},'.
	' {"field": "$.name", "type": "TEXT(20)"},'.
	' {"field": "$.age", "type": "NUMERIC UNSIGNED", "required": false}],'.
	'"unique": true}',
	array(
		array($index_name, 1, ''),
		array($index_name, 1, 'YES'),
		array($index_name, 1, 'YES')
	)
);

$index_name = "id_ordinal_age_job_index";
expect_create_multi_index_with_name($index_name,
	'{"fields": '.
	'[{"field": "$._id", "type": "TEXT(128)", "required": true},'.
	' {"field": "$.ordinal", "type": "INTEGER", "required": false}, '.
	' {"field": "$.age", "type": "BIGINT"}, '.
	' {"field": "$.job", "type": "TEXT(30)", "required": false}]}',
	array(
		array($index_name, 1, ''),
		array($index_name, 1, 'YES'),
		array($index_name, 1, 'YES'),
		array($index_name, 1, 'YES')
	)
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
