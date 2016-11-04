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

	/*
	    Fill the database table with stuff
	*/
	if(!function_exists('fill_db_table')){
		function fill_db_table() {
		    global $host;
		    global $user;
		    global $passwd;
		    global $db;
		    $nodeSession = mysql_xdevapi\getNodeSession($host, $user, $passwd);
		    $nodeSession->executeSql("insert into $db.test_table values ('Mamie', 11)");
		    $nodeSession->executeSql("insert into $db.test_table values ('Eulalia', 11)");
		    $nodeSession->executeSql("insert into $db.test_table values ('Polly', 12)");
		    $nodeSession->executeSql("insert into $db.test_table values ('Rufus', 12)");
		    $nodeSession->executeSql("insert into $db.test_table values ('Cassidy', 13)");
		    $nodeSession->executeSql("insert into $db.test_table values ('Olympia', 14)");
		    $nodeSession->executeSql("insert into $db.test_table values ('Lev', 14)");
		    $nodeSession->executeSql("insert into $db.test_table values ('Tierney', 15)");
		    $nodeSession->executeSql("insert into $db.test_table values ('Octavia', 15)");
		    $nodeSession->executeSql("insert into $db.test_table values ('Vesper', 16)");
		    $nodeSession->executeSql("insert into $db.test_table values ('Caspian', 17)");
		    $nodeSession->executeSql("insert into $db.test_table values ('Romy', 17)");
		}
	}
	/*
	    Fill the database collection with stuff
	*/
	if(!function_exists('fill_db_collection')){
		function fill_db_collection($coll) {
		    $coll->add('{"_id": 1, "name": "Marco",      "age": 19, "job": "Programmatore"}')->execute();
		    $coll->add('{"_id": 2, "name": "Lonardo",    "age": 59, "job": "Paninaro"}')->execute();
		    $coll->add('{"_id": 3, "name": "Riccardo",   "age": 27, "job": "Cantante"}')->execute();
		    $coll->add('{"_id": 4, "name": "Carlotta",   "age": 23, "job": "Programmatrice"}')->execute();
		    $coll->add('{"_id": 5, "name": "Carlo",      "age": 25, "job": "Programmatore"}')->execute();
		    $coll->add('{"_id": 6, "name": "Mariangela", "age": 41, "job": "Programmatrice"}')->execute();
		    $coll->add('{"_id": 7, "name": "Alfredo",    "age": 27, "job": "Programmatore"}')->execute();
		    $coll->add('{"_id": 8, "name": "Antonella",  "age": 42, "job": "Studente"}')->execute();
		    $coll->add('{"_id": 9, "name": "Monica",     "age": 35, "job": "Ballerino"}')->execute();
		    $coll->add('{"_id":10, "name": "Giulio",     "age": 29, "job": "Disoccupato"}')->execute();
		    $coll->add('{"_id":11, "name": "Lucia",      "age": 47, "job": "Barista"}')->execute();
		    $coll->add('{"_id":12, "name": "Filippo",    "age": 31, "job": "Spazzino"}')->execute();
		    $coll->add('{"_id":13, "name": "Alessandra", "age": 15, "job": "Barista"}')->execute();
		    $coll->add('{"_id":14, "name": "Massimo",    "age": 22, "job": "Programmatore"}')->execute();
		    $coll->add('{"_id":15, "name": "Carlo",      "age": 37, "job": "Calciatore"}')->execute();
		    $coll->add('{"_id":16, "name": "Leonardo",   "age": 23, "job": "Programmatore"}')->execute();
		}
	}
?>
