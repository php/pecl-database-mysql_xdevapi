--TEST--
orabug #30226250: some of Collection.Modify ops don't support JSON documents
--SKIPIF--
--INI--
error_reporting=0
--FILE--
<?php
require(__DIR__."/../connect.inc");

$session = create_test_db();

$schema = $session->getSchema($db);
$coll = $schema->getCollection($test_collection_name);

$coll->add(
	'{"name": "Stuart",'
	.'"grades": [{"subject": "English", "mark": 90},{"subject": "Math", "mark": 87}]}')
	->execute();

$res = $coll->find('name = "Stuart"')->execute();

$coll->modify('name = "Stuart"')
	->arrayAppend('grades', '{"subject": "History", "mark": 30}')->execute();

$str1 = '{"subject": "German", "mark": 40}';
$coll->modify('name = "Stuart"')->arrayAppend('grades', $str1)->execute();

// Let's create an array and then put it in the collection as is
$arr1 = array("subject" => "Geography", "mark" => 78);
$coll->modify('name = "Stuart"')->arrayAppend('grades', $arr1)->execute();

// Create another array but this time json_encode it before putting it in the collection
$arr2 = array("subject" => "Chemistry", "mark" => 11);
$arr2_enc = json_encode($arr2);
$coll->modify('name = "Stuart"')->arrayAppend('grades', $arr2_enc)->execute();

$res = $coll->find('name = "Stuart"')->execute();
print_r($res->fetchAll());

verify_expectations();
print "done!\n";
?>
--CLEAN--
<?php
require(__DIR__."/../connect.inc");
clean_test_db();
?>
--EXPECTF--
Array
(
    [0] => Array
        (
            [_id] => %s
            [name] => Stuart
            [grades] => Array
                (
                    [0] => Array
                        (
                            [mark] => 90
                            [subject] => English
                        )

                    [1] => Array
                        (
                            [mark] => 87
                            [subject] => Math
                        )

                    [2] => Array
                        (
                            [mark] => 30
                            [subject] => History
                        )

                    [3] => Array
                        (
                            [mark] => 40
                            [subject] => German
                        )

                    [4] => Array
                        (
                            [mark] => 78
                            [subject] => Geography
                        )

                    [5] => Array
                        (
                            [mark] => 11
                            [subject] => Chemistry
                        )

                )

        )

)
done!%A
