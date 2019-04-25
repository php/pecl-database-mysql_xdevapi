--TEST--
mysqlx session minor TC's
--SKIPIF--
--FILE--
<?php
	require("connect.inc");

	$test = "00";

	$session = create_test_db();

	$schema = $session->getSchema("bad");

	$schema = $session->getSchema($db);

	$svr_version = $session->getServerVersion();
	if(is_int($svr_version) && $svr_version != 0)
		$test[0] = "1";

	$uuid = $session->generateUUID();
	if(is_string($uuid) && !empty($uuid) != 0)
		$test[1] = "1";

	var_dump($session->quoteName("test test test"));
	var_dump($session->quoteName("x'y'z' test"));
	var_dump($session->quoteName(""));

        $session = mysql_xdevapi\getSession($connection_uri);

	var_dump($session->quoteName("test test test"));
	var_dump($session->quoteName("x'y'z' test"));
	var_dump($session->quoteName(""));

        var_dump($session->quoteName("test `test` `2`"));

	var_dump($test);

	print "done!\n";
?>
--CLEAN--
<?php
	require("connect.inc");
	clean_test_db();
?>
--EXPECTF--
string(16) "`test test test`"
string(13) "`x'y'z' test`"
string(0) ""
string(16) "`test test test`"
string(13) "`x'y'z' test`"
string(0) ""
string(21) "`test ``test`` ``2```"
string(2) "11"
done!%A
