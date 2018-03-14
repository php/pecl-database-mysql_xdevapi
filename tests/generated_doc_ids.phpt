--TEST--
mysqlx Collection
--SKIPIF--
--FILE--
<?php
	require("connect.inc");
	$ids = [];

function verify_monotonically_increasing_ids( $ids ) {
	for( $i = 1 ; $i < count( $ids ) ; $i++ ) {
		expect_true( $ids[ $i ] > $ids[ $i - 1 ] );
	}
}

function verify_single_id_and_add( $id ) {
	global $ids;
	expect_true( $id != null );
	if( $id != null ) {
		expect_eq( count( $id ), 1 );
		expect_eq( strlen($id[0]), 28 );
		array_push( $ids, $id[0] );
		verify_monotonically_increasing_ids( $ids );
	}
}

function verify_ids_and_add( $new_ids, $expected_count ) {
	global $ids;
	expect_true( $new_ids != null );
	if( $new_ids != null ) {
		expect_eq( count($new_ids), $expected_count );
		foreach( $new_ids as $id ) {
			expect_eq( strlen($id), 28 );
			array_push( $ids, $id );
		}
		verify_monotonically_increasing_ids( $ids );
	}
}

	$session = create_test_db();
	$schema = $session->getSchema($db);
	$coll = $schema->getCollection("test_collection");

	$res = $coll->add('{"name": "Marco",      "age": 19, "job": "Programmatore"}',
		'{"name": "Lonardo",    "age": 59, "job": "Paninaro"}',
		'{"name": "Riccardo",   "age": 27, "job": "Cantante"}',
		'{"name": "Carlotta",   "age": 23, "job": "Programmatrice"}')->execute();
	$ids = $res->getGeneratedIds();

	expect_eq( count( $ids ), 4 );
	verify_monotonically_increasing_ids( $ids );

	$res1 = $coll->add('{"name": "Massimo",    "age": 22, "job": "Programmatore"}')->execute();
	verify_single_id_and_add( $res1->getGeneratedIds(), $ids );
	$res2 = $coll->add('{"name": "Carlo",      "age": 37, "job": "Calciatore"}')->execute();
	verify_single_id_and_add( $res2->getGeneratedIds(), $ids );

	$res = $coll->add('{"_id":"1", "name": "Andrea",     "age": 58, "job": "Cantante"}',
				'{"_id":"2", "name": "Francesco",  "age": 40, "job": "Calciatore"}',
				'{"_id":"3", "name": "Dino",       "age": 75, "job": "Portiere"}')->execute();
	$ids_2 = $res->getGeneratedIds();
	expect_eq( count( $res->getGeneratedIds() ), 0 );

	$res = $coll->add('{"_id":"4", "name": "Alessandra", "age": 15, "job": "Barista"}',
		'{"name": "Massimo",    "age": 22, "job": "Programmatore"}',
		'{"_id":"5", "name": "Carlo",      "age": 37, "job": "Calciatore"}',
		'{"name": "Leonardo",   "age": 23, "job": "Programmatore"}')->execute();
	$ids_2 = $res->getGeneratedIds();

	expect_eq( count( $ids_2 ), 2 );
	verify_ids_and_add( $ids_2, 2 );
	expect_eq( count($ids), 8 );

	$res = $coll->add('{"name": "Lucia",      "age": 47, "job": "Barista"}')->execute();
	$ids_2 = $res->getGeneratedIds();
	verify_single_id_and_add( $ids_2 );
	$id_len = strlen( $ids_2[0] );
	$suffix = substr( $ids_2[0], $id_len - 8 );
	$next_suffix = dechex( 1 + hexdec($suffix) );
	$next_id = $ids_2[0];
	expect_true( strlen($next_id) > strlen($next_suffix) );
	if( strlen($next_id) > strlen($next_suffix) ) {
		$i = strlen( $next_id ) - 1;
		for($j = strlen($next_suffix) - 1 ; $j >= 0 ; $j--, $i-- ) {
			$next_id[$i] = $next_suffix[$j];
		}

		$res = $coll->add('{"_id":"'.$next_id.'", "name": "Beppe",      "age": 66, "job": "Comico"}')->execute();

		$ids_2 = $res->getGeneratedIds();
		expect_eq( count( $ids_2 ), 0 );
		try{
			$res = $coll->add('{"name": "Giulio",     "age": 29, "job": "Disoccupato"}')->execute();
			test_step_failed();
		} catch( Exception $e ) {
			test_step_ok();
		}
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
