--TEST--
mysqlx complex query
--SKIPIF--
--FILE--
<?php
	require("connect.inc");

	$session = create_test_db();

	fill_db_table();

	$schema = $session->getSchema($db);
	$table = $schema->getTable("test_table");

	$sel = $table->select(['age as age_group', 'count(name) as cnt'])->groupBy('age_group');
	$sel = $sel->where("age > 11 and 1 < 2 and 40 between 30 and 900");
	$sel = $sel->having('cnt > 1');
	$sel = $sel->orderBy('age_group desc');
	$res = $sel->limit(2)->offset(1)->execute();
	$data = $res->fetchAll();
	expect_eq(count($data), 2);
	expect_eq($data[0]['age_group'],15);
	expect_eq($data[0]['cnt'],2);
	expect_eq($data[1]['age_group'],14);
	expect_eq($data[1]['cnt'],2);

        //Make sure to have duplicated values
	fill_db_table();

        //groupBy with multiple arguments
	$sel = $table->select(['age','count(name) as cnt'])->groupBy('age','name')->execute();
	$data = $sel->fetchAll();
	expect_eq(count($data), 12);
	for( $i = 0 ; $i < 12 ; $i++ ) {
	    expect_eq($data[$i]['cnt'],2);
	}

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
