--TEST--
mysqlx prepared statement remove
--SKIPIF--
--INI--
error_reporting=0
--FILE--
<?php
    require_once("ps_utils.inc");
	$session = create_test_db();

    $schema = $session->getSchema($db);
	$coll = $schema->getCollection("test_collection");

    fill_db_collection($coll);

    $perf_schema = $session->getSchema("performance_schema");
	$perf_schema_table = $perf_schema->getTable("prepared_statements_instances");

    $sql_strings = array(
    1 => "DELETE FROM `$db`.`test_collection` WHERE JSON_CONTAINS(JSON_QUOTE('Programmatore'),JSON_EXTRACT(doc,'$.job')) LIMIT ?",
    2 => "DELETE FROM `$db`.`test_collection` WHERE (JSON_EXTRACT(doc,'$.age') > ?) LIMIT ?",
    3 => "DELETE FROM `$db`.`test_collection` WHERE (JSON_UNQUOTE(JSON_EXTRACT(doc,'$.job')) IN ('Barista','Programmatore','Ballerino','Programmatrice','Disoccupato')) ORDER BY JSON_EXTRACT(doc,'$.age') DESC,JSON_EXTRACT(doc,'$.name') LIMIT ?"
	);

    $coll->remove("job in 'Programmatore'")->limit(1)->execute();
	$stmt_id = get_stmt_id(0); //New PS
	$coll->remove("job in 'Programmatore'")->limit(2)->execute();
	verify_op_ps(0, $stmt_id, 1, 2);//Same PS
	$coll->remove('age > :age')->bind(['age' => 30])->limit(1)->execute();
	$stmt_id = get_stmt_id(2); //New PS
	$coll->remove('age > :age')->bind(['age' => 31])->limit(2)->execute();
	verify_op_ps(2, $stmt_id, 2, 3);//Same PS
	$coll->remove("job in ('Barista', 'Programmatore', 'Ballerino', 'Programmatrice','Disoccupato')")->limit(2)->sort(['age desc', 'name asc'])->execute();
	$stmt_id = get_stmt_id(3); //New PS
	$coll->remove("job in ('Barista', 'Programmatore', 'Ballerino', 'Programmatrice','Disoccupato')")->limit(2)->sort(['age desc', 'name asc'])->execute();
	verify_op_ps(3, $stmt_id, 3, 4);//Same PS

    $obj = $coll->find()->execute();
	$res = $obj->fetchAll();
	expect_eq(count($res),6);

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
