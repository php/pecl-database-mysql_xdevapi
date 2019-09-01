--TEST--
mysqlx Flexible number of arguments
--SKIPIF--
--INI--
error_reporting=0
--FILE--
<?php
	require("connect.inc");

	$session = mysql_xdevapi\getSession($connection_uri);
	$session->sql("create database $db")->execute();
	$session->sql("create table $db.test_table(a text, b int, c text)")->execute();
	$schema = $session->getSchema($db);
	$table = $schema->getTable("test_table");

	$table->insert('a', 'b', 'c')->values(['a', 1,'a2'],
		['b', 2,'b2'],
		['c',3,'c2'])->execute();

	$table->insert('a', 'b')->values(['aa', 4],
		['bb', 5],
		['cc',6])->execute();

	$table->insert('a')->values(['aaa'],
		['bbb'],
		['ccc'],
		['ddd'])->execute();

	$res = $table->select('a as aa','b as bb','c as cc')->orderby('b desc','a desc');
	$data = $res->execute()->fetchAll();
	expect_eq(count($data), 10);
	expect_eq($data[0]['aa'],'cc');
	expect_eq($data[0]['bb'],6);
	expect_eq($data[9]['aa'],'aaa');
	expect_eq($data[9]['bb'],null);

	$res = $table->select('count(b) as bb')->groupBy('b')->orderBy('b asc');
	$data = $res->execute()->fetchAll();
	expect_eq(count($data),7);
	$expected_vals = [0,1,1,1,1,1,1];
	for($i = 0 ; $i < 7 ; $i++) {
		expect_eq($data[$i]['bb'],$expected_vals[$i]);
	}

	$table->update()->set('b',69)->limit(7)->orderby('b desc','a desc')->execute();
	$data = $table->select('a','b','c')->where("a like 'ddd'")->execute()->fetchAll();
	expect_eq(count($data),1);
	expect_eq($data[0]['b'],69);

	$table->delete()->limit(9)->orderby('a desc','b desc')->execute();
	$data = $table->select('a','b','c')->execute()->fetchAll();
	expect_eq(count($data),1);
	expect_eq($data[0]['a'],'a');
	expect_eq($data[0]['b'],69);
	expect_eq($data[0]['c'],'a2');

	$session->getSchema($db)->createCollection("test_collection");
	$coll = $schema->getCollection("test_collection");
	$coll->add('{"name": "Marco", "job": "zzzz"}',
		'{"_id": 2, "name": "Lonardo", "job": "zzzz"}',
		'{"_id": 3, "name": "Riccardo", "num" : 2}',
		'{"_id": 4, "name": "Carlotta", "num" : 3}',
		'{"name": "x", "age": 77, "job": "a"}',
		'{"name": "xx", "age": 76, "job": "aa"}',
		'{"name": "xxx", "age": 75, "job": "aaa"}',
		'{"name": "x", "age": 74, "job": "aaaa"}')->execute();

	$res = $coll->find()->sort('age desc','job desc','num desc')->execute();
	$data = $res->fetchAll();
	expect_eq(count($data),8);
	expect_eq($data[0]['age'],77);
	expect_eq($data[0]['job'],'a');
	expect_eq($data[0]['name'],'x');
	expect_eq_id($data[7]['_id'],3);
	expect_eq($data[7]['num'],2);
	expect_eq($data[7]['name'],'Riccardo');

	$coll->modify('1=1')->unset('job','age','num')->execute();
	$res = $coll->find()->sort('name desc')->execute();
	$data = $res->fetchAll();
	expect_eq(count($data),8);
	$expected_vals = ['xxx','xx','x','x','Riccardo','Marco','Lonardo','Carlotta'];
	for($i = 0 ; $i < 8 ; $i++ ) {
		expect_eq($data[$i]['name'],$expected_vals[$i]);
		expect_false(array_key_exists('job',$data[$i]));
		expect_false(array_key_exists('age',$data[$i]));
		expect_false(array_key_exists('num',$data[$i]));
		//expect_false($data[$i]['age']);
		//expect_false($data[$i]['num']);
	}

	$new_values = [
		1,null,2,null,3,null,4,null,
		0,'z',0,'y',0,'x',0,'w',
	];
	for($i = 0 ; $i < 8 ; $i++ ) {
		$tmp = $coll->modify("name like '".$expected_vals[$i]."'");
		if($new_values[$i * 2]!=0)
			$tmp->set('num',$new_values[$i * 2]);
		if($new_values[$i * 2 + 1]!=null)
			$tmp->set('str',$new_values[$i * 2 + 1]);
		$tmp->execute();
	}

	$coll->remove('TRUE')->limit(6)->sort('str desc','num desc')->execute();
	$res = $coll->find()->execute();
	$data = $res->fetchAll();
	expect_eq(count($data),2);
	expect_eq($data[0]['num'],2);
	expect_eq($data[0]['name'],'xx');
	expect_eq($data[1]['num'],1);
	expect_eq($data[1]['name'],'xxx');

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
