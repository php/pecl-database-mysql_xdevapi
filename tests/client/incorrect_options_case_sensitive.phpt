--TEST--
mysqlx client fail due to option names with incorrect case-sensitive
--SKIPIF--
--INI--
error_reporting=E_ALL
default_socket_timeout=4
--FILE--
<?php
require_once(__DIR__."/../connect.inc");
require_once(__DIR__."/client_utils.inc");

$pooling_options = '{
	"ENABLED": true,
  	"maxSize": 10
}';
assert_client_fail_with_options($pooling_options);

$pooling_options = '{
	"eNaBlEd": false
}';
assert_client_fail_with_options($pooling_options);


$pooling_options = '{
	"enabled": true,
  	"MaxSize": 10
}';
assert_client_fail_with_options($pooling_options);

$pooling_options = '{
  	"maxsize": 10,
  	"queueTimeOut": 1000
}';
assert_client_fail_with_options($pooling_options);


$pooling_options = '{
  	"mAXiDLEtIME": 3600,
  	"enabled": false
}';
assert_client_fail_with_options($pooling_options);

$pooling_options = '{
  	"queueTimeOut": 1000,
  	"maXidlEtimE": 1500,
	"enabled": true
}';
assert_client_fail_with_options($pooling_options);


$pooling_options = '{
  	"maxSize": 25,
  	"QUEUETIMEOUT": 1000
}';
assert_client_fail_with_options($pooling_options);

$pooling_options = '{
  	"Queuetimeout": 100
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
[10052][HY000] Invalid argument. Client option 'ENABLED' is not recognized as valid.
[10052][HY000] Invalid argument. Client option 'eNaBlEd' is not recognized as valid.
[10052][HY000] Invalid argument. Client option 'MaxSize' is not recognized as valid.
[10052][HY000] Invalid argument. Client option 'maxsize' is not recognized as valid.
[10052][HY000] Invalid argument. Client option 'mAXiDLEtIME' is not recognized as valid.
[10052][HY000] Invalid argument. Client option 'maXidlEtimE' is not recognized as valid.
[10052][HY000] Invalid argument. Client option 'QUEUETIMEOUT' is not recognized as valid.
[10052][HY000] Invalid argument. Client option 'Queuetimeout' is not recognized as valid.
done!%A
