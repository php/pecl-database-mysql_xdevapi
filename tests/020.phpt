--TEST--
mysqlx NodeCollection
--SKIPIF--
--FILE--
<?php
        require("connect.inc");

        $nodeSession = create_test_db();
	$schema = $nodeSession->getSchema($db);
	$coll = $schema->getCollection("test_collection");

        fill_db_collection($coll);

        expect_eq($coll->getName(),'test_collection');
	expect_eq($coll->name, 'test_collection');
	expect_true($coll->existsInDatabase());
	expect_eq($coll->count(), 16);

        try {
	        //This is not implemented yet
		$schema = $coll->getSchema();
	} catch(Exception $ex) {
	        test_step_failed();
	}

        try {
	        //This is not implemented yet
		$session = $coll->getSession();
	} catch(Exception $ex) {
	        test_step_failed();
	}

        $coll = $schema->getCollection("not_existing_collection");
	expect_eq($coll->getName(), 'not_existing_collection');
	expect_eq($coll->name, 'not_existing_collection');
	expect_false($coll->existsInDatabase());

        try {
	        $schema = $coll->count();
		test_step_failed();
	} catch(Exception $ex) {
	        test_step_ok();
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
