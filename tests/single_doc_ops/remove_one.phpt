--TEST--
mysqlx collection single doc ops - removeOne
--SKIPIF--
--INI--
error_reporting=0
--FILE--
<?php
require_once(__DIR__."/../connect.inc");
require_once(__DIR__."/single_doc_utils.inc");

$session = create_test_db();
$coll = fill_test_collection(true);

// ----------------------------------------------------------------------
// removeOne

expect_doc(25, "Sergio", 65, "Direzione aziendale");
$res = $coll->removeOne('25');
verify_result($res, '25', 1);
expect_null_doc(25);

$res = $coll->removeOne('25');
verify_result($res, null, 0);
expect_null_doc(25);

expect_null_doc(1001);
$res = $coll->removeOne('1001');
verify_result($res, null, 0);
expect_null_doc(1001);

verify_expectations();
print "done!\n";
?>
--CLEAN--
<?php
	require_once(__DIR__."/../connect.inc");
	clean_test_db();
?>
--EXPECTF--
done!%A
