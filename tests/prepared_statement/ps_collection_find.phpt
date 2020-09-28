--TEST--
mysqlx prepared statement find
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
    1 => "SELECT doc FROM `$db`.`test_collection` WHERE (JSON_UNQUOTE(JSON_EXTRACT(doc,'$.job')) LIKE ?) ORDER BY JSON_EXTRACT(doc,'$.age') LIMIT ?, ?",
    2 => "SELECT doc FROM `$db`.`test_collection` WHERE (JSON_UNQUOTE(JSON_EXTRACT(doc,'$._id')) LIKE ?)"
	);

    verify_op_ps(0, 1, 0, 1);
	$res = $coll->find('job like :job')->bind(['job' => 'Programmatore'])->limit(1)->offset(3)->sort('age asc')->execute();
	$stmt_id = get_stmt_id(1); //New PS
	$res = $coll->find('job like :job')->bind(['job' => 'Programmatrice'])->limit(4)->offset(1)->sort('age asc')->execute();
	verify_op_ps(1, $stmt_id, 1, 2); //Same PS
	$data = $res->fetchAll();
	expect_eq($data[0]["name"],"Mariangela");
	$res = $coll->find('job like :job')->bind(['job' => 'Barista'])->limit(1)->offset(1)->sort('age asc')->execute();
	verify_op_ps(1, $stmt_id, 1, 2); //Same PS
	$data = $res->fetchAll();
	expect_eq($data[0]["name"],"Lucia");
	$res = $coll->find('_id like :id')->bind(['id' => 1])->execute();
	$data = $res->fetchAll();
	expect_eq($data[0]["name"],"Marco");
	$stmt_id = get_stmt_id(2); //New PS
	$names = [
	    "Lonardo",
		"Riccardo",
		"Carlotta",
		"Carlo",
		"Mariangela",
		"Alfredo",
		"Antonella",
		"Monica",
		"Giulio"
	];
	for( $i = 2 ; $i <= 10 ; $i++ ) {
	    $res = $coll->find('_id like :id')->bind(['id' => $i])->execute();
		$data = $res->fetchAll();
		expect_eq($data[0]["name"],$names[$i-2]);
		verify_op_ps(2, $stmt_id, 2, 3); //Same PS
	}

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
