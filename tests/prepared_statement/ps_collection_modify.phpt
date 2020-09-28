--TEST--
mysqlx prepared statement modify
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
	1 => "UPDATE `$db`.`test_collection` SET doc=JSON_SET(JSON_REMOVE(doc,'$.age','$.name'),'$._id',JSON_EXTRACT(`doc`,'$._id')) WHERE (JSON_UNQUOTE(JSON_EXTRACT(doc,'$.job')) LIKE ?)",
	2 => "UPDATE `$db`.`test_collection` SET doc=JSON_SET(JSON_SET(doc,'$.hobby','nr_1'),'$._id',JSON_EXTRACT(`doc`,'$._id')) WHERE (JSON_EXTRACT(doc,'$.age') > ?)",
	3 => "UPDATE `$db`.`test_collection` SET doc=JSON_SET(JSON_SET(doc,'$.hobby','nr_10'),'$._id',JSON_EXTRACT(`doc`,'$._id')) WHERE (JSON_EXTRACT(doc,'$.age') > ?)",
	4 => "UPDATE `$db`.`test_collection` SET doc=JSON_SET(JSON_SET(doc,'$.hobby','nr_11'),'$._id',JSON_EXTRACT(`doc`,'$._id')) WHERE (JSON_EXTRACT(doc,'$.age') > ?)",
	5 => "UPDATE `$db`.`test_collection` SET doc=JSON_SET(JSON_SET(doc,'$.hobby','nr_12'),'$._id',JSON_EXTRACT(`doc`,'$._id')) WHERE (JSON_EXTRACT(doc,'$.age') > ?)",
	6 => "UPDATE `$db`.`test_collection` SET doc=JSON_SET(JSON_SET(doc,'$.hobby','nr_13'),'$._id',JSON_EXTRACT(`doc`,'$._id')) WHERE (JSON_EXTRACT(doc,'$.age') > ?)",
	7 => "UPDATE `$db`.`test_collection` SET doc=JSON_SET(JSON_SET(doc,'$.hobby','nr_14'),'$._id',JSON_EXTRACT(`doc`,'$._id')) WHERE (JSON_EXTRACT(doc,'$.age') > ?)",
	9 => "SELECT doc FROM `$db`.`test_collection`"
	);

    verify_op_ps( 0, 1, 0, 1 );
	$coll->modify('job like :job_name')->unset(["age", "name"])->bind(['job_name' => 'Plumber'])->execute();
	verify_op_ps( 1, 2, 1, 2 );
	$coll->modify('age > :age')->set("hobby", "nr_1")->bind(['age' => 25 ] )->execute();
	verify_op_ps( 2, 3, 2, 3 );

    $obj = $coll->find()->execute();
	$res = $obj->fetchAll();
	expect_eq(count( $res ), 16);
	for( $i = 0 ; $i < 16 ; $i++ ) {
	    if( $res[$i]["age"] > 25 ) {
		    expect_eq($res[$i]["hobby"],"nr_1");
		} else {
		    expect_null($res[$i]["hobby"]);
		}
	}
	$stmt_id = get_stmt_id(3);
	verify_op_ps( 3, $stmt_id, 9, 4 );
	//Use the PS over and over.
	for( $i = 0 ; $i < 10 ; $i++ ) {
	    $coll->modify('age > :age')->set("hobby", "nr_1")->bind(['age' => 25 ] )->execute();
		verify_op_ps( 3, $stmt_id, 9, 4 );
	}

    //Generate new PSs
	$prev_stmt_id = get_stmt_id(3);
	for( $i = 0 ; $i < 5 ; $i++ ) {
	    $coll->modify('age > :age')->set("hobby", "nr_" . ( 10 + $i ))->bind(['age' => 25 ] )->execute();
		$stmt_id = get_stmt_id($i + 4);
		expect_true($prev_stmt_id != $stmt_id);
		verify_op_ps( $i + 4, $stmt_id, $i + 3, $i + 5 );
		$prev_stmt_id = $stmt_id;
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
