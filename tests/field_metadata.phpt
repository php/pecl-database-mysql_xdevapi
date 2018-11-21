--TEST--
mysqlx Field Metadata
--SKIPIF--
--INI--
error_reporting=0
--FILE--
<?php

	require("connect.inc");
	$session = mysql_xdevapi\getSession($connection_uri);
	$session->sql("create database $db")->execute();

function test_numeric_types() {
	global $session;
	global $db;
	$session->sql("create table $db.table1(a int, b tinyint, c smallint, d mediumint,
	e int unsigned, f tinyint unsigned,g smallint unsigned,h mediumint unsigned,
	i float,j double,k decimal,l float unsigned,m double unsigned,n decimal unsigned,
	o bit,p bigint,q bigint unsigned)")->execute();

	$schema = $session->getSchema($db);
	$table = $schema->getTable("table1");

	$session->sql("insert into $db.table1 values (1,1,1,1,1,1,1,1,1.1,1.1,1,1.1,1.1,1,1,1,1)")->execute();

	$res = $table->select(['a as aa','b as bb','c as cc','d as dd',
	'e as ee','f as ff','g as gg','h as hh',
	'i as ii','j as jj','k as kk','l as ll',
	'm as mm','n as nn','o as oo','p as pp',
	'q as qq'])->execute();
	$cols = $res->getColumns();
	$expected_data = [
		['a','aa',MYSQLX_TYPE_INT,11,true,0,false],
		['b','bb',MYSQLX_TYPE_TINY,4,true,0,false],
		['c','cc',MYSQLX_TYPE_SMALLINT,6,true,0,false],
		['d','dd',MYSQLX_TYPE_MEDIUMINT,9,true,0,false],
		['e','ee',MYSQLX_TYPE_INT,10,false,0,false],
		['f','ff',MYSQLX_TYPE_TINY,3,false,0,false],
		['g','gg',MYSQLX_TYPE_SMALLINT,5,false,0,false],
		['h','hh',MYSQLX_TYPE_MEDIUMINT,8,false,0,false],
		['i','ii',MYSQLX_TYPE_FLOAT,12,true,31,false],
		['j','jj',MYSQLX_TYPE_DOUBLE,22,true,31,false],
		['k','kk',MYSQLX_TYPE_DECIMAL,11,true,0,false],
		['l','ll',MYSQLX_TYPE_FLOAT,12,false,31,false],
		['m','mm',MYSQLX_TYPE_DOUBLE,22,false,31,false],
		['n','nn',MYSQLX_TYPE_DECIMAL,10,false,0,false],
		['o','oo',MYSQLX_TYPE_BIT,1,false,0,false],
		['p','pp',MYSQLX_TYPE_BIGINT,20,true,0,false],
		['q','qq',MYSQLX_TYPE_BIGINT,20,false,0,false]];
	expect_eq(count($cols),17);
	if( count($cols) == 17 ) {
		for( $i = 0; $i < 17; $i++ ) {
			expect_eq($cols[$i]->getSchemaName(),$db);
			expect_eq($cols[$i]->getColumnName(), $expected_data[$i][0]);
			expect_eq($cols[$i]->getColumnLabel(),$expected_data[$i][1]);
			expect_eq($cols[$i]->getType(),       $expected_data[$i][2]);
			expect_eq($cols[$i]->getTableName(),'table1');
			expect_eq($cols[$i]->getTableLabel(),'table1');
			expect_eq($cols[$i]->getLength(),     $expected_data[$i][3]);
			expect_eq($cols[$i]->isNumberSigned(),$expected_data[$i][4]);
			expect_eq($cols[$i]->getFractionalDigits(),$expected_data[$i][5]);
			expect_eq($cols[$i]->getCollationName(),null);
			expect_eq($cols[$i]->getCharacterSetName(),null);
			expect_eq($cols[$i]->isPadded(),$expected_data[$i][6]);
		}
	}
}

function test_collection() {
	global $session;
	global $db;
	$schema = $session->getSchema($db);
	$schema->createCollection("test_collection");
	$coll = $schema->getCollection("test_collection");
	$coll->add('{"_id":99, "name": "Ugp",                 "job": "Cavia"}')->execute();
	$coll->add('{"_id":98, "name": "Simone",              "job": "Cavia"}')->execute();
	$coll->add('{"_id":97, "name": "Matteo",              "job": "Cavia"}')->execute();

	$as_table = $schema->getCollectionAsTable("test_collection");
	$res = $as_table->select(['_id as a','doc as b'])->execute();
	$cols = $res->getColumns();
	$expected_data = [
		['_id','a',MYSQLX_TYPE_STRING,32,['binary'],['binary']],
		['doc','b',MYSQLX_TYPE_JSON,4294967295,'binary','binary']
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
			expect_eq($cols[$i]->isNumberSigned(),false, $i);
			expect_eq($cols[$i]->getFractionalDigits(),0);
			expect_eq_or_in($cols[$i]->getCollationName(),$expected_data[$i][4]);
			expect_eq_or_in($cols[$i]->getCharacterSetName(),$expected_data[$i][5]);
			expect_eq($cols[$i]->isPadded(),false, $i);
		}
	}
}


