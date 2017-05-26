--TEST--
mysqlx UUID Validation
--SKIPIF--
--FILE--
<?php
        //Tweak the test by changing those values
	$number_of_insertion = 10;
	$number_of_session = 10;
	require("connect.inc");

function run_sessiont_test() {

        global $connection_uri;
	global $db;
	global $number_of_insertion;
	$nodeSession = mysql_xdevapi\getSession($connection_uri);

        $nodeSession->createSchema($db);
	$schema = $nodeSession->getSchema($db);

        $schema->createCollection("test_collection");
	$coll = $schema->getCollection("test_collection");

        $num_of_op = 10;
	for( $i=0 ; $i < $number_of_insertion ; $i++ )
	{
	    $coll->add(
	        ["name" => "Sakila".$i, "age" => $i, "job" => "Student".$i],
		["product" => "iphone".$i, "price" => $i, "qty" => $i],
		["car" => "BMW".$i, "qty" => $i, "id" => $i]
		)->execute();
	}

	$res = $coll->find()->fields('_id')->execute()->fetchAll();
	$ids = [];
	for( $i=0 ; $i < count($res) ; $i++ )
	{
	    array_push($ids, $res[$i]['_id']);
	}
	expect_eq( count($res), $number_of_insertion * 3 );
	$unique_ids = array_unique($ids);
	expect_eq( count($ids), count($unique_ids));

        // The node id and higher part of the timestamp should
	// be the same for all (only time_mid and time_low should change)
	$pattern = substr($ids[0],0,-10);
	for( $i=1 ; $i < count($ids) ; $i++ )
	{
	    expect_eq( substr($ids[$i],0,-10), $pattern );
	}

        //Return the initial part of the ID,
	//for each session it should be different
	return $pattern;
}

        $ids = [];
	for( $i=0 ; $i < $number_of_session ; $i++ )
	{
	    $id = run_sessiont_test();
	    array_push( $ids, $id );
	    clean_test_db();
	}
	$unique_ids = array_unique($ids);
	expect_eq( count($ids), count($unique_ids) );

        print "done!".PHP_EOL;
	verify_expectations();
?>
--CLEAN--
<?php
        require("connect.inc");
	clean_test_db();
?>
--EXPECTF--
done!%A
