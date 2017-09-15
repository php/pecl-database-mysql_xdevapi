--TEST--
mysqlx create/drop schema
--SKIPIF--
--INI--
error_reporting=0
--FILE--
<?php
	require_once("connect.inc");

	$nodeSession = mysql_xdevapi\getSession($connection_uri);

	function database_exist($name) {
		global $nodeSession;
		$db_exist = $nodeSession->executeSql("show databases like '$name'");
		$res = $db_exist->fetchAll();
		try {
			if ($res[0]["Database ($name)"]==$name) {
				return true;
			}
		} catch(Exception $x) {
		}
		return false;
	}

	$test = "000000";

	$nodeSession->createSchema($test_schema_name);

	if (database_exist($test_schema_name)) {
		$test[0] = "1";

		try {
			#This should fail as the DB already exist
			$nodeSession->createSchema($test_schema_name);
		} catch(Exception $e) {
			$test[1] = "1";
		}

		try {
			$nodeSession->dropSchema($test_schema_name);
			if (!database_exist($test_schema_name)) {
				$test[2] = "1";
			}
		} catch(Exception $x) {
		}
	}

	if ($nodeSession->createSchema("") == NULL) {
		$test[3] = "1";
	}

	for ($n = 1; $n <= 3; $n++)
	{
		$nodeSession->createSchema("test_schema$n");
	}

	$schemas = $nodeSession->getSchemas();
	$discovered_schemas = 0;

	for ($i = 0; $i < count($schemas); $i++) {
		/*
			I'm assuming that the order of the DB's may be different
			from the one deduced by the creation sequence
		*/
		for ($j = 1; $j <= 3; $j++) {
			if ("test_schema$j" == $schemas[$i]->getName()) {
				$discovered_schemas++;
				$nodeSession->dropSchema("test_schema$j");
			}
		}
	}

	if ($discovered_schemas == 3)
		$test[4] = "1";

	for ($i = 1; $i <= 3; $i++) {
		if (database_exist("test_schema$i") == false)
			$discovered_schemas--;
	}

	if ($discovered_schemas == 0)
		$test[5] = "1";

	var_dump($test);
	print "done!\n";
?>
--CLEAN--
<?php
	require("connect.inc");
	clean_test_db($test_schema_name);
?>
--EXPECTF--
%s(6) "111111"
done!%A