function test_other_types() {
	global $session;
	global $db;

	$session->sql("create table $db.table2 (a bit, b char(20) not null, c tinyint unsigned primary key, d decimal(20, 3),
e time,f datetime, g timestamp,h date,i set('1','2'),j enum('1','2'))")->execute();
	$session->sql("insert into $db.table2 values (1,'test',1,22,'3:33:22',
'1900-01-22 22:22:22','1971-01-01 00:00:01','9000-12-31','1','2')")->execute();

	$schema = $session->getSchema($db);
	$table = $schema->getTable("table2");
	$res = $table->select(['a as aa','b as bb','c as cc','d as dd',
'e as ee','f as ff','g as gg','h as hh',
'i as ii','j as jj'])->execute();

	$cols = $res->getColumns();
	expect_eq(count($cols), 10);
	$expected_data = [
		['a','aa',MYSQLX_TYPE_BIT,1,0,false,null,null,false],
		['b','bb',MYSQLX_TYPE_BYTES,[20, 80],0,false,['utf8mb4_0900_ai_ci', 'latin1_swedish_ci'],['utf8mb4', 'latin1'],true],
		['c','cc',MYSQLX_TYPE_TINY,3,0,false,null,null,false],
		['d','dd',MYSQLX_TYPE_DECIMAL,22,3,true,null,null,false],
		['e','ee',MYSQLX_TYPE_TIME,10,0,false,null,null,false],
		['f','ff',MYSQLX_TYPE_DATETIME,19,0,false,null,null,false],
		['g','gg',MYSQLX_TYPE_TIMESTAMP,19,0,false,null,null,false],
		['h','hh',MYSQLX_TYPE_DATE,10,0,false,null,null,false],
		['i','ii',MYSQLX_TYPE_SET,[0, 3, 12],0,false,['utf8mb4_0900_ai_ci', 'latin1_swedish_ci'],['utf8mb4', 'latin1'],false],
		['j','jj',MYSQLX_TYPE_ENUM,[0, 1, 4],0,false,['utf8mb4_0900_ai_ci', 'latin1_swedish_ci'],['utf8mb4', 'latin1'],false]
	];
	if( count($cols) == 10 ) {
		for( $i = 0 ; $i < 10 ; $i++ ) {
			expect_eq($cols[$i]->getSchemaName(),$db);
			expect_eq($cols[$i]->getTableName(),'table2');
			expect_eq($cols[$i]->getTableLabel(),'table2');
			expect_eq($cols[$i]->getColumnName(), $expected_data[$i][0]);
			expect_eq($cols[$i]->getColumnLabel(),$expected_data[$i][1]);
			expect_eq($cols[$i]->getType(),       $expected_data[$i][2]);
			expect_eq_or_in($cols[$i]->getLength(),     $expected_data[$i][3]);
			expect_eq($cols[$i]->getFractionalDigits(),$expected_data[$i][4]);
			expect_eq($cols[$i]->isNumberSigned(),$expected_data[$i][5]);
			expect_eq_or_in($cols[$i]->getCollationName(),$expected_data[$i][6]);
			expect_eq_or_in($cols[$i]->getCharacterSetName(),$expected_data[$i][7]);
			expect_eq($cols[$i]->isPadded(),$expected_data[$i][8]);
		}
	}
}

function test_geometries() {
	global $session;
	global $db;

	$session->sql("create table $db.table3 (name int not null primary key, b geometry)")->execute();
	$session->sql("insert into $db.table3 values (1, ST_GeomFromText(\"POINT(1 1)\"))")->execute();
	$session->sql("insert into $db.table3 values (2, ST_GeomFromText(\"MULTIPOLYGON(((5 0,15 25,25 0,15 5,5 0)),
((25 0,0 15,30 15,22 10,25 0)))\"))")->execute();

	$schema = $session->getSchema($db);
	$table = $schema->getTable("table3");
	$res = $table->select(['name as myname','b as bb'])->execute();

	$expected_data = [
		['name','myname',MYSQLX_TYPE_INT,11,0,true,null,null,false],
		['b','bb',MYSQLX_TYPE_GEOMETRY,[0, 4294967295],0,false,null,null,false],
	];
	$cols = $res->getColumns();
	expect_eq(count($cols), 2);
	for( $i = 0 ; $i < 2 ; $i++ ) {
		expect_eq($cols[$i]->getSchemaName(),$db);
		expect_eq($cols[$i]->getTableName(),'table3');
		expect_eq($cols[$i]->getTableLabel(),'table3');
		expect_eq($cols[$i]->getColumnName(), $expected_data[$i][0]);
		expect_eq($cols[$i]->getColumnLabel(),$expected_data[$i][1]);
		expect_eq($cols[$i]->getType(),       $expected_data[$i][2]);
		expect_eq_or_in($cols[$i]->getLength(),     $expected_data[$i][3]);
		expect_eq($cols[$i]->getFractionalDigits(),$expected_data[$i][4]);
		expect_eq($cols[$i]->isNumberSigned(),$expected_data[$i][5]);
		expect_eq($cols[$i]->getCollationName(),$expected_data[$i][6]);
		expect_eq($cols[$i]->getCharacterSetName(),$expected_data[$i][7]);
		expect_eq($cols[$i]->isPadded(),$expected_data[$i][8]);
	}
	if( count($cols) == 2 ) {

	}
}

	test_numeric_types();
	test_collection();
	test_geometries();
	test_other_types();

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
