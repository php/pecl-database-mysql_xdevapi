--TEST--
mysqlx Client side failover
--SKIPIF--
--INI--
error_reporting=0
default_socket_timeout=1
--FILE--
<?php
	require_once("connect.inc");

	//[ URI, Expected code ]
	$uri_string = [
		[ $scheme.'://'.$user.':'.$passwd.'@[aa:bb:cc:dd]/db/?'.$disable_ssl_opt, 2002 ],
		[ $scheme.'://'.$user.':'.$passwd.'@[aaa,bbb,ccc]/db/?'.$disable_ssl_opt, 4001 ],
		[ $scheme.'://'.$user.':'.$passwd.'@[ [aaa:bbb:ccc], [xxx:yyy:zzz]]/db/?'.$disable_ssl_opt, 4001 ],
		[ $scheme.'://'.$user.':'.$passwd.'@[ [aaa:bbb:ccc], [xxx:yyy:zzz]:10 ,coolhost:69 ]/?'.$disable_ssl_opt, 4001 ],
		[ $scheme.'://'.$user.':'.$passwd.'@[(address= [aaa:bbb:ccc], priority = 3),[xxx:yyy:zzz]]/?'.$disable_ssl_opt, 4000 ],
		[ $scheme.'://'.$user.':'.$passwd.'@[(address=aaa:1,priority=2),(address=[aaa:bbb:ccc]:2,priority=3)]/db/?'.$disable_ssl_opt, 4001 ],
		[ $scheme.'://'.$user.':'.$passwd.'@[(address=[aaa:bbb:ccc],priority=1000),(address=superhost,priority=2)]/?'.$disable_ssl_opt, 4007 ],
		//Add failing URIs only BEFORE this point
		[ $scheme.'://'.$user.':'.$passwd.'@[[aa:bb:cc:dd],'.$host.':'.$port.']/db/?'.$disable_ssl_opt, 4001 ],
		[ $scheme.'://'.$user.':'.$passwd.'@[(address=aaa:1,priority=2),(address=bbb:2,priority=3),(address='.$host.':'.$port.',priority=1)]/?'.$disable_ssl_opt, 0 ],
		[ $scheme.'://'.$user.':'.$passwd.'@'.$host.':'.$port.'/?'.$disable_ssl_opt, 0 ],
	];

	for( $i = 0 ; $i < count($uri_string) ; $i++ ) {
		try {
			$session_uri = $uri_string[$i][0];
			$session = mysql_xdevapi\getSession($session_uri);
			if( $i > 6 ) {
				expect_true( $session != null, $session_uri );
			} else {
				expect_false( $session, $session_uri );
			}

		} catch(Exception $e) {
			expect_eq($e->getCode(), $uri_string[$i][1], $uri_string[$i][0]);
		}
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
