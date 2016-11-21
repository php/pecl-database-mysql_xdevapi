--TEST--
mysqlx collection remove
--SKIPIF--
--FILE--
<?php
	require("connect.inc");

	$nodeSession = create_test_db();
	$schema = $nodeSession->getSchema($db);
	$coll = $schema->getCollection("test_collection");

	fill_db_collection($coll);

	$coll->remove('age > :age_from and age < :age_to')->bind(['age_from' => 20, 'age_to' => 50])->limit(7)->execute();

	$coll->remove()->sort('age desc')->limit(2)->execute();
	$coll->modify('_id in (1,13,5,7)')->unset(['age'])->execute();
	$coll->remove('job in (\'Barista\', \'Programmatore\', \'Ballerino\', \'Programmatrice\')')->limit(5)->sort(['age desc', 'name asc'])->execute();

        $res = $coll->find()->execute()->fetchAll();
        expect_eq($res[0]['job'],'Programmatore');
        expect_eq($res[0]['name'],'Marco');
        expect_eq($res[1]['job'],'Programmatore');
        expect_eq($res[1]['name'],'Carlo');

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
