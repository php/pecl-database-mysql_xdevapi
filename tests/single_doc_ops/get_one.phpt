--TEST--
mysqlx collection single doc ops - getOne
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
// getOne
$doc = $coll->getOne(0);
expect_null($doc);

$doc = $coll->getOne(5);
verify_doc($doc, 5, "Carlo", 25, "Programmatore");

$doc = $coll->getOne('31');
expect_null($doc);

$doc = $coll->getOne('13');
verify_doc($doc, 13, "Alessandra", 15, "Barista");

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
