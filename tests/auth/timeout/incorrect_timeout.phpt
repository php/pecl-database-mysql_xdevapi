--TEST--
mysqlx incorrect connection timeout
--SKIPIF--
--INI--
error_reporting=1
--FILE--
<?php
require_once(__DIR__."/timeout_utils.inc");

function test_incorrect_timeouts($host) {
	test_incorrect_timeout($host, null, -5);
	test_incorrect_timeout($host, 81, -10);
	test_incorrect_timeout($host, 80, 'this_is_incorrect_timeout');
	test_incorrect_timeout($host, 82, 'invalid-time-out');
	test_incorrect_timeout($host, null, -1.1);
	test_incorrect_timeout($host, 83, -10.101);
	test_incorrect_timeout($host, null, ' ,');
	test_incorrect_timeout($host, null, '++--');
	test_incorrect_timeout($host, null, '');
}

test_incorrect_timeouts("127.0.0.1");
test_incorrect_timeouts($Non_routable_hosts[0]);
test_incorrect_timeouts("localhost");

verify_expectations();
print "done!\n";
?>
--CLEAN--
<?php
	require_once(__DIR__."/timeout_utils.inc");
	clean_test_db();
?>
--EXPECTF--
mysqlx://testuser:testpasswd@127.0.0.1/?connect-timeout=-5
[10050][HY000] TypeError: The connection timeout value must be a positive integer (including 0).
----------------------
mysqlx://testuser:testpasswd@127.0.0.1:81/?connect-timeout=-10
[10050][HY000] TypeError: The connection timeout value must be a positive integer (including 0).
----------------------
mysqlx://testuser:testpasswd@127.0.0.1:80/?connect-timeout=this_is_incorrect_timeout
[10052][HY000] Invalid argument. The argument to connect-timeout must be an integer, but it is 'this_is_incorrect_timeout'.
----------------------
mysqlx://testuser:testpasswd@127.0.0.1:82/?connect-timeout=invalid-time-out
[10052][HY000] Invalid argument. The argument to connect-timeout must be an integer, but it is 'invalid-time-out'.
----------------------
mysqlx://testuser:testpasswd@127.0.0.1/?connect-timeout=-1.1
[10050][HY000] TypeError: The connection timeout value must be a positive integer (including 0).
----------------------
mysqlx://testuser:testpasswd@127.0.0.1:83/?connect-timeout=-10.101
[10050][HY000] TypeError: The connection timeout value must be a positive integer (including 0).
----------------------
mysqlx://testuser:testpasswd@127.0.0.1/?connect-timeout= ,
[10052][HY000] Invalid argument. The argument to connect-timeout must be an integer, but it is ','.
----------------------
mysqlx://testuser:testpasswd@127.0.0.1/?connect-timeout=++--
[10052][HY000] Invalid argument. The argument to connect-timeout must be an integer, but it is '++--'.
----------------------
mysqlx://testuser:testpasswd@127.0.0.1/?connect-timeout=
[10052][HY000] Invalid argument. The argument to connect-timeout cannot be empty.
----------------------
mysqlx://testuser:testpasswd@198.51.100.255/?connect-timeout=-5
[10050][HY000] TypeError: The connection timeout value must be a positive integer (including 0).
----------------------
mysqlx://testuser:testpasswd@198.51.100.255:81/?connect-timeout=-10
[10050][HY000] TypeError: The connection timeout value must be a positive integer (including 0).
----------------------
mysqlx://testuser:testpasswd@198.51.100.255:80/?connect-timeout=this_is_incorrect_timeout
[10052][HY000] Invalid argument. The argument to connect-timeout must be an integer, but it is 'this_is_incorrect_timeout'.
----------------------
mysqlx://testuser:testpasswd@198.51.100.255:82/?connect-timeout=invalid-time-out
[10052][HY000] Invalid argument. The argument to connect-timeout must be an integer, but it is 'invalid-time-out'.
----------------------
mysqlx://testuser:testpasswd@198.51.100.255/?connect-timeout=-1.1
[10050][HY000] TypeError: The connection timeout value must be a positive integer (including 0).
----------------------
mysqlx://testuser:testpasswd@198.51.100.255:83/?connect-timeout=-10.101
[10050][HY000] TypeError: The connection timeout value must be a positive integer (including 0).
----------------------
mysqlx://testuser:testpasswd@198.51.100.255/?connect-timeout= ,
[10052][HY000] Invalid argument. The argument to connect-timeout must be an integer, but it is ','.
----------------------
mysqlx://testuser:testpasswd@198.51.100.255/?connect-timeout=++--
[10052][HY000] Invalid argument. The argument to connect-timeout must be an integer, but it is '++--'.
----------------------
mysqlx://testuser:testpasswd@198.51.100.255/?connect-timeout=
[10052][HY000] Invalid argument. The argument to connect-timeout cannot be empty.
----------------------
mysqlx://testuser:testpasswd@localhost/?connect-timeout=-5
[10050][HY000] TypeError: The connection timeout value must be a positive integer (including 0).
----------------------
mysqlx://testuser:testpasswd@localhost:81/?connect-timeout=-10
[10050][HY000] TypeError: The connection timeout value must be a positive integer (including 0).
----------------------
mysqlx://testuser:testpasswd@localhost:80/?connect-timeout=this_is_incorrect_timeout
[10052][HY000] Invalid argument. The argument to connect-timeout must be an integer, but it is 'this_is_incorrect_timeout'.
----------------------
mysqlx://testuser:testpasswd@localhost:82/?connect-timeout=invalid-time-out
[10052][HY000] Invalid argument. The argument to connect-timeout must be an integer, but it is 'invalid-time-out'.
----------------------
mysqlx://testuser:testpasswd@localhost/?connect-timeout=-1.1
[10050][HY000] TypeError: The connection timeout value must be a positive integer (including 0).
----------------------
mysqlx://testuser:testpasswd@localhost:83/?connect-timeout=-10.101
[10050][HY000] TypeError: The connection timeout value must be a positive integer (including 0).
----------------------
mysqlx://testuser:testpasswd@localhost/?connect-timeout= ,
[10052][HY000] Invalid argument. The argument to connect-timeout must be an integer, but it is ','.
----------------------
mysqlx://testuser:testpasswd@localhost/?connect-timeout=++--
[10052][HY000] Invalid argument. The argument to connect-timeout must be an integer, but it is '++--'.
----------------------
mysqlx://testuser:testpasswd@localhost/?connect-timeout=
[10052][HY000] Invalid argument. The argument to connect-timeout cannot be empty.
----------------------
done!%A
