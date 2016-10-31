--TEST--
xmysqlnd collection modify/set/unset
--SKIPIF--
--INI--
error_reporting=0
--FILE--
<?php
        require_once("connect.inc");

	$test = "000000000";

        $nodeSession = mysql_xdevapi\getNodeSession($host, $user, $passwd);

function verify_doc(&$doc,$name,$job,$age){
    $result = (strpos($doc,$name) != false);
    $result = ($result && (strpos($doc,$job) != false));
    $result = ($result && (strpos($doc,$age) != false));
    return $result;
}

        $nodeSession->createSchema("test_schema");
        $schema = $nodeSession->getSchema("test_schema");

        $schema->createCollection("test_collection");
        $coll = $schema->getCollection("test_collection");

        $coll->add('{"name": "Sakila", "age": 15, "job": "Programmer"}')->execute();
        $coll->add('{"name": "Sakila", "age": 17, "job": "Singer"}')->execute();
        $coll->add('{"name": "Sakila", "age": 18, "job": "Student"}')->execute();
        $coll->add('{"name": "Susanne", "age": 24, "job": "Plumber"}')->execute();
        $coll->add('{"name": "Mike", "age": 39, "job": "Manager"}')->execute();

	$coll->modify('name like "Sakila"')->set("job","Unemployed")->execute();

	$res = $coll->find('name like "Sakila"')->execute();
	$data = $res->fetchAll();

	for($i = 0;$i < count($data);$i++){
	     if(verify_doc($data[$i]['doc'],'Sakila','Unemployed','15'))
		$test[0] ='1';
	    if(verify_doc($data[$i]["doc"],'Sakila','Unemployed','17'))
		$test[1] ='1';
	    if(verify_doc($data[$i]['doc'],'Sakila','Unemployed','18'))
		$test[2] ='1';
	}


	$coll->modify('job like "Plumber"')->unset(["age","name"])->execute();
	$coll->modify('job like "Plumber"')->set("name","Artur")->set("age",49)->execute();

	$res = $coll->find('job like "Plumber"')->execute();
	$data = $res->fetchAll();

	for($i = 0;$i < count($data);$i++){
	    if(verify_doc($data[$i]["doc"],"Artur","Plumber","49"))
		$test[3] = "1";
	}

	$coll->modify('job like "nojob"')->set("name","Martin")->execute();
	$coll->modify('name like "Sakila"')->unset(["crap1","crap2"])->execute();

	$res = $coll->find()->execute();
	$data = $res->fetchAll();

	for($i = 0;$i < count($data);$i++){
	    if(verify_doc($data[$i]["doc"],"Sakila","Unemployed","15"))
		$test[4] = "1";
	    if(verify_doc($data[$i]["doc"],"Sakila","Unemployed","17"))
		$test[5] = "1";
	    if(verify_doc($data[$i]["doc"],"Sakila","Unemployed","18"))
		$test[6] = "1";
	    if(verify_doc($data[$i]["doc"],"Artur","Plumber","49"))
		$test[7] = "1";
	    if(verify_doc($data[$i]["doc"],"Mike","Manager","39"))
		$test[8] = "1";
	}

	$nodeSession->dropSchema("test_schema");

	var_dump($test);
        print "done!\n";
?>
--EXPECTF--
%s(9) "111111111"
done!
%a
