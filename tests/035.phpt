--TEST--
mysqlx Session Configuration Manager Part B
--SKIPIF--
--FILE--
<?php
        //This test should be executed immediately after 'Session Configuration Manager Part A'
	//In this way we can test if the sessions are properly stored on disk!
	require("connect.inc");
	$session = mysql_xdevapi\sessions();

        $sessions = $session->list();
	expect_eq( count($sessions), 2 );

        expect_false( $session->get( 'aaa' ) );
	$config1 = $session->get( 'bbb' );
	$config2 = $session->get( 'ccc' );

        expect_eq( $config1->getUri(), "mysqlx://Murphy:topsecret@tigerhost:666" );
	expect_eq( $config2->getUri(), "mysqlx://Arnold:@localhost:1001/?important=field&moreImportant=field" );

        expect_eq( $config1->getAppData('animal'), 'Dog');
	expect_eq( $config1->getAppData('stuff'), 'a lot');
	expect_eq( $config1->getAppData('alias'), 'Important');

        expect_eq( $config2->getAppData('cool'), 'Yes');
	expect_eq( $config2->getAppData('stuff'), 'not that much');
	expect_eq( $config2->getAppData('alias'), 'nope');

        try {
	    $nodeSession = mysql_xdevapi\getSession( $config1 );
	    $nodeSession = mysql_xdevapi\getSession( $config1, 'newpassword' );
	} catch( Exception $e ) {
	    test_step_failed();
	}

        $config1->setUri($connection_uri);
	try {
	    $nodeSession = mysql_xdevapi\getSession( $config1 );
	    $res = $nodeSession->executeSql("show variables like '%sql%'");
	    expect_true( count($res->fetchAll()) > 0 );
	} catch( Exception $e ) {
	    test_step_failed();
	}

        //Delete all the configuration data!
	expect_false( $session->delete( 'aaa' ) );
	expect_true( $session->delete( 'bbb' ) );
	expect_true( $session->delete( 'ccc' ) );

        expect_eq( $session->list(), 0 );

        print "done!".PHP_EOL;
	verify_expectations();
?>
--CLEAN--
<?php
        require("connect.inc");
?>
--EXPECTF--
%Adone!
