--TEST--
mysqlx schema validation parse error
--SKIPIF--
--FILE--
<?php
require(__DIR__."/../connect.inc");

$session = create_test_session(true);
$schema = $session->getSchema($db);

try {
	$schema_validation_with_error = '{ "validation": / { "level": "off" } }';
	$coll = $schema->createCollection("mycollection", $schema_validation_with_error);
	test_step_failed();
} catch(Exception $e) {
	echo $e->GetMessage(), "\n";
	test_step_ok();
}

$coll2 = $schema->createCollection("mycollection2", '{
	"validation": {
		"level": "strict",
		"schema": {
			"id": "http://json-schema.org/geo",
			"description": "A simple schema",
			"type": "object",
			"properties": {
				"value": {
					"type": "number"
				}
			}
		}
	}
}');

try {
	$schema_validation_with_error2 = '{ "validation": { "level": "off" } ! }';
	$schema->modifyCollection("mycollection2", $schema_validation_with_error2 );
	test_step_failed();
} catch(Exception $e) {
	echo $e->GetMessage(), "\n";
	test_step_ok();
}

verify_expectations();
print "done!\n";
?>
--CLEAN--
<?php
require(__DIR__."/../connect.inc");
clean_test_db();
?>
--EXPECTF--
[10077][HY000] json parse error, code: 4
[10077][HY000] json parse error, code: 4
done!%A
