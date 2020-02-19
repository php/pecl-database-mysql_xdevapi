--TEST--
mysqlx collection modify.patch vs json documents
--SKIPIF--
--INI--
error_reporting=E_ALL
--FILE--
<?php
require(__DIR__."/crud_json_utils.inc");

$session = create_test_db();
$schema = $session->getSchema($db);
$coll = fill_test_collection();

// init data
$coll->modify('name = "Monica"')
	->set('cities', '{"Poland" : "Gdansk", "Germany" : [ "Munich", "Stuttgart" ], "Mexico": ["Guadalajara"] }')
	->execute();

$coll->modify('name = "Monica"')
	->set('hobby', '["dancing", "cooking"]')->execute();

$coll->modify('name = "Monica"')
	->set('cats', ["Pimpus", "Sadelko"])->execute();

$coll->modify('name = "Monica"')
	->set('city', 'Cracow')->execute();

print_data("Monica");

// patch data
$coll->modify('name = "Monica"')
	->patch('{ "cities" : { "Portugal" : "Porto", Poland: ["Wroclaw"] } }')
	->execute();

$coll->modify('name = "Monica"')
	->patch('{"hobby" : ["celebrities", "quantum mechanics"]}')->execute();

$coll->modify('name = "Monica"')
	->patch('{"cats" : "Mruczek"}')->execute();

$coll->modify('name = "Monica"')
	->patch('{"city" : "Poznan"}')->execute();

$coll->modify('name = "Monica"')
	->patch('{"age" : 22}')->execute();

print_data("Monica");

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
            [_id] => 9
            [age] => 35
            [job] => Ballerino
            [cats] => Array
                (
                    [0] => Pimpus
                    [1] => Sadelko
                )

            [city] => Cracow
            [name] => Monica
            [hobby] => Array
                (
                    [0] => dancing
                    [1] => cooking
                )

            [cities] => Array
                (
                    [Mexico] => Array
                        (
                            [0] => Guadalajara
                        )

                    [Poland] => Gdansk
                    [Germany] => Array
                        (
                            [0] => Munich
                            [1] => Stuttgart
                        )

                )

            [ordinal] => 9
        )

)
Array
(
    [0] => Array
        (
            [_id] => 9
            [age] => 22
            [job] => Ballerino
            [cats] => Mruczek
            [city] => Poznan
            [name] => Monica
            [hobby] => Array
                (
                    [0] => celebrities
                    [1] => quantum mechanics
                )

            [cities] => Array
                (
                    [Mexico] => Array
                        (
                            [0] => Guadalajara
                        )

                    [Poland] => Array
                        (
                            [0] => Wroclaw
                        )

                    [Germany] => Array
                        (
                            [0] => Munich
                            [1] => Stuttgart
                        )

                    [Portugal] => Porto
                )

            [ordinal] => 9
        )

)
done!%A
