--TEST--
mysqlx save-points
--SKIPIF--
--INI--
error_reporting=0
--FILE--
<?php
        require_once("connect.inc");

        clean_test_db();
	$nodeSession = create_test_db();
	$coll = $nodeSession->getSchema('testx')->getCollection('test_collection');
	expect_true( null != $coll );

function fetch_and_verify( $num_of_docs ) {
        global $coll;
	$data = $coll->find()->execute()->fetchAll();
	if( 0 < $num_of_docs ) {
	    expect_eq( count( $data ) , $num_of_docs );
	    for( $i = 1 ; $i <= count( $data ) ; $i++ ) {
	        expect_eq( $data[$i-1]["test".( $i*2 - 1) ], $i*2 - 1 );
		expect_eq( $data[$i-1]["test".( $i*2) ], $i*2 );
	    }
	} else {
	    expect_false( $data );
	}
}

        /* 1th scenario */
	$nodeSession->startTransaction();
	$sp1 = $nodeSession->setSavepoint();
	expect_true( 0 < strlen( $sp1 ) );
	$nodeSession->rollbackTo( $sp1 ); //Raise error if sp_name do not exist
	$nodeSession->rollback();
	fetch_and_verify( 0 );

        /* 2th Scenario */
	$nodeSession->startTransaction();
	$coll->add( '{"test1":1, "test2":2}' )->execute();
	$sp1 = $nodeSession->setSavepoint();
	expect_true( 0 < strlen( $sp1 ) );
	$coll->add( '{"test3":3, "test4":4}' )->execute();
	fetch_and_verify( 2 );
	$nodeSession->rollbackTo( $sp1 );
	fetch_and_verify( 1 );
	$nodeSession->rollback();

        /* 3th Scenario */
	$nodeSession->startTransaction();
	$coll->add( '{"test1":1, "test2":2}' )->execute();
	$sp1 = $nodeSession->setSavepoint();
	expect_true( 0 < strlen( $sp1 ) );
	$coll->add( '{"test3":3, "test4":4}' )->execute();
	fetch_and_verify( 2 );
	$nodeSession->releaseSavepoint( $sp1 );
	fetch_and_verify( 2 );
	$nodeSession->rollback();

        /* 4th Scenario */
	$nodeSession->startTransaction();
	$coll->add( '{"test1":1, "test2":2}' )->execute();
	$sp1 = $nodeSession->setSavepoint( 'mysavepoint1' );
	expect_eq( $sp1, 'mysavepoint1' );
	$coll->add( '{"test3":3, "test4":4}' )->execute();
	fetch_and_verify( 2 );
	$nodeSession->rollbackTo( 'mysavepoint1' );
	fetch_and_verify( 1 );
	$nodeSession->rollback();

        /* 5th Scenario */
	$nodeSession->startTransaction();
	$coll->add( '{"test1":1, "test2":2}' )->execute();
	$sp1 = $nodeSession->setSavepoint( 'mysavepoint1' );
	expect_eq( $sp1, 'mysavepoint1' );
	$coll->add( '{"test3":3, "test4":4}' )->execute();
	fetch_and_verify( 2 );
	$nodeSession->releaseSavepoint( 'mysavepoint1' );
	fetch_and_verify( 2 );
	$nodeSession->rollback();

        /* 6th */
	$nodeSession->startTransaction();
	try{
	        $nodeSession->setSavepoint( ' ' );
		test_step_failed();
	} catch( Exception $e ) {
	        test_step_ok();
	}
	try{
	        $nodeSession->setSavepoint( '_' );
		test_step_failed();
	} catch( Exception $e ) {
	        test_step_ok();
	}
	try{
	        $nodeSession->setSavepoint( '-' );
		test_step_failed();
	} catch( Exception $e ) {
	        test_step_ok();
	}
	try{
	        $nodeSession->setSavepoint( 'mysp+' );
		test_step_failed();
	} catch( Exception $e ) {
	        test_step_ok();
	}
	try{
	        $nodeSession->setSavepoint( '3306' );
		test_step_failed();
	} catch( Exception $e ) {
	        test_step_ok();
	}
	try{
	        $nodeSession->setSavepoint( 'mysql3306' );
		test_step_ok();
	} catch( Exception $e ) {
	        test_step_failed();
	}
	try{
	        $nodeSession->releaseSavepoint( 'invalid ');
		test_step_failed();
	} catch( Exception $e ) {
	        test_step_ok();
	}
	try{
	        $nodeSession->rollbackTo( 'invalid ');
		test_step_failed();
	} catch( Exception $e ) {
	        test_step_ok();
	}
	$nodeSession->rollback();
	fetch_and_verify( 0 );

        /* 7th */
	$nodeSession->startTransaction();
	$coll->add( '{"test1":1, "test2":2}' )->execute();
	$spOrigin = $nodeSession->setSavepoint();
	$sp1 = $nodeSession->setSavepoint();
	expect_true( 0 < strlen( $sp1 ) );
	$coll->add( '{"test3":3, "test4":4}' )->execute();
	fetch_and_verify( 2 );
	$sp2 = $nodeSession->setSavepoint( $sp1 );
	expect_eq( $sp1 , $sp2 );
	$coll->add( '{"test5":5, "test6":6}' )->execute();
	fetch_and_verify( 3 );
	$sp3 = $nodeSession->setSavepoint( $sp1 );
	expect_eq( $sp1 , $sp3 );
	$coll->add( '{"test7":7, "test8":8}' )->execute();
	fetch_and_verify( 4 );
	$nodeSession->rollbackTo( $sp2 );
	fetch_and_verify( 3 );
	$nodeSession->rollbackTo( $spOrigin );
	fetch_and_verify( 1 );
	$nodeSession->rollback();
	fetch_and_verify( 0 );

        /* 8th */
	$nodeSession->startTransaction();
	$sp1 = $nodeSession->setSavepoint();
	$coll->add( '{"test1":1, "test2":2}' )->execute();
	$nodeSession->releaseSavepoint( $sp1 );
	try{
	        $nodeSession->releaseSavepoint( $sp1 );
		test_step_failed();
	} catch( Exception $e ){
	        test_step_ok();
	}
	fetch_and_verify( 1 );
	$nodeSession->rollback();
	fetch_and_verify( 0 );
	try{
	        $nodeSession->releaseSavepoint( $sp1 );
		test_step_failed();
	} catch( Exception $e ){
	        test_step_ok();
	}

        /* 9th */
	$nodeSession->startTransaction();
	$coll->add( '{"test1":1, "test2":2}' )->execute();
	$sp1 = $nodeSession->setSavepoint( );
	$coll->add( '{"test3":3, "test4":4}' )->execute();
	fetch_and_verify( 2 );
	$nodeSession->commit();
	try{
	        $nodeSession->rollbackTo( $sp1 );
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
	//clean_test_db();
?>
--EXPECTF--
done!%A
