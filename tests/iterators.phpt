--TEST--
mysqlx iterators
--SKIPIF--
--FILE--
<?php
	require_once("connect.inc");

	$session = create_test_db();
	$schema = $session->getSchema($db);
	$coll = $schema->getCollection("test_collection");

	fill_db_collection($coll);

	$expected_docs = [
		[11, "Barista", "Lucia"],
		[2 , "Paninaro", "Lonardo"],
		[6 , "Programmatrice", "Mariangela"],
		[8 , "Studente", "Antonella"]
	];

	$res = $coll->find('age > 40')->execute();
	expect_eq(count($res->fetchAll()), 4);
		$idx = 0;
	foreach( $res as $it ) {
		expect_eq_id($it["_id"],$expected_docs[$idx][0]);
		expect_eq($it["job"],$expected_docs[$idx][1]);
		expect_eq($it["name"],$expected_docs[$idx][2]);
		$idx++;
	}

	fill_db_table();

	$expected_rows = [
		[16, "Vesper"],
		[17, "Caspian"],
		[17, "Romy"]
	];

	$sql = $session->sql("select * from $db.$test_table_name where age > 15")->execute();
	$idx = 0;
	foreach( $sql as $it ) {
		expect_eq($it["age"],$expected_rows[$idx][0]);
		expect_eq($it["name"],$expected_rows[$idx][1]);
		$idx++;
	}

	$schema = $session->getSchema($db);
	$table = $schema->getTable($test_table_name);

	$res = $table->select(['age','name'])->where("age > 15")->execute();
	$idx = 0;
	foreach( $res as $it ) {
		expect_eq($it["age"],$expected_rows[$idx][0]);
		expect_eq($it["name"],$expected_rows[$idx][1]);
		$idx++;
	}
	$res = $table->insert('age','name')->values([23,'test'],
					[22,'test2'],
					[33,'test3'])->execute();
	expect_eq($res->getAffectedItemsCount(), 3);
	foreach( $res as $it ) {
		test_step_failed();
	}

	$session->sql("create database if not exists testx2")->execute();
	$session->sql("create database if not exists testx3")->execute();
	$session->sql("create database if not exists testx4")->execute();

	$expected_db = [$db, 'testx2', 'testx3', 'testx4'];

	$sql = $session->sql("show databases")->execute()->fetchAll();
	$idx = 0;
	foreach( $sql as $it ) {
		$dbname = $it["Database"];
		if (in_array($dbname, $expected_db)) {
			$idx++;
			$session->sql("drop database $dbname")->execute();
		}
	}

	expect_eq($idx, count($expected_db));

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
