--TEST--
mysqlx schema validation modify schema
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
					"description": "employees list",
					"type": "object",
					"properties": {
						"name": {
							"type": "string"
						},
						"job": {
							"type": "string"
						}
					},
					"required": ["name", "job"]
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
		"level": "strict",
		"schema": {
			"id": "http://json-schema.org/geo",
			"description": "candidates list",
			"type": "object",
			"properties": {
				"ordinal": {
					"type": "number"
				},
				"name": {
					"type": "string"
				},
				"age": {
					"type": "number"
				}
			},
			"required": ["ordinal", "name", "age"]
		}
	}
}');

$coll->add('{"ordinal": 1, "name": "Riccardo", "age": 37}')->execute();
$coll->add('{"ordinal": 2, "name": "Carlotta", "age": 33}')->execute();
$coll->add('{"ordinal": 3, "name": "Mariangela", "age": 41}')->execute();
$coll->add('{"ordinal": 4, "name": "Alfredo", "age": 27}')->execute();
expect_eq($coll->count(), 4);

expect_false(try_modify_collection());

try {
	$coll->modify(true)->unset('ordinal')->unset('age')->execute();
	test_step_failed();
} catch(Exception $ex) {
	test_step_ok();
}

$coll->modify('name like "Riccardo"')->set("job", "Cantante")->execute();
$coll->modify('name like "Carlotta"')->set("job", "Programmatrice")->execute();
$coll->modify('name like "Mariangela"')->set("job", "Ballerino")->execute();
$coll->modify('name like "Alfredo"')->set("job", "Barista")->execute();

expect_true(try_modify_collection());

$coll->modify(true)->unset('ordinal')->unset('age')->execute();

expect_eq($coll->count(), 4);

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
