--TEST--
mysqlx schema validation no schema
--SKIPIF--
--FILE--
<?php
require(__DIR__."/../connect.inc");

$session = create_test_session(true);
$schema = $session->getSchema($db);

$coll = $schema->createCollection("mycollection", '{
	"validation": {
		"level": "off"
	}
}');

$coll->add('{"name": "Barbara", "city": "Katowice"}')->execute();

expect_eq($coll->count(), 1);



$coll2 = $schema->createCollection("mycollection2", '{
	"validation": {
	}
}');

$coll2->add('{"name": "Agnieszka", "city": "Zory"}')->execute();

expect_eq($coll2->count(), 1);

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
