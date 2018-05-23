--TEST--
mysqlx simple expression
--SKIPIF--
--FILE--
<?php
	require("connect.inc");
        $session = create_test_db();

	$schema = $session->getSchema($db);
	$coll = $schema->getCollection("test_collection");

	fill_db_collection($coll);

        $expression = mysql_xdevapi\Expression("[age,job]");
        $res = $coll->find("age > 30")->fields($expression)->limit(3)->execute();
        $data = $res->fetchAll();

	expect_eq(count($data), 3 );
	expect_eq($data[0]["job"][0],47);
	expect_eq($data[1]["job"][0],31);
	expect_eq($data[2]["job"][0],37);

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
