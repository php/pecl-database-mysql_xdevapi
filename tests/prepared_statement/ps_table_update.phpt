--TEST--
mysqlx prepared statement table update
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
    1 => "UPDATE `$db`.`test_table` SET `name`='Alfonso' WHERE ((`name` = ?) AND (`age` > ?))",
    2 => "UPDATE `$db`.`test_table` SET `age`=1 WHERE ((`age` < ?) AND (`age` != 1)) LIMIT ?",
    3 => "UPDATE `$db`.`test_table` SET `age`=1 WHERE ((`age` < ?) AND (`age` != 1)) ORDER BY `name` DESC LIMIT ?"
	);

    $res = $table->update()->set('name', 'Alfonso')->where('name = :name and age > :age')->bind(['name' => 'Cassidy', 'age' => 30])->execute();
	$stmt_id = get_stmt_id(0); //New PS
	$res = $table->update()->set('name', 'Alfonso')->where('name = :name and age > :age')->bind(['name' => 'Tierney', 'age' => 40])->execute();
	verify_op_ps(0 ,$stmt_id ,1 ,2 ); //Same PS
	$res = $table->update()->set('age', 1)->limit(2)->where('age < :age and age != 1')->bind(['age' => 30])->execute();
	$stmt_id = get_stmt_id(2); //New PS
	$res = $table->update()->set('age', 1)->limit(3)->where('age < :age and age != 1')->bind(['age' => 30])->execute();
	verify_op_ps(2 ,$stmt_id ,2 ,3 ); //Same PS
	$res = $table->update()->set('age', 1)->limit(3)->orderby("name DESC")->where('age < :age and age != 1')->bind(['age' => 30])->execute();
	$stmt_id = get_stmt_id(3); //New PS
	$res = $table->update()->set('age', 1)->limit(1)->orderby("name DESC")->where('age < :age and age != 1')->bind(['age' => 10])->execute();
	verify_op_ps(3 ,$stmt_id ,3 ,4 ); //Same PS

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
