--TEST--
mysqlx getDocumentId
--SKIPIF--
--INI--
error_reporting=0
--FILE--
<?php
	require_once("connect.inc");

	$nodeSession = mysql_xdevapi\getNodeSession($connection_uri);
	$nodeSession->createSchema($db);
	$schema = $nodeSession->getSchema($db);

	$schema->createCollection("test_collection");
	$coll = $schema->getCollection("test_collection");

	$res = $coll->add('{"name": "Marco",      "age": 19, "job": "Programmatore", "_id":  "45" }')->execute();
	expect_eq($res->getDocumentId(),"45");
	$res = $coll->add('{"name": "Lonardo","_id": "  2  "  , "age": 59, "job": "Paninaro"}')->execute();
	expect_eq($res->getDocumentId(),"  2  ");
	$res = $coll->add('{"name": "Riccardo",   "age": 27, "job": "Cantante","_id":34}')->execute();
	expect_eq($res->getDocumentId(), "34");
	$res = $coll->add('{"name": "Carlotta",   "age": 23, "_id":     33.33    , "job": "Programmatrice"}')->execute();
	expect_eq($res->getDocumentId(), "33.33");
	$res = $coll->add('{"name": "Carlotta2",   "age": 23, "job": "Programmatrice2","_id":33.3354}')->execute();
	expect_eq($res->getDocumentId(),"33.3354");
	$res = $coll->add('{"_id":"34.23", "name": "Carlo",      "age": 25, "job": "Programmatore"}')->execute();
	expect_eq($res->getDocumentId(),"34.23");
	$res = $coll->add('{"_id":    "6", "name": "Mariangela", "age": 41, "job": "Programmatrice"}')->execute();
	expect_eq($res->getDocumentId(),"6");
//	$res = $coll->add('{"_id": "", "name": "Alfredo",    "age": 27, "job": "Programmatore"}')->execute();
//	expect_eq($res->getDocumentId(),"");
	$res = $coll->add('{"_id": "tes,t"  , "name": "Alessandra", "age": 15, "job": "Barista"}')->execute();
	expect_eq($res->getDocumentId(),"tes,t");
	$res = $coll->add('{"name": "Massimo",    "age": 22, "job": "Programmatore", "_id":   "  testtt ,"}')->execute();
	expect_eq($res->getDocumentId(),"  testtt ,");
	$res = $coll->add(["name" => "Sakila2", "age" => 18, "_id" =>     324,"job" => "Student"])->execute();
	expect_eq($res->getDocumentId(),"324");
	$res = $coll->add(["name" => "Sakila3", "age" => 18, "_id" => "3244","job" => "Student"])->execute();
	expect_eq($res->getDocumentId(),"3244");
	$res = $coll->add(["name" => "Sakila4", "age" => 18, "_id" => "ni,ce   n","job" => "Student"])->execute();
	expect_eq($res->getDocumentId(),"ni,ce   n");
	$res = $coll->add(["name" => "Sakila5", "age" => 18, "_id" => 3.1415,"job" => "Student"])->execute();
	expect_eq($res->getDocumentId(),"3.1415");
	$res = $coll->add(["name" => "Sakila6", "age" => 18, "_id" => "n   n","job" => "Student"])->execute();
	expect_eq($res->getDocumentId(),"n   n");
	$res = $coll->add(["name" => "Sakila7", "age" => 18, "_id" => "   55    ","job" => "Student"])->execute();
	expect_eq($res->getDocumentId(),"   55    ");
	try {
		$res = $coll->add(["name" => "Sakila8", "age" => 58, "_id" => "u78e4jcnjkd95uijh343d4ffgfdscdfer","job" => "Student2"])->execute();
		test_step_failed();
	} catch( Exception $ex ) {
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
