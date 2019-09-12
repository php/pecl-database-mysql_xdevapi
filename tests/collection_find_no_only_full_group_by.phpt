--TEST--
mysqlx Collection find, no 'only full group by'
--SKIPIF--
--FILE--
<?php
	require("connect.inc");

	$session = create_test_db();

	$schema = $session->getSchema($db);
	$coll = $schema->getCollection("test_collection");
	fill_db_collection($coll);

	$res = $session->sql('SELECT @@SESSION.sql_mode')->execute();
	$saved_sql_modes = $res->fetchAll()[0]['@@SESSION.sql_mode'];

	//This will disable 'only full group by'
	$new_sql_modes = 'STRICT_TRANS_TABLES,NO_ZERO_IN_DATE,NO_ZERO_DATE,ERROR_FOR_DIVISION_BY_ZERO,NO_ENGINE_SUBSTITUTION';
	$session->sql("SET SESSION sql_mode = '".$new_sql_modes."'")->execute();

	try {
		//This is going to work now but the representant of each group is indeterminate (probably the first available entry)
		$res = $coll->find()->fields(['name as n','age as a','job as j'])->groupBy('j')->limit(11)->execute();
		$data = $res->fetchAll();
		expect_eq(count($data),10);
		//For the next fetch, the output is unique
		$res = $coll->find()->fields(['COUNT(name) as cn', 'MAX(age) as ma', 'job as j'])->having('MAX(age) > 50')->groupBy('j')->execute();
		$data = $res->fetchAll();
		expect_eq(count($data),1);
		expect_eq($data[0]['ma'],'59');
		expect_eq($data[0]['j'],'Paninaro');
		expect_eq($data[0]['cn'],1);
	} catch( Exception $ex ) {
		test_step_failed();
	}

	$session->sql("SET SESSION sql_mode = '".$saved_sql_modes."'")->execute();
	//This shall fail now
	try {
		$res = $coll->find()->fields(['name as n','age as a','job as j'])->groupBy('j')->execute();
		test_step_failed();
	} catch(Exception $ex) {}
	print "verifying";
	verify_expectations();
	print "done!\n";
?>
--CLEAN--
<?php
	require("connect.inc");
	clean_test_db();
?>
--EXPECTF--
%Averifyingdone!%A
