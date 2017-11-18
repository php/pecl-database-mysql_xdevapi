<?php
        require_once("connect.inc");

        create_test_db();
	fill_db_table();

        //[ URI, Expected code ]
	$uri_string = [
	    [ $scheme.'://user:password@localhost/?'.$disable_ssl_opt       ,1045 ],
	    [ $scheme.'://'.$user.':password@localhost/?'.$disable_ssl_opt  ,1045 ],
	    [ $scheme.'://'.$user.':'.$passwd.'@fakehost/?'.$disable_ssl_opt,2002 ],
	    [ $scheme.'://:'.$passwd.'@fakehost/?'.$disable_ssl_opt         ,2002 ],
	    [ '//'.$user.':'.$passwd.'@'.$host.':19999/?'.$disable_ssl_opt  ,2002 ],
	    [ '//user:password@localhost/?'.$disable_ssl_opt                ,1045]
	];

        for( $i = 0 ; $i < count($uri_string) ; $i++ ) {
	    try {
	            $nodeSession = mysql_xdevapi\getSession($uri_string[$i][0]);
		     expect_null( $nodeSession );
	    } catch(Exception $e) {
	            expect_eq($e->getCode(), $uri_string[$i][1]);
	    }
	}

        try {
	        $uri = '//'.$user.':'.$passwd.'@'.$host.':'.$port.'/?'.$disable_ssl_opt;
		$nodeSession = mysql_xdevapi\getSession($uri);

		$nodeSession = mysql_xdevapi\getSession($scheme.':'.$uri);
	} catch(Exception $e) {
	        test_step_failed();
	}

        //test IPv6
	try {
	        $uri = $scheme.'://'.$user.':'.$passwd.'@'.'[::1]:'.$port.'/?'.$disable_ssl_opt;
		$nodeSession = mysql_xdevapi\getSession($uri);
	} catch(Exception $e) {
	        print $e->getCode()." : ".$e->getMessage().PHP_EOL;
	        test_step_failed();
	}

	//Verify SSL options
	$basic_uri = $scheme.'://'.$user.':'.$passwd.'@'.$host.':'.$port;
	$wrong_ssl = $basic_uri.'/?ssl-mode=disabled&ssl-mode=verify_ca&ssl-default';
	try {
	        $nodeSession = mysql_xdevapi\getSession($wrong_ssl);
		test_step_failed();
	} catch(Exception $e) {
	        expect_eq( $e->getCode(), 10033 );
	}
	$wrong_ssl = $basic_uri.'/?ssl-mode=disabled&ssl-ca=/path/to/ca&ssl-default';
	try {
	        $nodeSession = mysql_xdevapi\getSession($wrong_ssl);
		test_step_failed();
	} catch(Exception $e) {
	        expect_eq( $e->getCode(), 10033 );
	}
	try {
	        $nodeSession = mysql_xdevapi\getSession($basic_uri);
		expect_null( $nodeSession );
	} catch(Exception $e) {
	        test_step_failed();
	}

        verify_expectations();
	print "done!\n";
?>
