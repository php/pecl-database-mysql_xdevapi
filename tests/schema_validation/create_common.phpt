--TEST--
mysqlx schema validation common
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

$coll->add('{"latitude": 10, "longitude": 20, "name": "Munich", "comment": "scripting"}')->execute();
$coll->add('{"latitude": 30, "longitude": 40, "name": "Gdansk"}')->execute();

expect_eq($coll->count(), 2);

try {
	$coll->add('{"latitude": "incorrect", "longitude": 60, "name": "Katowice"}')->execute();
	test_step_failed();
} catch(Exception $ex) {
	test_step_ok();
}

try {
	$coll->add('{"latitude": "50", "longitude": 70}')->execute();
	test_step_failed();
} catch(Exception $ex) {
	test_step_ok();
}

expect_eq($coll->count(), 2);

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
