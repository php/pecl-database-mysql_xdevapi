--TEST--
mysqlx Array or Object, 'contains' operator
--SKIPIF--
--INI--
error_reporting=0
--FILE--
<?php
	require("connect.inc");

	$session = create_test_db();

	$schema = $session->getSchema($db);
	$coll = $schema->getCollection("test_collection");

	$coll->add('{"_id": "1", "name": "Marco",
		"jobs": [{"title":"Mangiatore","Salary":1000},{"title":"Ciarlatano","Salary":12000}],
		"hobby": ["Spavare","Soffiare Minestrine"], "code":0}')->execute();

	$coll->add('{"_id": "2", "name": "Lucrezia",
		"jobs": [{"title":"Urlatrice","Salary":2000},{"title":"Parlatrice","Salary":3400}],
		"hobby": ["Cucinare","Guidare auto sportive","Cavalcare"], "code":1}')->execute();

	$coll->add('{"_id": "3", "name": "Ciro",
		"jobs": [{"title":"Dentista","Salary":2400},{"title":"Autista","Salary":400},{"title":"Spavatore","Salary":3399}],
		"hobby": ["PS4","Spavare","Cavalcare"], "code":5}')->execute();

	$coll->add('{"_id": "4", "name": "Lucio",
		"jobs": [{"title":"Cartolaio","Salary":200},{"title":"Barista","Salary":400},{"title":"Buttafuori","Salary":122}],
		"hobby": ["nessuno"], "code":3}')->execute();

	$coll->add('{"_id": "5", "name": "Lucio",
		"jobs": [{"title":"Spavatore","Salary":1200},{"title":"Mangiatore","Salary":1400},{"title":"Spazzino","Salary":499}],
		"hobby": ["PS4","Burattinaio","Fachiro","Calciatore"], "code":6}')->execute();


	$res = $coll->find('name IN ("Marco","Lucio")')->execute()->fetchAll();
	expect_eq(count($res),3);
	expect_eq_id($res[0]["_id"],1);
	expect_eq_id($res[1]["_id"],4);
	expect_eq_id($res[2]["_id"],5);

	$res = $coll->find('name NOT IN ("Marco","Lucio")')->execute()->fetchAll();
	expect_eq(count($res),2);
	expect_eq_id($res[0]["_id"],2);
	expect_eq_id($res[1]["_id"],3);

	$res = $coll->find('name NOT IN ("Marco","Lucio")')->sort('_id DESC')->execute()->fetchAll();
	expect_eq(count($res),2);
	expect_eq_id($res[0]["_id"],3);
	expect_eq_id($res[1]["_id"],2);

	$res = $coll->find('_id IN ["1","3","4"]')->execute()->fetchAll();
	expect_eq(count($res),3);
	expect_eq_id($res[0]["_id"],1);
	expect_eq_id($res[1]["_id"],3);
	expect_eq_id($res[2]["_id"],4);

	$res = $coll->find('_id NOT IN ["1","3","4"]')->execute()->fetchAll();

	expect_eq(count($res),2);
	expect_eq_id($res[0]["_id"],2);
	expect_eq_id($res[1]["_id"],5);

	$res = $coll->find('{"title":"Spavatore"} IN jobs')->execute()->fetchAll();
	expect_eq(count($res),2);
	expect_eq_id($res[0]["_id"],3);
	expect_eq_id($res[1]["_id"],5);

	$res = $coll->find('{"title":"Spavatore"} NOT IN jobs')->execute()->fetchAll();
	expect_eq(count($res),3);
	expect_eq_id($res[0]["_id"],1);
	expect_eq_id($res[1]["_id"],2);
	expect_eq_id($res[2]["_id"],4);

	$res = $coll->find('(1>5) IN (true,false) && {"title":"Spavatore"} NOT IN jobs && CAST(_id AS SIGNED) > 2')->execute()->fetchAll();
	expect_eq(count($res),1);
	expect_eq_id($res[0]["_id"],4);

	$coll->add('{"_id": "6", "name": "Gianfranco",
		"jobs": [{"title":"Programmatore","Salary":3200},{"title":"Cartolaio","Salary":340}],
		"hobby": ["Cavalcare","Programmare","Cucinare"], "code":1}')->execute();

	$coll->add('{"_id": "7", "name": "Magalli",
		"jobs": [{"title":"Spavatore","Salary":5200},{"title":"Programmatore","Salary":3405},{"title":"Contadino","Salary":1900}],
		"hobby": ["Spavare","Dipingere"], "code":2}')->execute();

	$res = $coll->find('"Spavatore" IN jobs[*].title AND "Spavare" IN hobby')->execute()->fetchAll();
	expect_eq(count($res), 2);
	expect_eq_id($res[0]["_id"],3);
	expect_eq_id($res[1]["_id"],7);

	$res = $coll->find('("Spavatore" IN jobs[*].title OR "Mangiatore" IN jobs[*].title ) AND 12000 IN jobs[*].Salary')->execute()->fetchAll();
	expect_eq(count($res), 1);
	expect_eq_id($res[0]["_id"],1);

	$res = $coll->find('true IN [(1>5), !(false), (true || false), (false && true)] AND CAST(_id AS SIGNED) > 5')->execute()->fetchAll();
	expect_eq(count($res), 2);
	expect_eq_id($res[0]["_id"],6);
	expect_eq_id($res[1]["_id"],7);

	$res = $coll->find('true IN [1-5/2*2 < 3-2/1*2] AND $.code > CAST($._id AS SIGNED)')->execute()->fetchAll();
	expect_eq(count($res), 2);
	expect_eq_id($res[0]["_id"],3);
	expect_eq_id($res[1]["_id"],5);
	expect_eq($res[0]["code"],5);
	expect_eq($res[1]["code"],6);

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
