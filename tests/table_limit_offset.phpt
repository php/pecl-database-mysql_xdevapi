--TEST--
mysqlx table limit offset
--SKIPIF--
--INI--
error_reporting=E_ALL
--FILE--
<?php
require("connect.inc");
$session = create_test_db();

$schema = $session->getSchema($db);
$table = $schema->getTable($test_table_name);

fill_db_table($table);

// fetch all from given age range
$query = $table->select('name', 'age')
	->where(':age_min <= age and age < :age_max')
	->bind(['age_min' => 12, 'age_max' => 15])->orderBy('age desc', 'name asc');
$res = $query->execute();
$data = $res->fetchAll();
expect_eq(count($data), 5);

// fetch only 3 records from given age range
$query = $query->limit(3);
$res = $query->execute();
$data = $res->fetchAll();
expect_eq(count($data), 3);
expect_eq($data[0]['name'], 'Lev');
expect_eq($data[1]['name'], 'Olympia');
expect_eq($data[2]['name'], 'Cassidy');

// fetch only 3 records from given age range with 2 rows offset (counting
// from 3rd record)
$query = $query->offset(2);
$res = $query->execute();
$data = $res->fetchAll();
expect_eq(count($data), 3);
expect_eq($data[0]['name'], 'Cassidy');
expect_eq($data[1]['name'], 'Polly');
expect_eq($data[2]['name'], 'Rufus');

// fetch only 3 records from given age range with 4 rows offset (counting from
// 5th record, including it), but there aren't enough records - so in the result
// there is only 1
$query = $query->offset(4);
$res = $query->execute();
$data = $res->fetchAll();
expect_eq(count($data), 1);
expect_eq($data[0]['name'], 'Rufus');

// offset 6 is behind all records from given age range (there are only 5 records)
$query = $query->offset(6);
$res = $query->execute();
$data = $res->fetchAll();
expect_eq(count($data), 0);


// offset requires limit
$query2 = $table->select("name")->where('age = 17')->offset(1)->orderBy('age desc');
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
expect_eq($data[0]['name'], 'Romy');

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
