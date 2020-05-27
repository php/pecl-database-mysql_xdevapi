--TEST--
mysqlx client fail due to incorrect options
--SKIPIF--
--INI--
error_reporting=E_ALL
default_socket_timeout=4
--FILE--
<?php
require_once(__DIR__."/../connect.inc");
require_once(__DIR__."/client_utils.inc");

$pooling_options = '{
	"enabled": "incorrect_boolen_value",
  	"maxSize": 10
}';
assert_client_fail_with_options($pooling_options);

$pooling_options = '{
	"enabled": 1.5,
  	"queueTimeOut": 1000
}';
assert_client_fail_with_options($pooling_options);

$pooling_options = '{
  	"queueTimeOut": 1000,
	"unknown_option": true
}';
assert_client_fail_with_options($pooling_options);

$pooling_options = '{
	"non_existing_option": 12,
  	"maxSize": 20
}';
assert_client_fail_with_options($pooling_options);

$pooling_options = '{
  	"maxSize": -10,
  	"maxIdleTime": 3600,
  	"queueTimeOut": 1000
}';
assert_client_fail_with_options($pooling_options);

$pooling_options = '{
  	"maxSize": 0,
  	"queueTimeOut": 1000
}';
assert_client_fail_with_options($pooling_options);

$pooling_options = '{
  	"maxSize": "eleven",
  	"enabled": true
}';
assert_client_fail_with_options($pooling_options);

$pooling_options = '{
	"enabled": false,
  	"maxIdleTime": "incorrect_idle_time",
  	"queueTimeOut": 1000
}';
assert_client_fail_with_options($pooling_options);

$pooling_options = '{
  	"maxIdleTime": -100
}';
assert_client_fail_with_options($pooling_options);

$pooling_options = '{
  	"maxIdleTime": true,
  	"queueTimeOut": 1000
}';
assert_client_fail_with_options($pooling_options);

$pooling_options = '{
  	"queueTimeOut": "non_default_queue_time"
}';
assert_client_fail_with_options($pooling_options);

$pooling_options = '{
	"enabled": false,
  	"queueTimeOut": -5000
}';
assert_client_fail_with_options($pooling_options);

$pooling_options = '{
  	"maxSize": 5,
  	"queueTimeOut": false
}';
assert_client_fail_with_options($pooling_options);

verify_expectations();
print "done!\n";
?>
--CLEAN--
<?php
	require_once(__DIR__."/../connect.inc");
	clean_test_db();
?>
--EXPECTF--
[10052][HY000] Invalid argument. Client option 'enabled' does not support value 'incorrect_boolen_value'.
[10052][HY000] Invalid argument. Client option 'enabled' does not support value 1.5.
[10052][HY000] Invalid argument. Client option 'unknown_option' is not recognized as valid.
[10052][HY000] Invalid argument. Client option 'non_existing_option' is not recognized as valid.
[10052][HY000] Invalid argument. Client option 'maxSize' does not support value -10.
[10052][HY000] Invalid argument. Client option 'maxSize' does not support value 0.
[10052][HY000] Invalid argument. Client option 'maxSize' does not support value 'eleven'.
[10052][HY000] Invalid argument. Client option 'maxIdleTime' does not support value 'incorrect_idle_time'.
[10052][HY000] Invalid argument. Client option 'maxIdleTime' does not support value -100.
[10052][HY000] Invalid argument. Client option 'maxIdleTime' does not support value true.
[10052][HY000] Invalid argument. Client option 'queueTimeOut' does not support value 'non_default_queue_time'.
[10052][HY000] Invalid argument. Client option 'queueTimeOut' does not support value -5000.
[10052][HY000] Invalid argument. Client option 'queueTimeOut' does not support value false.
done!%A
