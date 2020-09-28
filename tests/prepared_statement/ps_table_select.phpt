--TEST--
mysqlx prepared statement table select
--SKIPIF--
--FILE--
<?php
    require_once("ps_utils.inc");
	$session = create_test_db();

    fill_db_table_use_dup();

    $schema = $session->getSchema($db);
	$table = $schema->getTable('test_table');
	$perf_schema = $session->getSchema("performance_schema");
	$perf_schema_table = $perf_schema->getTable("prepared_statements_instances");

    $sql_strings = array(
    1 => "SELECT `name` AS `name`,`age` AS `age` FROM `$db`.`test_table` WHERE ((`name` LIKE ?) AND (`age` > ?)) ORDER BY `age` DESC",
    2 => "SELECT `name` AS `name`,`age` AS `age` FROM `$db`.`test_table` WHERE (`age` > ?) LIMIT ?",
    3 => "UPDATE `$db`.`test_table` SET `age`=1 WHERE ((`age` < ?) AND (`age` != 1)) ORDER BY `name` DESC LIMIT ?"
	);

    $res = $table->select('name','age')->where('name like :name and age > :age')->bind(['name' => 'Tierney', 'age' => 34])->orderBy('age desc')->execute();
	$stmt_id = get_stmt_id(0); //New PS
	$data = $res->fetchAll();
	expect_eq(count($data), 2);
	expect_eq($data[0]["name"], "Tierney");
	expect_eq($data[1]["name"], "Tierney");
	$res = $table->select('name','age')->where('name like :name and age > :age')->bind(['name' => 'Mamie', 'age' => 22])->orderBy('age desc')->execute();
	verify_op_ps(0, $stmt_id, 1, 2);//Same PS
	$data = $res->fetchAll();
	expect_eq(count($data), 1);
	expect_eq($data[0]["name"], "Mamie");
	$res = $table->select('name','age')->where('age > :age')->limit(1)->bind(['age' => 22])->execute();
	$stmt_id = get_stmt_id(2); //New PS
	$data = $res->fetchAll();
	expect_eq(count($data), 1);
	expect_eq($data[0]["name"], "Mamie");
	$results = array(
	    "Mamie" => 0,
		"Polly" => 0,
		"Tierney" => 0
	);
	for( $i = 1 ; $i <= 20 ; $i++ ) {
	    $res = $table->select('name','age')->where('age > :age')->limit(1)->bind(['age' => 22 + $i])->execute();
		verify_op_ps(2, $stmt_id, 2, 3);//Same PS
		$data = $res->fetchAll();
		expect_eq(count($data), 1);
		$results[$data[0]["name"]]++;
	}
	expect_eq($results["Mamie"],6);
	expect_eq($results["Polly"],5);
	expect_eq($results["Tierney"],9);
	verify_expectations();
	print "done!\n";
?>
--CLEAN--
<?php
    require_once(__DIR__."/../connect.inc");
	clean_test_db();
?>
--EXPECTF--
done!%A
