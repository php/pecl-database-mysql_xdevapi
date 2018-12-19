--TEST--
mysqlx collection fields
--SKIPIF--
--FILE--
<?php
require("connect.inc");

function find_with_fields($fields) {
	global $coll;
	echo "-- Input:\n";
	print_r($fields);
	$result = $coll
		->find('job like :job and age > :age')
		->bind(['job' => 'Programmatore', 'age' => 24])
		->fields($fields)
		->execute();
	echo "\n-- Output:\n";
	print_r($result->fetchAll());
	echo "_____________\n";
}

$session = create_test_db();
$schema = $session->getSchema($db);
$coll = $schema->getCollection($test_collection_name);

fill_db_collection($coll, true);

// see also: orabug #28803039 the fields() method seems odd

find_with_fields(['age','name']);
find_with_fields("[age,name] as x");
find_with_fields("[name,age,'any_text']");
find_with_fields("['name',age,'any_text','age']");
find_with_fields("[age,'age']");
find_with_fields("[name,age,job2]");


find_with_fields("nothing_to_see_here");
$coll->modify(true)->set("nothing_to_see_here", "something_to_see_here")->execute();
find_with_fields("nothing_to_see_here");
$coll->modify(true)->unset("nothing_to_see_here")->execute();
find_with_fields("nothing_to_see_here");

verify_expectations();
print "done!\n";
?>
--CLEAN--
<?php
require("connect.inc");
clean_test_db();
?>
--EXPECTF--
-- Input:
Array
(
    [0] => age
    [1] => name
)

-- Output:
Array
(
    [0] => Array
        (
            [age] => 25
            [name] => Carlo
        )

    [1] => Array
        (
            [age] => 27
            [name] => Alfredo
        )

)
_____________
-- Input:
[age,name] as x
-- Output:
Array
(
    [0] => Array
        (
            [x] => Array
                (
                    [0] => 25
                    [1] => Carlo
                )

        )

    [1] => Array
        (
            [x] => Array
                (
                    [0] => 27
                    [1] => Alfredo
                )

        )

)
_____________
-- Input:
[name,age,'any_text']
-- Output:
Array
(
    [0] => Array
        (
            [any_text] => Array
                (
                    [0] => Carlo
                    [1] => 25
                    [2] => any_text
                )

        )

    [1] => Array
        (
            [any_text] => Array
                (
                    [0] => Alfredo
                    [1] => 27
                    [2] => any_text
                )

        )

)
_____________
-- Input:
['name',age,'any_text','age']
-- Output:
Array
(
    [0] => Array
        (
            [age] => Array
                (
                    [0] => name
                    [1] => 25
                    [2] => any_text
                    [3] => age
                )

        )

    [1] => Array
        (
            [age] => Array
                (
                    [0] => name
                    [1] => 27
                    [2] => any_text
                    [3] => age
                )

        )

)
_____________
-- Input:
[age,'age']
-- Output:
Array
(
    [0] => Array
        (
            [age] => Array
                (
                    [0] => 25
                    [1] => age
                )

        )

    [1] => Array
        (
            [age] => Array
                (
                    [0] => 27
                    [1] => age
                )

        )

)
_____________
-- Input:
[name,age,job2]
-- Output:
Array
(
    [0] => Array
        (
            [job2] => Array
                (
                    [0] => Carlo
                    [1] => 25
                    [2] =>%s
                )

        )

    [1] => Array
        (
            [job2] => Array
                (
                    [0] => Alfredo
                    [1] => 27
                    [2] =>%s
                )

        )

)
_____________
-- Input:
nothing_to_see_here
-- Output:
Array
(
    [0] => Array
        (
            [nothing_to_see_here] =>%s
        )

    [1] => Array
        (
            [nothing_to_see_here] =>%s
        )

)
_____________
-- Input:
nothing_to_see_here
-- Output:
Array
(
    [0] => Array
        (
            [nothing_to_see_here] => something_to_see_here
        )

    [1] => Array
        (
            [nothing_to_see_here] => something_to_see_here
        )

)
_____________
-- Input:
nothing_to_see_here
-- Output:
Array
(
    [0] => Array
        (
            [nothing_to_see_here] =>%s
        )

    [1] => Array
        (
            [nothing_to_see_here] =>%s
        )

)
_____________
done!%A
