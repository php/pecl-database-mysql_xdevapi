--TEST--
mysqlx Unix domain socket
--SKIPIF--
--FILE--
<?php
        require("connect.inc");

        $valid_socket = $socket;
	if( $valid_socket == null ) {
	    /*
	     * Attempt to obtain the socket path
	     * from the server
	     */
	    $nodeSession = mysql_xdevapi\getSession($connection_uri);
	    $res = $nodeSession->executeSql("show variables like 'mysqlx_socket'");
	    $var = $res->fetchAll();
	    if( count( $var ) == 1 ) {
	        $valid_socket = $var[0]["Value"];
	    }
	    //last resort..
	    if( $valid_socket == null ) {
	        $valis_socket = '/tmp/mysqlx.sock';
	    }
	}

	$sock = substr( $valid_socket, 1 );

        //Prepare the test socket addresses
	$socket_1 = '/'.str_replace( "/", "%2F", $sock );
	$socket_2 = '/'.str_replace( "/", "/./", $sock );
	$socket_3 = '/'.str_replace( "/", "///", $sock );
	$tok = strtok($sock, "/");
	$socket_4 = '/'.$tok;
	$last_tok = $tok;
	while ($tok !== false) {
	    $tok = strtok("/");
	    if( $tok != false ) {
	        $socket_4 = $socket_4.'%2F.%2F'.$tok;
		$last_tok = $tok;
	    }
	}

        try{
	        $uri = $scheme.'://'.$user.':'.$passwd.'@'.$socket_1.'/?'.$disable_ssl_opt;
		$nodeSession = mysql_xdevapi\getSession($uri);
		$uri = $scheme.'://'.$user.':'.$passwd.'@('.$socket_2.')/testx/?'.$disable_ssl_opt;
		$nodeSession = mysql_xdevapi\getSession($uri);
		$uri = $scheme.'://'.$user.':'.$passwd.'@('.$valid_socket.')/?'.$disable_ssl_opt;
		$nodeSession = mysql_xdevapi\getSession($uri);
		$uri = $connection_uri;
		$nodeSession = mysql_xdevapi\getSession($uri);
		$uri = $scheme.'://'.$user.':'.$passwd.'@('.$socket_3.')/?'.$disable_ssl_opt;
		$nodeSession = mysql_xdevapi\getSession($uri);
		$uri = $scheme.'://'.$user.':'.$passwd.'@'.$socket_4.'/?'.$disable_ssl_opt;
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
?>
--EXPECTF--
done!%A
