--TEST--
mysqlx Client side failover
--SKIPIF--
--INI--
error_reporting=0
--FILE--
<?php
        require_once("connect.inc");

        //[ URI, Expected code ]
	$uri_string = [
	    [ $scheme.'://'.$user.':'.$passwd.'@[aa:bb:cc:dd]/db', 4001 ],
	    [ $scheme.'://'.$user.':'.$passwd.'@[aaa,bbb,ccc]/db', 4001 ],
	    [ $scheme.'://'.$user.':'.$passwd.'@[ [aaa:bbb:ccc], [xxx:yyy:zzz]]/db', 4001 ],
	    [ $scheme.'://'.$user.':'.$passwd.'@[ [aaa:bbb:ccc], [xxx:yyy:zzz]:10 ,coolhost:69 ]', 4001 ],
	    [ $scheme.'://'.$user.':'.$passwd.'@[(address= [aaa:bbb:ccc], priority = 3),[xxx:yyy:zzz]]', 4000 ],
	    [ $scheme.'://'.$user.':'.$passwd.'@[(address=aaa:1,priority=2),(address=[aaa:bbb:ccc]:2,priority=3)]/db', 4001 ],
	    [ $scheme.'://'.$user.':'.$passwd.'@[(address=[aaa:bbb:ccc],priority=1000),(address=superhost,priority=2)]', 4007 ],
	    //Add failing URIs only BEFORE this point
	    [ $scheme.'://'.$user.':'.$passwd.'@[[aa:bb:cc:dd],'.$host.']/db', 0 ],
	    [ $scheme.'://'.$user.':'.$passwd.'@[(address=aaa:1,priority=2),(address=bbb:2,priority=3),(address='.$host.',priority=1)]', 0 ],
	    [ $scheme.'://'.$user.':'.$passwd.'@'.$host.':'.$port, 0 ],
	];

        for( $i = 0 ; $i < count($uri_string) ; $i++ ) {
	    try {
	            $nodeSession = mysql_xdevapi\getSession($uri_string[$i][0]);
		    if( $i > 6 ) {
		        expect_true( $nodeSession != null );
		    } else {
		        expect_false( $nodeSession );
		    }

            } catch(Exception $e) {
	            expect_eq($e->getCode(), $uri_string[$i][1]);
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
