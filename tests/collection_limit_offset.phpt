--TEST--
mysqlx collection limit offset
--SKIPIF--
--INI--
error_reporting=E_ALL
--FILE--
<?php
require("connect.inc");
$session = create_test_db();

$schema = $session->getSchema($db);
$coll = $schema->getCollection($test_collection_name);

fill_db_collection($coll, true);

// fetch all from given age range
$query = $coll->find(':age_min < age and age < :age_max')->fields('name');
$query = $query->bind(['age_min' => 30, 'age_max' => 45])->sort('age desc');
$res = $query->execute();
$data = $res->fetchAll();
expect_eq(count($data), 7);

// fetch only 3 records from given age range
$query = $query->limit(3);
$res = $query->execute();
$data = $res->fetchAll();
expect_eq(count($data), 3);
expect_eq($data[0]['name'], 'Antonella');
expect_eq($data[1]['name'], 'Mariangela');
expect_eq($data[2]['name'], 'Francesco');

// fetch only 3 records from given age range with 2 rows offset (counting
// from 3rd record)
$query = $query->offset(2);
$res = $query->execute();
$data = $res->fetchAll();
expect_eq(count($data), 3);
expect_eq($data[0]['name'], 'Francesco');
expect_eq($data[1]['name'], 'Gianluigi');
expect_eq($data[2]['name'], 'Carlo');

// fetch only 3 records from given age range with 5 rows offset (counting from
// 6th record), but there aren't enough records - so in the result there are 2
$query = $query->offset(5);
$res = $query->execute();
$data = $res->fetchAll();
expect_eq(count($data), 2);
expect_eq($data[0]['name'], 'Monica');
expect_eq($data[1]['name'], 'Filippo');

// offset 8 is behind all records from given age range (there are only 7 records)
$query = $query->offset(8);
$res = $query->execute();
$data = $res->fetchAll();
expect_eq(count($data), 0);


// offset requires limit
$query2 = $coll->find("job like 'Barista'")->offset(1)->sort('age desc');
try {
	$query2->execute();
	test_step_failed();
} catch(Exception $ex) {
	expect_eq(
		$ex->getMessage(),
		"[10057][HY000] The use of 'offset' without 'limit' is not allowed");
}

$query2 = $query2->limit(1);
$res = $query2->execute();
$data = $res->fetchAll();
expect_eq(count($data), 1);
expect_eq($data[0]['age'], 15);

verify_expectations();
print "done!\n";
?>
--CLEAN--
<?php
require("connect.inc");
clean_test_db();
?>
--EXPECTF--
done!%A
