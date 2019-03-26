--TEST--
mysqlx connection test / URI string
--SKIPIF--
--INI--
error_reporting=0
default_socket_timeout=1
--FILE--
<?php
	require_once("connect.inc");

	create_test_db();
	fill_db_table();

	//[ URI, Expected code ]
	$uri_string = [
		[ $scheme.'://user:password@localhost/?'.$disable_ssl_opt       ,10054 ],
		[ $scheme.'://'.$user.':password@localhost/?'.$disable_ssl_opt  ,10054 ],
		[ $scheme.'://'.$user.':'.$passwd.'@fakehost/?'.$disable_ssl_opt,2002 ],
		[ $scheme.'://:'.$passwd.'@fakehost/?'.$disable_ssl_opt         ,2002 ],
		[ '//'.$user.':'.$passwd.'@'.$host.':19999/?'.$disable_ssl_opt  ,2002 ],
		[ '//user:password@localhost/?'.$disable_ssl_opt                ,10054]
	];

	for( $i = 0 ; $i < count($uri_string) ; $i++ ) {
		try {
			$session = mysql_xdevapi\getSession($uri_string[$i][0]);
			expect_null( $session, $uri_string[$i][0] );
		} catch(Exception $e) {
			expect_eq($e->getCode(), $uri_string[$i][1]);
		}
	}

	try {
		$uri = '//'.$user.':'.$passwd.'@'.$host.':'.$port.'/?'.$disable_ssl_opt;
		$session = mysql_xdevapi\getSession($uri);

		$session = mysql_xdevapi\getSession($scheme.':'.$uri);
	} catch(Exception $e) {
		test_step_failed();
	}

	//test IPv6
	try {
		$uri = $scheme.'://'.$user.':'.$passwd.'@'.'[::1]:'.$port.'/?'.$disable_ssl_opt;
		$session = mysql_xdevapi\getSession($uri);
	} catch(Exception $e) {
		print $e->getCode()." : ".$e->getMessage().PHP_EOL;
		test_step_failed();
	}

	//Verify SSL options
	$basic_uri = $scheme.'://'.$user.':'.$passwd.'@'.$host.':'.$port;
	$wrong_ssl = $basic_uri.'/?ssl-mode=disabled&ssl-mode=verify_ca&ssl-default';
	try {
		$session = mysql_xdevapi\getSession($wrong_ssl);
		test_step_failed();
	} catch(Exception $e) {
		expect_eq( $e->getCode(), 10045, $e->getMessage() );
	}
	$wrong_ssl = $basic_uri.'/?ssl-mode=disabled&ssl-ca=/path/to/ca&ssl-default';
	try {
		$session = mysql_xdevapi\getSession($wrong_ssl);
		test_step_failed();
	} catch(Exception $e) {
		expect_eq( $e->getCode(), 10045, $e->getMessage() );
	}
	try {
		$session = mysql_xdevapi\getSession($basic_uri);
		expect_not_null( $session, $basic_uri );
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
