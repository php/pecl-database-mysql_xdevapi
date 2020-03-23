--TEST--
mysqlx schema validation simple
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
				}
			},
			"required": ["latitude", "longitude"]
		}
	}
}');

$coll->add('{"latitude": 10, "longitude": 20}')->execute();

expect_eq($coll->count(), 1);

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
