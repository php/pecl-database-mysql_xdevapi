--TEST--
mysqlx connection test / URI string
--SKIPIF--
--INI--
error_reporting=0
--FILE--
<?php
        require_once("connect.inc");

        create_test_db();
	fill_db_table();

        //[ URI, Expected code ]
	$uri_string = [
	    [ $scheme.'://user:password@localhost'       ,1045 ],
	    [ $scheme.'://'.$user.':password@localhost'  ,1045 ],
	    [ $scheme.'://'.$user.':'.$passwd.'@fakehost',2002 ],
	    [ $scheme.'://:'.$passwd.'@fakehost'         ,2002 ],
	    [ $connection_uri.':19999'                   ,2002 ],
	    [ '//user:password@localhost'                ,1045]
	];

        for( $i = 0 ; $i < count($uri_string) ; $i++ ) {
	    try {
	            $nodeSession = mysql_xdevapi\getNodeSession($uri_string[$i][0]);
		    test_step_failed();
	    } catch(Exception $e) {
	            expect_eq($e->getCode(), $uri_string[$i][1]);
	    }
	    try {
	            $nodeSession = mysql_xdevapi\getSession($uri_string[$i][0]);
		    test_step_failed();
	    } catch(Exception $e) {
	            expect_eq($e->getCode(), $uri_string[$i][1]);
	    }
	}

        try {
	        $uri = '//'.$user.':'.$passwd.'@'.$host;
		$nodeSession = mysql_xdevapi\getNodeSession($uri);
		$nodeSession = mysql_xdevapi\getSession($uri);

                $nodeSession = mysql_xdevapi\getNodeSession($scheme.':'.$uri);
		$nodeSession = mysql_xdevapi\getSession($scheme.':'.$uri);
	} catch(Exception $e) {
	        test_step_failed();
	}

        //test IPv6
	try {
	        $uri = $scheme.'://'.$user.':'.$passwd.'@'.'[::1]';
		$nodeSession = mysql_xdevapi\getNodeSession($uri);
		$nodeSession = mysql_xdevapi\getSession($uri);
	} catch(Exception $e) {
	        test_step_failed();
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
