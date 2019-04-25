--TEST--
mysqlx Date, Time types
--SKIPIF--
--INI--
error_reporting=E_ALL
--FILE--
<?php

	require("connect.inc");
	$session = mysql_xdevapi\getSession($connection_uri);
	create_test_db();
	$test_table = 'test_date_time_table';

	$session->sql("create table $db.$test_table ("
		."dt datetime, ts timestamp, dd date, tt time)")->execute();

	$schema = $session->getSchema($db);
	$table = $schema->getTable($test_table);

	$table->insert('dt', 'ts', 'dd', 'tt')->values(
		['1918-11-11 11:18:19','1981-12-13 09:02:01','1944-08-01', '17:00:0'],
		['1815-6-15 11:30:0','1999-03-12 08:02','2016-08-1', '07:00:00'],
		['1813-10-13 7:30','1995-1-1 0:0','1996-10-1', '5:40:0'],
		['1794-03-24 10:01','1999-01-1 21','2001-12-13', '0:0:1'],
		['1830-11-29 21','1996-04-4 14:3','1997-2-4', '0:1:13'],
		['1863-1-22 7:3:01','2003-06-07 8:1:2','2008-4-01', '12:0:00'],
		['1981-12-13 9:00:1','1989-06-04 7','1989-11-09', '19:03'],
		['1918-11-18','2004-05-01','1997-5-25', '22']
		)->execute();

	$res = $table->select('dt', 'ts', 'dd', 'tt');
	$data = $res->execute()->fetchAll();
	var_dump($data);

	verify_expectations();
	print "done!".PHP_EOL;
?>
--CLEAN--
<?php
	require("connect.inc");
	clean_test_db();
?>
--EXPECTF--
array(8) {
  [0]=>
  array(4) {
    ["dt"]=>
    string(19) "1918-11-11 11:18:19"
    ["ts"]=>
    string(19) "1981-12-13 09:02:01"
    ["dd"]=>
    string(10) "1944-08-01"
    ["tt"]=>
    string(17) "17:00:00.00000000"
  }
  [1]=>
  array(4) {
    ["dt"]=>
    string(19) "1815-06-15 11:30:00"
    ["ts"]=>
    string(19) "1999-03-12 08:02:00"
    ["dd"]=>
    string(10) "2016-08-01"
    ["tt"]=>
    string(17) "07:00:00.00000000"
  }
  [2]=>
  array(4) {
    ["dt"]=>
    string(19) "1813-10-13 07:30:00"
    ["ts"]=>
    string(19) "1995-01-01 00:00:00"
    ["dd"]=>
    string(10) "1996-10-01"
    ["tt"]=>
    string(17) "05:40:00.00000000"
  }
  [3]=>
  array(4) {
    ["dt"]=>
    string(19) "1794-03-24 10:01:00"
    ["ts"]=>
    string(19) "1999-01-01 21:00:00"
    ["dd"]=>
    string(10) "2001-12-13"
    ["tt"]=>
    string(17) "00:00:01.00000000"
  }
  [4]=>
  array(4) {
    ["dt"]=>
    string(19) "1830-11-29 21:00:00"
    ["ts"]=>
    string(19) "1996-04-04 14:03:00"
    ["dd"]=>
    string(10) "1997-02-04"
    ["tt"]=>
    string(17) "00:01:13.00000000"
  }
  [5]=>
  array(4) {
    ["dt"]=>
    string(19) "1863-01-22 07:03:01"
    ["ts"]=>
    string(19) "2003-06-07 08:01:02"
    ["dd"]=>
    string(10) "2008-04-01"
    ["tt"]=>
    string(17) "12:00:00.00000000"
  }
  [6]=>
  array(4) {
    ["dt"]=>
    string(19) "1981-12-13 09:00:01"
    ["ts"]=>
    string(19) "1989-06-04 07:00:00"
    ["dd"]=>
    string(10) "1989-11-09"
    ["tt"]=>
    string(17) "19:03:00.00000000"
  }
  [7]=>
  array(4) {
    ["dt"]=>
    string(19) "1918-11-18 00:00:00"
    ["ts"]=>
    string(19) "2004-05-01 00:00:00"
    ["dd"]=>
    string(10) "1997-05-25"
    ["tt"]=>
    string(17) "00:00:22.00000000"
  }
}
done!%A
