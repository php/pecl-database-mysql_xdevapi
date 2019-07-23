--TEST--
mysqlx DNS SRV support
--SKIPIF--
--INI--
error_reporting=E_ALL
--FILE--
<?php
    require("connect.inc");

    $connection_uri = 'mysqlx+srv://'.$user.':'.$passwd.'@'.$host.'/'.$default_schema;
	$session = mysql_xdevapi\getSession($connection_uri);

    verify_expectations();
	print "done!".PHP_EOL;
?>
--CLEAN--
<?php
    require("connect.inc");
	clean_test_db();
?>
--EXPECTF--
done!%A
