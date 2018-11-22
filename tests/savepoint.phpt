--TEST--
mysqlx save-points
--SKIPIF--
--INI--
error_reporting=0
--FILE--
<?php
	require_once("connect.inc");
	$session = create_test_db();

	$coll = $session->getSchema($db)->getCollection( $test_collection_name );
	expect_true( null != $coll );

function fetch_and_verify( $num_of_docs ) {
	global $coll;
	$data = $coll->find()->execute()->fetchAll();
	if( 0 < $num_of_docs ) {
		expect_eq( count( $data ) , $num_of_docs );
		if( count( $data ) == $num_of_docs ) {
			for( $i = 1 ; $i <= count( $data ) ; $i++ ) {
				expect_eq( $data[$i-1]["test".( $i*2 - 1) ], $i*2 - 1 );
				expect_eq( $data[$i-1]["test".( $i*2) ], $i*2 );
			}
		}
	} else {
		expect_empty_array( $data );
	}
}

	/* 1th scenario */
	$session->startTransaction();
	$sp1 = $session->setSavepoint();
	expect_true( 0 < strlen( $sp1 ) );
	$session->rollbackTo( $sp1 ); //Raise error if sp_name do not exist
	$session->rollback();
	fetch_and_verify( 0 );

	/* 2th Scenario */
	$session->startTransaction();
	$coll->add( '{"test1":1, "test2":2}' )->execute();
	$sp1 = $session->setSavepoint();
	expect_true( 0 < strlen( $sp1 ) );
	$coll->add( '{"test3":3, "test4":4}' )->execute();
	fetch_and_verify( 2 );
	$session->rollbackTo( $sp1 );
	fetch_and_verify( 1 );
	$session->rollback();

	/* 3th Scenario */
	$session->startTransaction();
	$coll->add( '{"test1":1, "test2":2}' )->execute();
	$sp1 = $session->setSavepoint();
	expect_true( 0 < strlen( $sp1 ) );
	$coll->add( '{"test3":3, "test4":4}' )->execute();
	fetch_and_verify( 2 );
	$session->releaseSavepoint( $sp1 );
	fetch_and_verify( 2 );
	$session->rollback();

	/* 4th Scenario */
	$session->startTransaction();
	$coll->add( '{"test1":1, "test2":2}' )->execute();
	$sp1 = $session->setSavepoint( 'mysavepoint1' );
	expect_eq( $sp1, 'mysavepoint1' );
	$coll->add( '{"test3":3, "test4":4}' )->execute();
	fetch_and_verify( 2 );
	$session->rollbackTo( 'mysavepoint1' );
	fetch_and_verify( 1 );
	$session->rollback();

	/* 5th Scenario */
	$session->startTransaction();
	$coll->add( '{"test1":1, "test2":2}' )->execute();
	$sp1 = $session->setSavepoint( 'mysavepoint1' );
	expect_eq( $sp1, 'mysavepoint1' );
	$coll->add( '{"test3":3, "test4":4}' )->execute();
	fetch_and_verify( 2 );
	$session->releaseSavepoint( 'mysavepoint1' );
	fetch_and_verify( 2 );
	$session->rollback();

	/* 6th */
	$session->startTransaction();
	try{
		$session->setSavepoint( ' ' );
		test_step_ok();
	} catch( Exception $e ) {
		test_step_failed();
	}
	try{
		$session->setSavepoint( '_' );
		test_step_ok();
	} catch( Exception $e ) {
		test_step_failed();
	}
	try{
		$session->setSavepoint( '-' );
		test_step_ok();
	} catch( Exception $e ) {
		test_step_failed();
	}
	try{
		$session->setSavepoint( 'mysp+' );
		test_step_ok();
	} catch( Exception $e ) {
		test_step_failed();
	}
	try{
		$session->setSavepoint( '3306' );
		test_step_ok();
	} catch( Exception $e ) {
		test_step_failed();
	}
	try{
		$session->setSavepoint( 'mysql3306' );
		test_step_ok();
	} catch( Exception $e ) {
		test_step_failed();
	}
	try{
		$session->releaseSavepoint( 'invalid ');
		test_step_failed();
	} catch( Exception $e ) {
		test_step_ok();
	}
	try{
		$session->rollbackTo( 'invalid ');
		test_step_failed();
	} catch( Exception $e ) {
		test_step_ok();
	}
	$session->rollback();
	fetch_and_verify( 0 );

	/* 7th */
	$session->startTransaction();
	$coll->add( '{"test1":1, "test2":2}' )->execute();
	$spOrigin = $session->setSavepoint();
	$sp1 = $session->setSavepoint();
	expect_true( 0 < strlen( $sp1 ) );
	$coll->add( '{"test3":3, "test4":4}' )->execute();
	fetch_and_verify( 2 );
	$sp2 = $session->setSavepoint( $sp1 );
	expect_eq( $sp1 , $sp2 );
	$coll->add( '{"test5":5, "test6":6}' )->execute();
	fetch_and_verify( 3 );
	$sp3 = $session->setSavepoint( $sp1 );
	expect_eq( $sp1 , $sp3 );
	$coll->add( '{"test7":7, "test8":8}' )->execute();
	fetch_and_verify( 4 );
	$session->rollbackTo( $sp2 );
	fetch_and_verify( 3 );
	$session->rollbackTo( $spOrigin );
	fetch_and_verify( 1 );
	$session->rollback();
	fetch_and_verify( 0 );

	/* 8th */
	$session->startTransaction();
	$sp1 = $session->setSavepoint();
	$coll->add( '{"test1":1, "test2":2}' )->execute();
	$session->releaseSavepoint( $sp1 );
	try{
		$session->releaseSavepoint( $sp1 );
		test_step_failed();
	} catch( Exception $e ){
		test_step_ok();
	}
	fetch_and_verify( 1 );
	$session->rollback();
	fetch_and_verify( 0 );
	try{
		$session->releaseSavepoint( $sp1 );
		test_step_failed();
	} catch( Exception $e ){
		test_step_ok();
	}

	/* 9th */
	$session->startTransaction();
	$coll->add( '{"test1":1, "test2":2}' )->execute();
	$sp1 = $session->setSavepoint( );
	$coll->add( '{"test3":3, "test4":4}' )->execute();
	fetch_and_verify( 2 );
	$session->commit();
	try{
		$session->rollbackTo( $sp1 );
		test_step_failed();
	} catch( Exception $e ){
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
