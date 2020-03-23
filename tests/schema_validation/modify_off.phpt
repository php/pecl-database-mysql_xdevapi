--TEST--
mysqlx schema validation level off
--SKIPIF--
--FILE--
<?php
require(__DIR__."/../connect.inc");

$session = create_test_session(true);
$schema = $session->getSchema($db);

$coll = $schema->createCollection("mycollection", '{
	"validation": {
		"level": "strict",
		"schema": {
			"id": "http://json-schema.org/geo",
			"description": "A geographical coordinate",
			"type": "object",
			"properties": {
				"latitude": {
					"type": "number"
				},
				"longitude": {
					"type": "number"
				},
				"name": {
					"type": "string"
				},
				"comment": {
					"type": "string"
				}
			},
			"required": ["latitude", "longitude", "name"]
		}
	}
}');

$coll->add('{"latitude": 10, "longitude": 20, "name": "Cracow"}')->execute();

expect_eq($coll->count(), 1);

try {
	$coll->add('{"name": "Wroclaw", "comment": "other"}')->execute();
	test_step_failed();
} catch(Exception $ex) {
	test_step_ok();
}

try {
	$coll->add('{"latitude": "lat", "longitude": "long", "name": "Warsaw"}')->execute();
	test_step_failed();
} catch(Exception $ex) {
	test_step_ok();
}

expect_eq($coll->count(), 1);

$schema->modifyCollection("mycollection", '{
	"validation": {
		"level": "off"
	}
}');

$coll->add('{"name": "Wroclaw", "comment": "other"}')->execute();
$coll->add('{"latitude": "lat", "longitude": "long", "name": "Warsaw"}')->execute();

expect_eq($coll->count(), 3);

verify_expectations();
print "done!\n";
?>
--CLEAN--
<?php
require(__DIR__."/../connect.inc");
clean_test_db();
?>
--EXPECTF--
done!%A
