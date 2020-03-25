--TEST--
mysqlx schema validation modify level strict
--SKIPIF--
--FILE--
<?php
require(__DIR__."/../connect.inc");

function try_modify_collection() {
	global $schema;
	try {
		$schema->modifyCollection("mycollection", '{
			"validation": {
				"level": "strict",
				"schema": {
					"id": "http://json-schema.org/geo",
					"description": "Formula 1 teams",
					"type": "object",
					"properties": {
						"identifier": {
							"type": "number"
						},
						"brand": {
							"type": "string"
						},
						"location": {
							"type": "string"
						}
					},
					"required": ["identifier", "brand", "location"]
				}
			}
		}');
		return true;
	} catch(Exception $ex) {
		return false;
	}
}

$session = create_test_session(true);
$schema = $session->getSchema($db);

$coll = $schema->createCollection("mycollection", '{
	"validation": {
		"level": "off"
	}
}');

$coll->add('{"identifier":0, "brand": "BMW", "location": "Germany"}')->execute();
$coll->add('{"identifier":1, "brand": "Honda", "location": "Japan"}')->execute();
$coll->add('{"identifier":2, "location": "France"}')->execute();
$coll->add('{"identifier":3, "brand": "Ferrari", "location": "Italy"}')->execute();
$coll->add('{"identifier":4, "brand": "McLaren"}')->execute();
$coll->add('{"name": "Maryjan", "surname": "Pazdzioch"}')->execute();

expect_eq($coll->count(), 6);
expect_false(try_modify_collection());

$coll->remove('name like "Maryjan"')->execute();

expect_eq($coll->count(), 5);
expect_false(try_modify_collection());

$coll->modify('brand like "McLaren"')->set("location", "Great Britain")->execute();
$coll->modify('location like "France"')->set("brand", "Renault")->execute();

expect_true(try_modify_collection());
expect_eq($coll->count(), 5);

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
