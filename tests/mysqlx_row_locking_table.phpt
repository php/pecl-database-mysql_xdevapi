--TEST--
mysqlx table row locking
--SKIPIF--
--INI--
error_reporting=0
--FILE--
<?php
	require_once("connect.inc");

	function check_value($val, $expected_id, $expected_n) {
		$stack_depth = 3;
		expect_eq($val['id'], $expected_id, 'id', $stack_depth);
		expect_eq($val['n'], $expected_n, 'n', $stack_depth);
	}

	function check_one($res, $expected_id, $expected_n) {
		$val = $res->fetchOne();
		check_value($val, $expected_id, $expected_n);
	}

	function check_all($res, $expected_vals) {
		$vals = $res->fetchAll();
		$i = 0;
		expect_eq(count($vals), count($expected_vals));
		foreach ($expected_vals as $id => $n) {
			check_value($vals[$i++], $id, $n);
		}
	}

	$session1 = mysql_xdevapi\getSession($connection_uri);
	$session2 = mysql_xdevapi\getSession($connection_uri);

	$session1->createSchema($test_schema_name);
	$schema1 = $session1->getSchema($test_schema_name);

	$schema1->createTable($test_table_name)
		->addColumn(new mysql_xdevapi\ColumnDef('id', 'varchar', 1024))
		->addColumn(new mysql_xdevapi\ColumnDef('n', 'int'))
		->execute();

	$tab1 = $schema1->getTable($test_table_name);

	$tab1->insert(["id", "n"])->values(["1", 1])->execute();
	$tab1->insert(["id", "n"])->values(["2", 2])->execute();
	$tab1->insert(["id", "n"])->values(["3", 3])->execute();

	$schema2 = $session2->getSchema($test_schema_name);
	$tab2 = $schema2->getTable($test_table_name);

	// test1: Shared Lock

	$session1->startTransaction();
	$res1 = $tab1->select('id', 'n')->where("id like '1'")->lockShared()->execute();
	check_one($res1, '1', 1);

	$session2->startTransaction();

	// should return immediately
	$res2 = $tab2->select('id', 'n')->where("id like '2'")->lockShared()->execute();
	check_one($res2, '2', 2);

	// should return immediately
	$res2 = $tab2->select('id', 'n')->where("id like '1'")->lockShared()->execute();
	check_one($res2, '1', 1);

	$session1->rollback();
	$session2->rollback();


	// test2: Shared Lock after Exclusive

	$session1->startTransaction();
	$res1 = $tab1->select('id', 'n')->where("id like '1'")->lockExclusive()->execute();
	check_one($res1, '1', 1);

	$session2->startTransaction();

	// should return immediately
	$res2 = $tab2->select('id', 'n')->where("id like '2'")->lockShared()->execute();
	check_one($res2, '2', 2);

	// $session2 blocks
	$res2 = $tab2->select('id', 'n')->where("id like '1'")->lockShared()->execute();
	check_one($res2, '1', 1);

	$session1->rollback(); // $session2 should unblock now
	$session2->rollback();


	// test3: Exclusive after Shared

	$session1->startTransaction();
	$res1 = $tab1->select('id', 'n')->where("id in ('1', '3')")->lockShared()->execute();
	check_all($res1, ['1' => 1, '3' => 3]);

	$session2->startTransaction();

	// should return immediately
	$res2 = $tab2->select('id', 'n')->where("id like '2'")->lockExclusive()->execute();
	check_one($res2, '2', 2);

	// should return immediately
	$res2 = $tab2->select('id', 'n')->where("id like '3'")->lockShared()->execute();
	check_one($res2, '3', 3);

	// $session2 should block
	$res2 = $tab2->select('id', 'n')->where("id like '1'")->lockExclusive()->execute();
	check_one($res2, '1', 1);

	$session1->rollback(); // $session2 should unblock now
	$session2->rollback();


	// test4: Exclusive after Exclusive

	$session1->startTransaction();
	$res1 = $tab1->select('id', 'n')->where("id like '1'")->lockExclusive()->execute();
	check_one($res1, '1', 1);

	$session2->startTransaction();

	// should return immediately
	$res2 = $tab2->select('id', 'n')->where("id like '2'")->lockExclusive()->execute();
	check_one($res2, '2', 2);

	// $session2 should block
	$res2 = $tab2->select('id', 'n')->where("id like '1'")->lockExclusive()->execute();
	check_one($res2, '1', 1);

	$session1->rollback(); // $session2 should unblock now
	$session2->rollback();


	// test5: Shared Lock read after Exclusive write

	$session1->startTransaction();

	$res1 = $tab1->select('id', 'n')->where("id in ('1', '2')")->lockExclusive()->execute();
	$tab1->update()->set('n', 11)->where("id = '1'")->execute();
	$tab1->update()->set('n', 22)->where("id = '2'")->execute();
	$res1 = $tab1->select('id', 'n')->where("id in ('1', '2')")->lockExclusive()->execute();
	check_one($res1, '1', 11);
	check_one($res1, '2', 22);

	$session2->startTransaction();

	// should return immediately
	$res2 = $tab2->select('id', 'n')->where("id like '2'")->lockShared()->execute();
	check_one($res2, '2', 2);

	// $session2 blocks
	$res2 = $tab2->select('id', 'n')->where("id like '1'")->lockShared()->execute();
	check_one($res2, '1', 1);


	$session1->commit(); // $session2 should unblock now
	$session2->commit();


	// test6: Exclusive write after Shared read

	$session1->startTransaction();
	$res1 = $tab1->select('id', 'n')->where("id in ('1', '3')")->lockShared()->execute();
	check_all($res1, ['1' => 11, '3' => 3]);

	$session2->startTransaction();

	// should return immediately
	$res2 = $tab2->select('id', 'n')->where("id like '2'")->lockExclusive()->execute();
	$tab2->update()->set('n', 222)->where("id = '2'")->execute();
	$res2 = $tab2->select('id', 'n')->where("id like '2'")->lockExclusive()->execute();
	check_one($res2, '2', 222);

	// should return immediately
	$res2 = $tab2->select('id', 'n')->where("id like '3'")->lockShared()->execute();
	check_one($res2, '3', 3);

	// $session2 should block
	$res2 = $tab2->select('id', 'n')->where("id like '1'")->lockExclusive()->execute();
	$tab2->update()->set('n', 111)->where("id = '1'")->execute();
	$res2 = $tab2->select('id', 'n')->where("id like '1'")->lockExclusive()->execute();
	check_one($res2, '1', 111);

	$session1->commit(); // $session2 should unblock now
	$session2->commit();


	// test7: Exclusive write after Exclusive write

	$session1->startTransaction();
	$res1 = $tab1->select('id', 'n')->where("id like '1'")->lockExclusive()->execute();
	$tab1->update()->set('n', 1111)->where("id = '1'")->execute();
	$res1 = $tab1->select('id', 'n')->where("id like '1'")->lockExclusive()->execute();
	check_one($res1, '1', 1111);

	$session2->startTransaction();

	// should return immediately
	$res2 = $tab2->select('id', 'n')->where("id like '2'")->lockExclusive()->execute();
	//TODO: blocks execution, needs some async stuff to test it
//	$tab2->update()->set('n', 22222)->where("id = '2'")->execute();
//	$res2 = $tab2->select('id', 'n')->where("id like '2'")->lockExclusive()->execute();
//	check_one($res2, '2', 22222);

	// $session2 should block
	$res2 = $tab2->select('id', 'n')->where("id like '1'")->lockExclusive()->execute();
	//TODO: blocks execution, needs some async stuff to test it
//	$tab2->update()->set('n', 11111)->where("id = '1'")->execute();
//	$res2 = $tab2->select('id', 'n')->where("id like '1'")->lockExclusive()->execute();
//	check_one($res2, '1', 11111);

	$session1->commit(); // $session2 should unblock now
	$session2->commit();

	verify_expectations();
	print "done!\n";
?>
--CLEAN--
<?php
	require("connect.inc");
	clean_test_db($test_schema_name);
?>
--EXPECTF--
done!%A
