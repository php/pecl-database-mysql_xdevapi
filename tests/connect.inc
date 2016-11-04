<?php
	/*
	Default values are "localhost", "root",
	database "stest" and empty password.
	Change the MYSQLX_TEST environment values
	if you want to use another configuration
	*/

	$driver    = new mysqli_driver;

	$host      = getenv("MYSQLX_TEST_HOST")     ? getenv("MYSQLX_TEST_HOST") : "localhost";
	$port      = getenv("MYSQLX_TEST_PORT")     ? getenv("MYSQLX_TEST_PORT") : 33060;
	$user      = getenv("MYSQLX_TEST_USER")     ? getenv("MYSQLX_TEST_USER") : "root";
	$passwd    = getenv("MYSQLX_TEST_PASSWD")   ? getenv("MYSQLX_TEST_PASSWD") : "";
	$db        = getenv("MYSQLX_TEST_DB")       ? getenv("MYSQLX_TEST_DB") : "testx";
	$engine    = getenv("MYSQLX_TEST_ENGINE")   ? getenv("MYSQLX_TEST_ENGINE") : "MyISAM";
	$socket    = getenv("MYSQLX_TEST_SOCKET")   ? getenv("MYSQLX_TEST_SOCKET") : null;
	$skip_on_connect_failure  = getenv("MYSQLX_TEST_SKIP_CONNECT_FAILURE") ? getenv("MYSQLX_TEST_SKIP_CONNECT_FAILURE") : true;
	$connect_flags = getenv("MYSQLX_TEST_CONNECT_FLAGS") ? (int)getenv("MYSQLX_TEST_CONNECT_FLAGS") : 0;
	if ($socket) {
		ini_set('mysqli.default_socket', $socket);
	}

	/* Development setting: test experimal features and/or feature requests that never worked before? */
	$TEST_EXPERIMENTAL = (in_array(getenv("MYSQLX_TEST_EXPERIMENTAL"), array(0, 1))) ?
				((1 == getenv("MYSQLX_TEST_EXPERIMENTAL")) ? true : false) :
				false;

	/*
		The idea is to have a common function for the purpose of the creationg
		of the DB and tables needed to run the tests.
	*/
	if (!function_exists('create_test_db')) {
		function create_test_db() {
			global $host;
			global $user;
			global $passwd;
			global $db;
			$nodeSession = mysql_xdevapi\getNodeSession($host, $user, $passwd);
			$nodeSession->executeSql("drop database if exists $db");
			$nodeSession->executeSql("create database $db");
			$nodeSession->executeSql("create table $db.test_table(name text, age int)");
			$nodeSession->getSchema($db)->createCollection("test_collection");
			return $nodeSession;
		}
	}

	/*
		Clean up the previously created DB and tables
	*/
	if (!function_exists('clean_test_db')) {
		function clean_test_db() {
			global $host;
			global $user;
			global $passwd;
			global $db;
			$nodeSession = mysql_xdevapi\getNodeSession($host, $user, $passwd);
			$nodeSession->executeSql("drop database if exists $db");
		}
	}
?>
