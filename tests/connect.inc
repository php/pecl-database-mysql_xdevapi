<?php
	/*
	Default values are "localhost", "root",
	database "stest" and empty password.
	Change the MYSQLX_TEST environment values
	if you want to use another configuration
	*/

	$driver    = new mysqli_driver;

	if (!function_exists('get_mysqlx_env')) {
		function get_mysqlx_env($test_envar, $common_envar, $default_value) {
			if (getenv($test_envar)) return getenv($test_envar);
			if (getenv($common_envar)) return getenv($common_envar);
			return $default_value;
		}
	}

	$scheme = get_mysqlx_env("MYSQLX_TEST_SCHEME", "MYSQLX_SCHEME", "mysqlx");
	$host = get_mysqlx_env("MYSQLX_TEST_HOST", "MYSQLX_HOST", "localhost");
	$port = get_mysqlx_env("MYSQLX_TEST_PORT", "MYSQLX_PORT", 33060);
	$user = get_mysqlx_env("MYSQLX_TEST_USER", "MYSQLX_USER", "root");
	$passwd = get_mysqlx_env("MYSQLX_TEST_PASSWD", "MYSQLX_PASSWD", "");
	$db = get_mysqlx_env("MYSQLX_TEST_DB", "MYSQLX_DB", "testx");
	$engine = get_mysqlx_env("MYSQLX_TEST_ENGINE", "MYSQLX_ENGINE", "MyISAM");
	$socket = get_mysqlx_env("MYSQLX_TEST_SOCKET", "MYSQLX_SOCKET", "");
	$skip_on_connect_failure = get_mysqlx_env("MYSQLX_TEST_SKIP_CONNECT_FAILURE", "MYSQLX_SKIP_CONNECT_FAILURE", true);
	$connect_flags = (int)get_mysqlx_env("MYSQLX_TEST_CONNECT_FLAGS", "MYSQLX_CONNECT_FLAGS", 0);
	$disable_ssl_opt = 'ssl-mode=disabled';
	$connection_uri = $scheme.'://'.$user.':'.$passwd.'@'.$host.':'.$port.'/?'.$disable_ssl_opt;

	$test_schema_name = $db;
	$test_table_name = "test_table";
	$test_collection_name = "test_collection";
	$test_view_name = "test_view";

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
			global $connection_uri;
			global $db;
			global $test_table_name;
			global $test_collection_name;
			/*
				Would make sense to clean the database here, before attempting
				to create it. BUT, the code raise a warning during executeSql
				if the script attempt to remove a DB which is not existing,
				(error 1008) which in turns cause the test to fail.

				For this reason, at least for now is better to expect from
				the test script to call clean_test_db *after* the test
			*/
			//clean_test_db();
			$nodeSession = mysql_xdevapi\getSession($connection_uri);
			$nodeSession->executeSql("create database $db");
			$schema = $nodeSession->getSchema($db);
			$schema->createTable($test_table_name)
				->addColumn(new mysql_xdevapi\ColumnDef('name', 'varchar', 1024))
				->addColumn(new mysql_xdevapi\ColumnDef('age', 'int'))
				->execute();
			$schema->createCollection($test_collection_name);
			return $nodeSession;
		}
	}

	/*
		Clean up the previously created DB and tables
	*/
	if (!function_exists('clean_test_db')) {
		function clean_test_db($db_name = null) {
			global $connection_uri;
			global $db;
			if ($db_name == null) $db_name = $db;
			$nodeSession = mysql_xdevapi\getSession($connection_uri);
			$nodeSession->executeSql("drop database if exists $db_name");
		}
	}

	/*
		Fill the database table with stuff
	*/
	if(!function_exists('fill_db_table')){
		function fill_db_table() {
			global $connection_uri;
			global $db;
			global $test_table_name;
			$nodeSession = mysql_xdevapi\getSession($connection_uri);
			$nodeSession->executeSql("insert into $db.$test_table_name values ('Mamie', 11)");
			$nodeSession->executeSql("insert into $db.$test_table_name values ('Eulalia', 11)");
			$nodeSession->executeSql("insert into $db.$test_table_name values ('Polly', 12)");
			$nodeSession->executeSql("insert into $db.$test_table_name values ('Rufus', 12)");
			$nodeSession->executeSql("insert into $db.$test_table_name values ('Cassidy', 13)");
			$nodeSession->executeSql("insert into $db.$test_table_name values ('Olympia', 14)");
			$nodeSession->executeSql("insert into $db.$test_table_name values ('Lev', 14)");
			$nodeSession->executeSql("insert into $db.$test_table_name values ('Tierney', 15)");
			$nodeSession->executeSql("insert into $db.$test_table_name values ('Octavia', 15)");
			$nodeSession->executeSql("insert into $db.$test_table_name values ('Vesper', 16)");
			$nodeSession->executeSql("insert into $db.$test_table_name values ('Caspian', 17)");
			$nodeSession->executeSql("insert into $db.$test_table_name values ('Romy', 17)");
		}
	}

	/*
		Fill the database with duplicated rows.
	*/
	if(!function_exists('fill_db_table_use_dup')){
		function fill_db_table_use_dup() {
			global $connection_uri;
			global $db;
			global $test_table_name;
			$nodeSession = mysql_xdevapi\getSession($connection_uri);
			$nodeSession->executeSql("insert into $db.$test_table_name values ('Mamie', 29)");
			$nodeSession->executeSql("insert into $db.$test_table_name values ('Mamie', 21)");
			$nodeSession->executeSql("insert into $db.$test_table_name values ('Polly', 34)");
			$nodeSession->executeSql("insert into $db.$test_table_name values ('Polly', 27)");
			$nodeSession->executeSql("insert into $db.$test_table_name values ('Cassidy', 33)");
			$nodeSession->executeSql("insert into $db.$test_table_name values ('Cassidy', 34)");
			$nodeSession->executeSql("insert into $db.$test_table_name values ('Cassidy', 20)");
			$nodeSession->executeSql("insert into $db.$test_table_name values ('Tierney', 46)");
			$nodeSession->executeSql("insert into $db.$test_table_name values ('Tierney', 34)");
			$nodeSession->executeSql("insert into $db.$test_table_name values ('Tierney', 25)");
			$nodeSession->executeSql("insert into $db.$test_table_name values ('Tierney', 39)");
			$nodeSession->executeSql("insert into $db.$test_table_name values ('ARomy', 17)");
			$nodeSession->executeSql("insert into $db.$test_table_name values ('BRomy', 17)");
			$nodeSession->executeSql("insert into $db.$test_table_name values ('CRomy', 17)");
			$nodeSession->executeSql("insert into $db.$test_table_name values ('DRomy', 17)");
			$nodeSession->executeSql("insert into $db.$test_table_name values ('ERomy', 17)");
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

	/*
		Fill the test_collection with stuff
	*/
	if (!function_exists('fill_test_collection')) {
		function fill_test_collection() {
			global $nodeSession;
			global $db;
			global $test_collection_name;
			$schema = $nodeSession->getSchema($db);
			$coll = $schema->getCollection($test_collection_name);
			fill_db_collection($coll);
		}
	}

	$number_of_expectations = 0;
	$test_result_string = "";

	/*
		The idea is to have a common function for the purpose of the creating
		of the view to run the tests. It expects test_table is available.
	*/
	if (!function_exists('create_test_view')) {
		function create_test_view($nodeSession) {
			global $db;
			global $test_table_name;
			global $test_view_name;
			$schema = $nodeSession->getSchema($db);
			$table = $schema->getTable($test_table_name);
			$select = $table->select('name', 'age');
			$schema->createView($test_view_name, true)
				->columns(['name', 'age'])->definedAs($select)->execute();
			$view = $schema->getTable($test_view_name);
			return $view;
		}
	}

	/*
		Used to get line number of specified stack frame
	*/
	if (!function_exists('get_stack_frame_line_num')) {
		function get_stack_frame_line_num($depth = 1) {
			$backtrace = debug_backtrace();
			assert($depth <= count($backtrace));
			$line_num = $backtrace[$depth]["line"];
			return $line_num;
		}
	}

	/*
		Used to match expected pattern with actual value
	*/
	if( !function_exists('expect_regex')) {
		function expect_regex($target, $pattern, $hint = '') {
			global $number_of_expectations;
			global $test_result_string;

			$number_of_expectations++;

			if (preg_match($pattern, $target)) {
				$test_result_string .= '1';
			} else {
				print 'Line '.get_stack_frame_line_num().': \''.$target.'\' does not match pattern \''.$pattern.'\' '.$hint.PHP_EOL;
				$test_result_string .= '0';
			}
		}
	}

	/*
		Used to compare expected values with actual values
	*/
	if( !function_exists('expect_eq')) {
		function expect_eq($target, $expectation, $hint = '') {
			global $number_of_expectations;
			global $test_result_string;

			$number_of_expectations++;

			if( $target == $expectation ){
				$test_result_string .= '1';
			} else {
				print 'Line '.get_stack_frame_line_num().': \''.$target.'\' != \''.$expectation.'\' '.$hint.PHP_EOL;
				$test_result_string .= '0';
			}
		}
	}
	/*
		Used to compare expected a boolean value being true
	*/
	if( !function_exists('expect_true')) {
		function expect_true($expectation, $hint = '') {
			global $number_of_expectations;
			global $test_result_string;

			$number_of_expectations++;

			if( true == $expectation ){
				$test_result_string .= '1';
			} else {
				print 'Line '.get_stack_frame_line_num().': true expected '.$hint.PHP_EOL;
				$test_result_string .= '0';
			}
		}
	}
	/*
		Used to compare expected a boolean value being false
	*/
	if( !function_exists('expect_false')) {
		function expect_false($expectation, $hint = '') {
			global $number_of_expectations;
			global $test_result_string;

			$number_of_expectations++;

			if( false == $expectation ){
				$test_result_string .= '1';
			} else {
				print 'Line '.get_stack_frame_line_num().': false expected '.$hint.PHP_EOL;
				$test_result_string .= '0';
			}
		}
	}
	/*
		Used to compare expected a null value
	*/
	if( !function_exists('expect_null')) {
		function expect_null($expectation, $hint = '') {
			global $number_of_expectations;
			global $test_result_string;

			$number_of_expectations++;

			if( null == $expectation ){
				$test_result_string .= '1';
			} else {
				print 'Line '.get_stack_frame_line_num().': null expected '.$hint.PHP_EOL;
				$test_result_string .= '0';
			}
		}
	}
	/*
		Call this function to add a failing point
		to the test
	*/
	if( !function_exists('test_step_failed')) {
		function test_step_failed() {
			global $number_of_expectations;
			global $test_result_string;

			print 'Line '.get_stack_frame_line_num().': test step failed'.PHP_EOL;

			$number_of_expectations++;
			$test_result_string .= 'F';
		}
	}
	/*
		Call this function to add a success point
		to the test
	*/
	if( !function_exists('test_step_ok')) {
		function test_step_ok() {
			global $number_of_expectations;
			global $test_result_string;

			$number_of_expectations++;
			$test_result_string .= '1';
		}
	}
	/*
		Verify if there were missed expectations and
		print out the test result string
	*/
	if( !function_exists('verify_expectations')) {
		function verify_expectations() {
			global $number_of_expectations;
			global $test_result_string;

			$expected = str_repeat( '1', $number_of_expectations );
			if( $expected != $test_result_string) {
				print 'Some expectation were not meet!'.PHP_EOL;
				print 'Expected vs Result: '.PHP_EOL.$expected.PHP_EOL.$test_result_string.PHP_EOL;
			}
		}
	}
?>
