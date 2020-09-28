--TEST--
mysqlx prepared statement table delete
--SKIPIF--
--FILE--
<?php
    require_once("ps_utils.inc");

    $session = create_test_db();

    fill_db_table_use_dup();
	fill_db_table_use_dup();
	fill_db_table_use_dup();

    $schema = $session->getSchema($db);
	$table = $schema->getTable('test_table');
	$perf_schema = $session->getSchema("performance_schema");
	$perf_schema_table = $perf_schema->getTable("prepared_statements_instances");

    $sql_strings = array(
    1 => "DELETE FROM `$db`.`test_table` WHERE (`name` = ?) ORDER BY `age` DESC LIMIT ?",
    2 => "DELETE FROM `$db`.`test_table` WHERE ((`name` = ?) OR (`age` > ?)) LIMIT ?",
	);

    $table->delete()->where('name = :name')->orderby('age desc')->limit(2)->bind(['name' => 'Tierney'])->execute();
	$stmt_id0 = get_stmt_id(0); //New PS
	$table->delete()->where('name = :name')->orderby('age desc')->limit(2)->bind(['name' => 'Mamie'])->execute();
	verify_op_ps(0, $stmt_id0, 1, 2);//Same PS
	$table->delete()->where('name = :name')->orderby('age desc')->limit(1)->bind(['name' => 'Mamie'])->execute();
	verify_op_ps(0, $stmt_id0, 1, 2);//Same PS
	$table->delete()->where('name = :name')->orderby('age desc')->limit(6)->bind(['name' => 'Cassidy'])->execute();
	verify_op_ps(0, $stmt_id0, 1, 2);//Same PS

    $table->delete()->where('name = :name or age > :age')->limit(4)->bind(['name' => 'Polly', 'age' => 20])->execute();
	$stmt_id1 = get_stmt_id(2); //New PS
	$table->delete()->where('name = :name or age > :age')->limit(3)->bind(['name' => 'ARomy', 'age' => 17])->execute();
	verify_op_ps(2, $stmt_id1, 2, 3);//Same PS
	$table->delete()->where('name = :name or age > :age')->limit(8)->bind(['name' => 'BRomy', 'age' => 17])->execute();
	verify_op_ps(2, $stmt_id1, 2, 3);//Same PS
	$table->delete()->where('name = :name')->orderby('age desc')->limit(2)->bind(['name' => 'Tierney'])->execute();
	verify_op_ps(0, $stmt_id0, 1, 3);//Same (old) PS

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
