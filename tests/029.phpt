--TEST--
mysqlx Field Metadata, empty rowset
--SKIPIF--
--FILE--
<?php

        require("connect.inc");
	$nodeSession = mysql_xdevapi\getSession($connection_uri);
	$nodeSession->executeSql("create database $db");
	$schema = $nodeSession->getSchema($db);

        $schema->createCollection("test_collection");
	$coll = $schema->getCollection("test_collection");

        $as_table = $schema->getCollectionAsTable("test_collection");
	$res = $as_table->select(['_id as a','doc as b'])->execute();
	$cols = $res->getColumns();

        $expected_data = [
	    ['_id','a',MYSQLI_TYPE_STRING,128,'utf8mb4_general_ci','utf8mb4'],
	    ['doc','b',MYSQLI_TYPE_JSON,4294967295,'binary','binary']
	];

        expect_eq(count($cols),2);
	if( count($cols) == 2 ) {
	    for( $i = 0; $i < 2; $i++ ) {
	        expect_eq($cols[$i]->getSchemaName(),$db);
		expect_eq($cols[$i]->getColumnName(), $expected_data[$i][0]);
		expect_eq($cols[$i]->getColumnLabel(),$expected_data[$i][1]);
		expect_eq($cols[$i]->getType(),       $expected_data[$i][2]);
		expect_eq($cols[$i]->getTableName(),'test_collection');
		expect_eq($cols[$i]->getTableLabel(),'test_collection');
		expect_eq($cols[$i]->getLength(),     $expected_data[$i][3]);
		expect_eq($cols[$i]->isNumberSigned(),false);
		expect_eq($cols[$i]->getFractionalDigits(),0);
		expect_eq($cols[$i]->getCollationName(),$expected_data[$i][4]);
		expect_eq($cols[$i]->getCharacterSetName(),$expected_data[$i][5]);
		expect_eq($cols[$i]->isPadded(),false);
	    }
	}

        verify_expectations();
	print "done!".PHP_EOL;
?>
--CLEAN--
<?php
        require("connect.inc");
	clean_test_db();
?>
--EXPECTF--
done!%A
