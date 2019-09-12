--TEST--
mysqlx collection modify.replace vs json documents
--SKIPIF--
--INI--
error_reporting=E_ALL
--FILE--
<?php
require(__DIR__."/crud_json_utils.inc");

function replace_data() {
	global $coll;
	$coll->modify('name = "Antonella"')
		->replace('travels', '{"Portugal" : ["Lisbon", "Porto"], "Germany" : "Berlin", "Poland" : ["Warsaw"], others: 10 }')
		->execute();

	$coll->modify('name = "Antonella"')
		->replace('grades', '["good", "very good", "excellent"]')->execute();

	$coll->modify('name = "Antonella"')
		->replace('courses', ["English", "Mathematics", "French", "Physics"])->execute();

	$coll->modify('name = "Antonella"')
		->replace('year_of_study', 4)->execute();
}

function set_data() {
	global $coll;
	$coll->modify('name = "Antonella"')
		->set('travels', '{"Japan" : "Tokio", "USA" : [ "Washington", "New York" ], another: 5 }')
		->execute();

	$coll->modify('name = "Antonella"')
		->set('grades', '["bad", "very bad", "even worse"]')->execute();

	$coll->modify('name = "Antonella"')
		->set('courses', ["Biology", "Chemistry", "Dutch"])->execute();

	$coll->modify('name = "Antonella"')
		->set('year_of_study', 2)->execute();
}

$session = create_test_db();
$schema = $session->getSchema($db);
$coll = fill_test_collection();

// nothing should be changed, there are no related keys
replace_data();
print_data("Antonella");

// add 'missing' data
set_data($coll);
print_data("Antonella");

// this time data should be replaced
replace_data($coll);
print_data("Antonella");

verify_expectations();
print "done!\n";
?>
--CLEAN--
<?php
require(__DIR__."/crud_json_utils.inc");
clean_test_db();
?>
--EXPECTF--
Array
(
    [0] => Array
        (
            [_id] => 8
            [age] => 42
            [job] => Studente
            [name] => Antonella
            [ordinal] => 8
        )

)
Array
(
    [0] => Array
        (
            [_id] => 8
            [age] => 42
            [job] => Studente
            [name] => Antonella
            [grades] => Array
                (
                    [0] => bad
                    [1] => very bad
                    [2] => even worse
                )

            [courses] => Array
                (
                    [0] => Biology
                    [1] => Chemistry
                    [2] => Dutch
                )

            [ordinal] => 8
            [travels] => Array
                (
                    [USA] => Array
                        (
                            [0] => Washington
                            [1] => New York
                        )

                    [Japan] => Tokio
                    [another] => 5
                )

            [year_of_study] => 2
        )

)
Array
(
    [0] => Array
        (
            [_id] => 8
            [age] => 42
            [job] => Studente
            [name] => Antonella
            [grades] => Array
                (
                    [0] => good
                    [1] => very good
                    [2] => excellent
                )

            [courses] => Array
                (
                    [0] => English
                    [1] => Mathematics
                    [2] => French
                    [3] => Physics
                )

            [ordinal] => 8
            [travels] => Array
                (
                    [Poland] => Array
                        (
                            [0] => Warsaw
                        )

                    [others] => 10
                    [Germany] => Berlin
                    [Portugal] => Array
                        (
                            [0] => Lisbon
                            [1] => Porto
                        )

                )

            [year_of_study] => 4
        )

)
done!%A
