<?php
	/*
	Default values are "localhost", "root",
	database "stest" and empty password.
	Change the MYSQL_TEST environment values
	if you want to use another configuration
	*/

	$driver    = new mysqli_driver;

	$host      = getenv("MYSQL_TEST_HOST")     ? getenv("MYSQL_TEST_HOST") : "localhost";
        $port      = getenv("MYSQL_TEST_PORT")     ? getenv("MYSQL_TEST_PORT") : 33060;
	$user      = getenv("MYSQL_TEST_USER")     ? getenv("MYSQL_TEST_USER") : "root";
        $passwd    = getenv("MYSQL_TEST_PASSWD")   ? getenv("MYSQL_TEST_PASSWD") : "";
        $db        = getenv("MYSQL_TEST_DB")       ? getenv("MYSQL_TEST_DB") : "test";
	$engine    = getenv("MYSQL_TEST_ENGINE")   ? getenv("MYSQL_TEST_ENGINE") : "MyISAM";
	$socket    = getenv("MYSQL_TEST_SOCKET")   ? getenv("MYSQL_TEST_SOCKET") : null;
	$skip_on_connect_failure  = getenv("MYSQL_TEST_SKIP_CONNECT_FAILURE") ? getenv("MYSQL_TEST_SKIP_CONNECT_FAILURE") : true;
	$connect_flags = getenv("MYSQL_TEST_CONNECT_FLAGS") ? (int)getenv("MYSQL_TEST_CONNECT_FLAGS") : 0;
	if ($socket) {
		ini_set('mysqli.default_socket', $socket);
	}

	/* Development setting: test experimal features and/or feature requests that never worked before? */
	$TEST_EXPERIMENTAL = (in_array(getenv("MYSQL_TEST_EXPERIMENTAL"), array(0, 1))) ?
				((1 == getenv("MYSQL_TEST_EXPERIMENTAL")) ? true : false) :
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
                $nodeSession = mysql_xdevapi\getNodeSession($host, $user, $passwd);
                $nodeSession->executeSql("create database test;");
                $nodeSession->executeSql("create table test.test_table(name text, age int)");
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
                $nodeSession = mysql_xdevapi\getNodeSession($host, $user, $passwd);
                $nodeSession = mysql_xdevapi\getNodeSession($host, $user, $passwd);
                $nodeSession->executeSql("drop database if exists test");
            }
        }
?>
