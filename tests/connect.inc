<?php
	/*
	Default values are "localhost", "root",
	database "stest" and empty password.
	Change the MYSQLX_TEST environment values
	if you want to use another configuration
	*/

	const DEVAPI_EXT_NAME = "mysql_xdevapi";
	const OPENSSL_EXT_NAME = "openssl";

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
	$default_schema = get_mysqlx_env("MYSQLX_TEST_DEFAULT_SCHEMA", "MYSQLX_DEFAULT_SCHEMA", "");
	$engine = get_mysqlx_env("MYSQLX_TEST_ENGINE", "MYSQLX_ENGINE", "MyISAM");
	$socket = get_mysqlx_env("MYSQLX_TEST_SOCKET", "MYSQLX_SOCKET", "");
	$skip_on_connect_failure = get_mysqlx_env("MYSQLX_TEST_SKIP_CONNECT_FAILURE", "MYSQLX_SKIP_CONNECT_FAILURE", true);
	$connect_flags = (int)get_mysqlx_env("MYSQLX_TEST_CONNECT_FLAGS", "MYSQLX_CONNECT_FLAGS", 0);
	$disable_ssl_opt = 'ssl-mode=disabled';
	$base_uri = $scheme.'://'.$user.':'.$passwd.'@'.$host.':'.$port;
	$connection_uri = $base_uri.'/'.$default_schema;

	$test_schema_name = $db;
	$test_table_name = "test_table";
	$test_collection_name = "test_collection";
	$test_view_name = "test_view";

	// ----------------------------------------------------------------------

	if (!function_exists('msleep')) {
		function msleep($milliseconds) {
			usleep($milliseconds * 1000);
		}
	}

	// ----------------------------------------------------------------------

	if (!function_exists('is_odd')) {
		function is_odd($value) {
			return ($value & 1) == 1;
		}
	}

	if (!function_exists('is_even')) {
		function is_even($value) {
			return !is_odd($value);
		}
	}

	if (!function_exists('is_in_range')) {
		function is_in_range($value, $min, $max) {
			return ($min <= $value) && ($value <= $max);
		}
	}

	// ----------------------------------------------------------------------

	if (!function_exists('verify_os')) {
		function verify_os($os_mask) {
			return strtoupper(substr(PHP_OS, 0, 3)) === $os_mask;
		}
	}

	if (!function_exists('is_running_on_windows')) {
		function is_running_on_windows() {
			return verify_os('WIN');
		}
	}

	if (!function_exists('is_running_on_solaris')) {
		function is_running_on_solaris() {
			return verify_os('SunOS');
		}
	}

	if (!function_exists('assert_mysql_xdevapi_loaded')) {
		function assert_mysql_xdevapi_loaded() {
			assert(extension_loaded(DEVAPI_EXT_NAME), DEVAPI_EXT_NAME." not loaded!");
		}
	}

	if (!function_exists('is_module_builtin')) {
		function is_module_builtin($module_name) {
			$command = PHP_BINARY." -nm";
			exec($command, $builtin_modules, $retval);
			return ($retval == 0) && in_array($module_name, $builtin_modules);
		}
	}

	if (!function_exists('is_mysql_xdevapi_builtin')) {
		function is_mysql_xdevapi_builtin() {
			return is_module_builtin(DEVAPI_EXT_NAME);
		}
	}

	if (!function_exists('is_openssl_builtin')) {
		function is_openssl_builtin() {
			return is_module_builtin(OPENSSL_EXT_NAME);
		}
	}

	// ----------------------------------------------------------------------

	if (!function_exists('create_test_session')) {
		function create_test_session($create_test_db = false) {
			global $connection_uri;
			$session = mysql_xdevapi\getSession($connection_uri);
			if ($create_test_db) {
				global $db;
				$session->sql("create database $db")->execute();
			}
			return $session;
		}
	}

	/*
		The idea is to have a common function for the purpose of the creationg
		of the DB and tables needed to run the tests.
	*/
	if (!function_exists('create_test_db')) {
		function create_test_db($session = null, $table_spec = null) {
			global $connection_uri;
			global $db;
			global $test_table_name;
			global $test_collection_name;
			/*
				Would make sense to clean the database here, before attempting
				to create it. BUT, the code raise a warning during execute SQL
				if the script attempt to remove a DB which is not existing,
				(error 1008) which in turns cause the test to fail.

				For this reason, at least for now is better to expect from
				the test script to call clean_test_db *after* the test
			*/
			if ($session == null) $session = mysql_xdevapi\getSession($connection_uri);
			$session->sql("create database $db")->execute();
			$schema = $session->getSchema($db);
			if (!$table_spec) $table_spec = "name varchar(1024), age int, job varchar(1024)";
			$create_table_query = "create table $db.$test_table_name(" . $table_spec . ")";
			$session->sql($create_table_query)->execute();
			$schema->createCollection($test_collection_name);
			return $session;
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
			$session = mysql_xdevapi\getSession($connection_uri);
			$session->sql("drop database if exists $db_name")->execute();
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
			$session = mysql_xdevapi\getSession($connection_uri);
			$session->sql("insert into $db.$test_table_name values ('Mamie', 11, 'hostess')")->execute();
			$session->sql("insert into $db.$test_table_name values ('Eulalia', 11, 'teacher')")->execute();
			$session->sql("insert into $db.$test_table_name values ('Polly', 12, 'tailor')")->execute();
			$session->sql("insert into $db.$test_table_name values ('Rufus', 12, 'butcher')")->execute();
			$session->sql("insert into $db.$test_table_name values ('Cassidy', 13, 'student')")->execute();
			$session->sql("insert into $db.$test_table_name values ('Olympia', 14, 'teacher')")->execute();
			$session->sql("insert into $db.$test_table_name values ('Lev', 14, 'builder')")->execute();
			$session->sql("insert into $db.$test_table_name values ('Tierney', 15, 'driver')")->execute();
			$session->sql("insert into $db.$test_table_name values ('Octavia', 15, 'cook')")->execute();
			$session->sql("insert into $db.$test_table_name values ('Vesper', 16, 'builder')")->execute();
			$session->sql("insert into $db.$test_table_name values ('Caspian', 17, 'tailor')")->execute();
			$session->sql("insert into $db.$test_table_name values ('Romy', 17, 'teacher')")->execute();
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
			$session = mysql_xdevapi\getSession($connection_uri);
			$session->sql("insert into $db.$test_table_name values ('Mamie', 29, 'hostess')")->execute();
			$session->sql("insert into $db.$test_table_name values ('Mamie', 21, 'teacher')")->execute();
			$session->sql("insert into $db.$test_table_name values ('Polly', 34, 'cook')")->execute();
			$session->sql("insert into $db.$test_table_name values ('Polly', 27, 'tailor')")->execute();
			$session->sql("insert into $db.$test_table_name values ('Cassidy', 33, 'cook')")->execute();
			$session->sql("insert into $db.$test_table_name values ('Cassidy', 34, 'singer')")->execute();
			$session->sql("insert into $db.$test_table_name values ('Cassidy', 20, 'cook')")->execute();
			$session->sql("insert into $db.$test_table_name values ('Tierney', 46, 'plunger')")->execute();
			$session->sql("insert into $db.$test_table_name values ('Tierney', 34, 'driver')")->execute();
			$session->sql("insert into $db.$test_table_name values ('Tierney', 25, 'plunger')")->execute();
			$session->sql("insert into $db.$test_table_name values ('Tierney', 39, 'driver')")->execute();
			$session->sql("insert into $db.$test_table_name values ('ARomy', 17, 'actor')")->execute();
			$session->sql("insert into $db.$test_table_name values ('BRomy', 17, 'driver')")->execute();
			$session->sql("insert into $db.$test_table_name values ('CRomy', 17, 'seller')")->execute();
			$session->sql("insert into $db.$test_table_name values ('DRomy', 17, 'butcher')")->execute();
			$session->sql("insert into $db.$test_table_name values ('ERomy', 17, 'builder')")->execute();
		}
	}

	/*
		Fill the database collection with stuff
	*/
	if(!function_exists('fill_db_collection')){
		function fill_db_collection($coll, $extended_set = false) {
			global $test_collection_name;
			$coll->createIndex(
				$test_collection_name.'ordinal_index',
				'{"fields": [{"field": "$.ordinal", "type": "INTEGER", "required": false}], "unique": false}');

			$coll->add('{"_id": "1", "ordinal":  1, "name": "Marco",      "age": 19, "job": "Programmatore"}')->execute();
			$coll->add('{"_id": "2", "ordinal":  2, "name": "Lonardo",    "age": 59, "job": "Paninaro"}')->execute();
			$coll->add('{"_id": "3", "ordinal":  3, "name": "Riccardo",   "age": 27, "job": "Cantante"}')->execute();
			$coll->add('{"_id": "4", "ordinal":  4, "name": "Carlotta",   "age": 23, "job": "Programmatrice"}')->execute();
			$coll->add('{"_id": "5", "ordinal":  5, "name": "Carlo",      "age": 25, "job": "Programmatore"}')->execute();
			$coll->add('{"_id": "6", "ordinal":  6, "name": "Mariangela", "age": 41, "job": "Programmatrice"}')->execute();
			$coll->add('{"_id": "7", "ordinal":  7, "name": "Alfredo",    "age": 27, "job": "Programmatore"}')->execute();
			$coll->add('{"_id": "8", "ordinal":  8, "name": "Antonella",  "age": 42, "job": "Studente"}')->execute();
			$coll->add('{"_id": "9", "ordinal":  9, "name": "Monica",     "age": 35, "job": "Ballerino"}')->execute();
			$coll->add('{"_id":"10", "ordinal": 10, "name": "Giulio",     "age": 29, "job": "Disoccupato"}')->execute();
			$coll->add('{"_id":"11", "ordinal": 11, "name": "Lucia",      "age": 47, "job": "Barista"}')->execute();
			$coll->add('{"_id":"12", "ordinal": 12, "name": "Filippo",    "age": 31, "job": "Spazzino"}')->execute();
			$coll->add('{"_id":"13", "ordinal": 13, "name": "Alessandra", "age": 15, "job": "Barista"}')->execute();
			$coll->add('{"_id":"14", "ordinal": 14, "name": "Massimo",    "age": 22, "job": "Programmatore"}')->execute();
			$coll->add('{"_id":"15", "ordinal": 15, "name": "Carlo",      "age": 37, "job": "Calciatore"}')->execute();
			$coll->add('{"_id":"16", "ordinal": 16, "name": "Leonardo",   "age": 23, "job": "Programmatore"}')->execute();

			if (!$extended_set) return;

			$coll->add('{"_id":"17", "ordinal": 17, "name": "Andrea",     "age": 58, "job": "Cantante"}')->execute();
			$coll->add('{"_id":"18", "ordinal": 18, "name": "Francesco",  "age": 40, "job": "Calciatore"}')->execute();
			$coll->add('{"_id":"19", "ordinal": 19, "name": "Dino",       "age": 75, "job": "Portiere"}')->execute();
			$coll->add('{"_id":"20", "ordinal": 20, "name": "Monica",     "age": 52, "job": "Attrice e modella"}')->execute();
			$coll->add('{"_id":"21", "ordinal": 21, "name": "Fabio",      "age": 59, "job": "Pilota automobilistico"}')->execute();
			$coll->add('{"_id":"22", "ordinal": 22, "name": "Gianluigi",  "age": 39, "job": "Portiere"}')->execute();
			$coll->add('{"_id":"23", "ordinal": 23, "name": "Sophia",     "age": 82, "job": "Attrice"}')->execute();
			$coll->add('{"_id":"24", "ordinal": 24, "name": "Enzo",       "age": 90, "job": "Ferrari"}')->execute();
			$coll->add('{"_id":"25", "ordinal": 25, "name": "Sergio",     "age": 65, "job": "Direzione aziendale"}')->execute();
		}
	}

	/*
		Fill the test_collection with stuff
	*/
	if (!function_exists('fill_test_collection')) {
		function fill_test_collection($extended_set = false) {
			global $session;
			global $db;
			global $test_collection_name;
			$schema = $session->getSchema($db);
			$coll = $schema->getCollection($test_collection_name);
			fill_db_collection($coll, $extended_set);
			return $coll;
		}
	}

	$number_of_expectations = 0;
	$test_result_string = "";

	/*
		The idea is to have a common function for the purpose of the creating
		of the view to run the tests. It expects test_table is available.
	*/
	if (!function_exists('create_test_view')) {
		function create_test_view($session) {
			global $db;
			global $test_table_name;
			global $test_view_name;
			$session->sql(
				"CREATE VIEW $db.$test_view_name ".
				"AS SELECT name, age FROM $db.$test_table_name")->execute();
			$schema = $session->getSchema($db);
			$view = $schema->getTable($test_view_name);
			return $view;
		}
	}


	if (!function_exists('get_mysql_variable')) {
		function get_mysql_variable($session, $varname) {
			$res = $session->sql("SHOW VARIABLES LIKE '$varname'")->execute();
			$vardata = $res->fetchOne();
			return $vardata["Value"];
		}
	}

	if (!function_exists('get_mysql_variables')) {
		function get_mysql_variables($session, $pattern) {
			$res = $session->sql("SHOW VARIABLES LIKE '$pattern'")->execute();
			return $res->fetchAll();
		}
	}

	// ----------------------------------------------------------------------

	if (!function_exists('get_sql_modes')) {
		function get_sql_modes($session) {
			$res = $session->sql('SELECT @@SESSION.sql_mode')->execute();
			$sql_modes_str = $res->fetchAll()[0]['@@SESSION.sql_mode'];
			$sql_modes = explode(',', $sql_modes_str);
			return $sql_modes;
		}
	}

	if (!function_exists('set_sql_modes')) {
		function set_sql_modes($session, $sql_modes) {
			$sql_modes_str = implode(',', $sql_modes);
			$session->sql('SET SESSION sql_mode = \'' . $sql_modes_str . '\'')->execute();
		}
	}

	if (!function_exists('disable_sql_mode')) {
		function disable_sql_mode($session, $sql_mode) {
			$sql_modes = get_sql_modes($session);
			$new_sql_modes = array_diff($sql_modes, array($sql_mode));
			set_sql_modes($session, $new_sql_modes);
		}
	}

	if (!function_exists('enable_sql_mode')) {
		function enable_sql_mode($session, $sql_mode) {
			$sql_modes = get_sql_modes($session);
			array_push($sql_modes, $sql_mode);
			set_sql_modes($session, $sql_modes);
		}
	}

	// ----------------------------------------------------------------------

	/*
		Used to get backtrace in easy to read form (filename / line number)
	*/
	if (!function_exists('get_backtrace')) {
		function get_backtrace() {
			$backtrace_log = '';
			$backtrace = debug_backtrace();
			$frames_count = count($backtrace);
			$prev_filename = null;
 			for ($i = 0; $i < $frames_count; ++$i) {
				if ($prev_filename) {
					$backtrace_log .= "=>";
				}

				$frame_info = $backtrace[$i];
				$full_path = $frame_info["file"];
				$filename = pathinfo($full_path, PATHINFO_BASENAME);
				if ($prev_filename != $filename) {
					$backtrace_log .= $filename;
					$prev_filename = $filename;
				}

				$line_num = $frame_info["line"];
				$backtrace_log .= "(".$line_num.")";
			}
			return $backtrace_log;
		}
	}

	// to_string conversion for diagnostics purposes
	if( !function_exists('to_string')) {
		function to_string($value) {
			if (is_array($value)) {
				return implode(' or ', $value);
			} else {
				return $value;
			}
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
				print get_backtrace().': \''.$target.'\' does not match pattern \''.$pattern.'\' '.$hint.PHP_EOL;
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

			if( $target === $expectation ){
				$test_result_string .= '1';
			} else {
				print get_backtrace().': \''.$target.'\' !== \''.$expectation.'\' '.$hint.PHP_EOL;
				$test_result_string .= '0';
			}
		}
	}

	/*
		used to compare id expected values with actual values
	*/
	if( !function_exists('expect_eq_id')) {
		function expect_eq_id($raw_target, $raw_expectation, $hint = '') {
			global $number_of_expectations;
			global $test_result_string;

			$number_of_expectations++;

			$target = is_int($raw_target) ? strval($target) : $raw_target;
			$expectation = is_int($raw_expectation) ? strval($target) : $raw_expectation;

			if( $target === $expectation ){
				$test_result_string .= '1';
			} else {
				print get_backtrace().': _id \''.$raw_target.'\' !== \''.$raw_expectation.'\' '.$hint.PHP_EOL;
				$test_result_string .= '0';
			}
		}
	}

	/*
		Used to check actual value differs from expected value
	*/
	if( !function_exists('expect_neq')) {
		function expect_neq($target, $expectation, $hint = '') {
			global $number_of_expectations;
			global $test_result_string;

			$number_of_expectations++;

			if( $target !== $expectation ){
				$test_result_string .= '1';
			} else {
				print get_backtrace().': \''.$target.'\' === \''.$expectation.'\' '.$hint.PHP_EOL;
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

			if( true === $expectation ){
				$test_result_string .= '1';
			} else {
				print get_backtrace().': true expected '.$hint.PHP_EOL;
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

			if( false === $expectation ){
				$test_result_string .= '1';
			} else {
				print get_backtrace().': false expected '.$hint.PHP_EOL;
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

			if( null === $expectation ){
				$test_result_string .= '1';
			} else {
				print get_backtrace().
					': null expected, but met value ('.$hint.')'.PHP_EOL;
				var_dump($expectation);
				$test_result_string .= '0';
			}
		}
	}

	/*
		Used to verify an expected non-null value
	*/
	if( !function_exists('expect_not_null')) {
		function expect_not_null($expectation, $hint = '') {
			global $number_of_expectations;
			global $test_result_string;

			$number_of_expectations++;

			if( null !== $expectation ){
				$test_result_string .= '1';
			} else {
				print get_backtrace().
					': non-null expected, but met null value '.$expectation.' ('.$hint.')'.PHP_EOL;
				$test_result_string .= '0';
			}
		}
	}

	/*
		check whether value is equal to expected value or in list of possible values
		$expectation may be scalar or array
	*/
	if( !function_exists('expect_eq_or_in')) {
		function expect_eq_or_in($target, $expectation, $hint = '') {
			global $number_of_expectations;
			global $test_result_string;

			$number_of_expectations++;

			$is_equal = is_array($expectation) ?
				in_array($target, $expectation) : ($target === $expectation);
			if ($is_equal) {
				$test_result_string .= '1';
			} else {
				print get_backtrace().': \''.$target.'\' !== \''.to_string($expectation).'\' '.$hint.PHP_EOL;
				$test_result_string .= '0';
			}
		}
	}

	/*
		used to ensure expected value is an empty array
	*/
	if (!function_exists('expect_empty_array')) {
		function expect_empty_array($expectation, $hint = '') {
			expect_eq($expectation, [], $hint);
		}
	}

	/*
		used to check whether expected value is an empty array
	*/
	if (!function_exists('is_empty_array')) {
		function is_empty_array($variable) {
			return is_array($variable) && empty($variable);
		}
	}

	/*
		Call this function to add a failing point
		to the test
	*/
	if( !function_exists('test_step_failed')) {
		function test_step_failed($hint = '') {
			global $number_of_expectations;
			global $test_result_string;

			print get_backtrace().': test step failed '.$hint.PHP_EOL;

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
				return false;
			}
			return true;
		}
	}
	/*
			Return true if no expectation are broken
	*/
	if( !function_exists('any_expectation_broken')) {
		function any_expectation_broken() {
			global $number_of_expectations;
			global $test_result_string;

			$expected = str_repeat( '1', $number_of_expectations );
			if( $expected != $test_result_string) {
					return true;
			}
			return false;
		}
	}
?>
