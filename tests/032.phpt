--TEST--
mysqlx Unix domain socket
--SKIPIF--
--FILE--
<?php
        require("connect.inc");
	create_test_db();

        try{
	        $uri = $scheme.'://'.$user.':'.$passwd.'@/tmp%2Fmysqlx.sock';
		$nodeSession = mysql_xdevapi\getSession($uri);
		$uri = $scheme.'://'.$user.':'.$passwd.'@(/tmp/./mysqlx.sock)/testx';
		$nodeSession = mysql_xdevapi\getSession($uri);
		$uri = $scheme.'://'.$user.':'.$passwd.'@(/tmp/mysqlx.sock)';
		$nodeSession = mysql_xdevapi\getSession($uri);
		$uri = $connection_uri;
		$nodeSession = mysql_xdevapi\getNodeSession($uri);
		$uri = $scheme.'://'.$user.':'.$passwd.'@(/tmp///mysqlx.sock)';
		$nodeSession = mysql_xdevapi\getSession($uri);
		$uri = $scheme.'://'.$user.':'.$passwd.'@/tmp%2F..%2Ftmp%2Fmysqlx.sock';
		$nodeSession = mysql_xdevapi\getSession($uri);
		test_step_ok();
	} catch( Exception $e ) {
	        test_step_failed();
	}
	try{
	        $uri = $scheme.'://'.$user.':'.$passwd.'@/tmp%2Fmysqlx.sockk/testx';
		$nodeSession = mysql_xdevapi\getSession($uri);
		expect_null( $nodeSession );
	} catch( Exception $e ) {
	        test_step_ok();
	}
	try{
	        $uri = $scheme.'://'.$user.':'.$passwd.'@(/tmp/mysqlx.socck)/testx';
		$nodeSession = mysql_xdevapi\getSession($uri);
		expect_null( $nodeSession );
	} catch( Exception $e ) {
	        test_step_ok();
	}
	try{
	        $uri = $scheme.'://'.$user.':'.$passwd.'@(/tmp2/mysqlx.sock)';
		$nodeSession = mysql_xdevapi\getSession($uri);
		expect_null( $nodeSession );
	} catch( Exception $e ) {
	        test_step_ok();
	}
	try{
	        $uri = $scheme.'://'.$user.':'.$passwd.'@';
		$nodeSession = mysql_xdevapi\getSession($uri);
		expect_null( $nodeSession );
	} catch( Exception $e ) {
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
