--TEST--
mysqlx compression algorithm
--SKIPIF--
--FILE--
<?php
    require("connect.inc");
	//$session = mysql_xdevapi\getSession($scheme.'://'.$user.':'.$passwd.'@'.$host.':'.$port.'/?compression=false');
	//$session = mysql_xdevapi\getSession($scheme.'://'.$user.':'.$passwd.'@'.$host.':'.$port.'/?compression=FalSe');
	$session = mysql_xdevapi\getSession($scheme.'://'.$user.':'.$passwd.'@'.$host.':'.$port.'/?compression=On');
	//$session = mysql_xdevapi\getSession($scheme.'://'.$user.':'.$passwd.'@'.$host.':'.$port.'/?compression=TRUE,merda=true');

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
