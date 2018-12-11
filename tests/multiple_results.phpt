--TEST--
mysqlx support for multiple resultsets
--SKIPIF--
--FILE--
<?php
	require("connect.inc");

	function dump_single_result($res) {
		if ($res->hasData()) {
			var_dump($res->fetchAll());
		} else {
			echo "<empty rowset>", PHP_EOL;
		}
	}

	function dump_multi_result($res) {
		do {
			dump_single_result($res);
		} while ($res->nextResult());
	}

	function run_test_proc($create_test_proc) {
		global $counter;
		global $session;
		global $session;
		global $db;
		echo "---- ", $counter, " ----", PHP_EOL;
		$session->sql($create_test_proc)->execute();

		$res = $session->sql("CALL $db.test_proc()")->execute();
		dump_multi_result($res);

		$session->sql("DROP PROCEDURE $db.test_proc")->execute();
	}


	$session = create_test_db();
	fill_db_table();

	$counter = 0;
	$create_test_proc = "
		CREATE DEFINER = CURRENT_USER PROCEDURE $db.test_proc()
		BEGIN
			SELECT * FROM $db.test_table WHERE age = 12;
		END;
	";
	run_test_proc($create_test_proc);

	++$counter;
	$create_test_proc = "
		CREATE DEFINER = CURRENT_USER PROCEDURE $db.test_proc()
		BEGIN
			SELECT * FROM $db.test_table WHERE age = 1024;
		END;
	";
	run_test_proc($create_test_proc);

	++$counter;
	$create_test_proc = "
		CREATE DEFINER = CURRENT_USER PROCEDURE $db.test_proc()
		BEGIN
			SELECT * FROM $db.test_table WHERE age = 2048;
			SELECT * FROM $db.test_table WHERE age = 13;
			SELECT * FROM $db.test_table WHERE age = 4096;

		END;
	";
	run_test_proc($create_test_proc);

	++$counter;
	$create_test_proc = "
		CREATE DEFINER = CURRENT_USER PROCEDURE $db.test_proc()
		BEGIN
			SELECT * FROM $db.test_table WHERE age = 11;
			SELECT * FROM $db.test_table where name like 'O%';
			SELECT * FROM $db.test_table WHERE age = 16;
			SELECT * FROM $db.test_table where name = 'Romy';
		END;
	";
	run_test_proc($create_test_proc);

	print "done!";
?>
--CLEAN--
<?php
	require("connect.inc");
	clean_test_db();
?>
--EXPECTF--
---- 0 ----
array(2) {
  [0]=>
  array(3) {
    ["name"]=>
    string(5) "Polly"
    ["age"]=>
    int(12)
    ["job"]=>
    string(6) "tailor"
  }
  [1]=>
  array(3) {
    ["name"]=>
    string(5) "Rufus"
    ["age"]=>
    int(12)
    ["job"]=>
    string(7) "butcher"
  }
}
---- 1 ----
<empty rowset>
---- 2 ----
<empty rowset>
array(1) {
  [0]=>
  array(3) {
    ["name"]=>
    string(7) "Cassidy"
    ["age"]=>
    int(13)
    ["job"]=>
    string(7) "student"
  }
}
<empty rowset>
---- 3 ----
array(2) {
  [0]=>
  array(3) {
    ["name"]=>
    string(5) "Mamie"
    ["age"]=>
    int(11)
    ["job"]=>
    string(7) "hostess"
  }
  [1]=>
  array(3) {
    ["name"]=>
    string(7) "Eulalia"
    ["age"]=>
    int(11)
    ["job"]=>
    string(7) "teacher"
  }
}
array(2) {
  [0]=>
  array(3) {
    ["name"]=>
    string(7) "Olympia"
    ["age"]=>
    int(14)
    ["job"]=>
    string(7) "teacher"
  }
  [1]=>
  array(3) {
    ["name"]=>
    string(7) "Octavia"
    ["age"]=>
    int(15)
    ["job"]=>
    string(4) "cook"
  }
}
array(1) {
  [0]=>
  array(3) {
    ["name"]=>
    string(6) "Vesper"
    ["age"]=>
    int(16)
    ["job"]=>
    string(7) "builder"
  }
}
array(1) {
  [0]=>
  array(3) {
    ["name"]=>
    string(4) "Romy"
    ["age"]=>
    int(17)
    ["job"]=>
    string(7) "teacher"
  }
}
done!%A
