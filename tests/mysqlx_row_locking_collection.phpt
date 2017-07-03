--TEST--
mysqlx collection row locking
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

	$col1 = $schema1->createCollection($test_collection_name);

	$col1->add('{"id": "1", "n": 1}')->execute();
	$col1->add('{"id": "2", "n": 2}')->execute();
	$col1->add('{"id": "3", "n": 3}')->execute();

	$schema2 = $session2->getSchema($test_schema_name);
	$col2 = $schema2->getCollection($test_collection_name);

	// test1: Shared Lock

	$session1->startTransaction();
	$res1 = $col1->find("id like '1'")->lockShared()->execute();
	check_one($res1, '1', 1);

	$session2->startTransaction();

	// should return immediately
	$res2 = $col2->find("id like '2'")->lockShared()->execute();
	check_one($res2, '2', 2);

	$res2 = $col2->find("id like '1'")->lockShared()->execute();
	check_one($res2, '1', 1);

	$session1->rollback();
	$session2->rollback();


	// test2: Shared Lock after Exclusive

	$session1->startTransaction();
	$res1 = $col1->find("id like '1'")->lockExclusive()->execute();
	check_one($res1, '1', 1);

	$session2->startTransaction();

	// should return immediately
	$res2 = $col2->find("id like '2'")->lockShared()->execute();
	check_one($res2, '2', 2);

	// $session2 blocks
	$res2 = $col2->find("id like '1'")->lockShared()->execute();
	check_one($res2, '1', 1);

	$session1->rollback(); // $session2 should unblock now
	$session2->rollback();


	// test3: Exclusive after Shared

	$session1->startTransaction();
	$res1 = $col1->find("id in ('1', '3')")->lockShared()->execute();

	$session2->startTransaction();

	// should return immediately
	$res2 = $col2->find("id like '2'")->lockExclusive()->execute();
	check_one($res2, '2', 2);

	// should return immediately
	$res2 = $col2->find("id like '3'")->lockShared()->execute();
	check_one($res2, '3', 3);

	// $session2 should block
	$res2 = $col2->find("id like '1'")->lockExclusive()->execute();
	check_one($res2, '1', 1);

	$session1->rollback(); // $session2 should unblock now
	$session2->rollback();


	// test4: Exclusive after Exclusive

	$session1->startTransaction();
	$res1 = $col1->find("id like '1'")->lockExclusive()->execute();
	check_one($res1, '1', 1);

	$session2->startTransaction();

	// should return immediately
	$res2 = $col2->find("id like '2'")->lockExclusive()->execute();
	check_one($res2, '2', 2);

	// $session2 should block
	$res2 = $col2->find("id like '1'")->lockExclusive()->execute();
	check_one($res2, '1', 1);

	$session1->rollback(); // $session2 should unblock now
	$session2->rollback();


	// test5: Shared Lock read after Exclusive write

	$session1->startTransaction();

	$res1 = $col1->find("id in ('1', '2')")->lockExclusive()->execute();
	$col1->modify("id = '1'")->set('n', 11)->execute();
	$col1->modify("id = '2'")->set('n', 22)->execute();
	$res1 = $col1->find("id in ('1', '2')")->lockExclusive()->execute();
	check_one($res1, '1', 11);
	check_one($res1, '2', 22);

	$session2->startTransaction();

	// should return immediately
	$res2 = $col2->find("id like '2'")->lockShared()->execute();
	check_one($res2, '2', 2);

	// $session2 blocks
	$res2 = $col2->find("id like '1'")->lockShared()->execute();
	check_one($res2, '1', 1);

	$session1->commit(); // $session2 should unblock now
	$session2->commit();


	// test6: Exclusive write after Shared read

	$session1->startTransaction();
	$res1 = $col1->find("id in ('1', '3')")->lockShared()->execute();
	check_all($res1, ['1' => 11, '3' => 3]);

	$session2->startTransaction();

	// should return immediately
	$res2 = $col2->find("id like '2'")->lockExclusive()->execute();
	$col2->modify("id = '2'")->set('n', 222)->execute();
	$res2 = $col2->find("id like '2'")->lockExclusive()->execute();
	check_one($res2, '2', 222);

	// should return immediately
	$res2 = $col2->find("id like '3'")->lockShared()->execute();
	check_one($res2, '3', 3);

	// $session2 should block
	$res2 = $col2->find("id like '1'")->lockExclusive()->execute();
	$col2->modify("id = '1'")->set('n', 111)->execute();
	$res2 = $col2->find("id like '1'")->lockExclusive()->execute();
	check_one($res2, '1', 111);

	$session1->commit(); // $session2 should unblock now
	$session2->commit();


	// test7: Exclusive write after Exclusive write

	$session1->startTransaction();
	$res1 = $col1->find("id like '1'")->lockExclusive()->execute();
	$col1->modify("id = '1'")->set('n', 1111)->execute();
	$res1 = $col1->find("id like '1'")->lockExclusive()->execute();
	check_one($res1, '1', 1111);

	$session2->startTransaction();
	// should return immediately
	$res2 = $col2->find("id like '2'")->lockExclusive()->execute();
	//TODO: blocks execution, needs some async stuff to test it
//	$col2->modify("id = '2'")->set('n', 22222)->execute();
//	$res2 = $col2->find("id like '2'")->lockExclusive()->execute();
//	check_one($res2, '2', 22222);

	// $session2 should block
	$res2 = $col2->find("id like '1'")->lockExclusive()->execute();
	//TODO: blocks execution, needs some async stuff to test it
//	$col2->modify("id = '1'")->set('n', 11111)->execute();
//	$res2 = $col2->find("id like '1'")->lockExclusive()->execute();
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
