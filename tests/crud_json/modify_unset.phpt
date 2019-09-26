--TEST--
mysqlx collection modify.unset vs json documents
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
$coll->modify('name = "Mariangela"')
	->set('tracks', '{"2017" : [ "Russia", "Singapure" ], "2018": [ "Italy", "Belgium", "Great Britain", "Germany" ] }')
	->execute();

$coll->modify('name = "Mariangela"')
	->set('seasons', '{"2017" : { "points" : 40, "classified" : 8 }, "2018": { "points" : 45, "classified" : 7 } }')
	->execute();

$coll->modify('name = "Mariangela"')
	->set('teams', '["Alfa Romeo", "Toro Rosso", "Haas", "Racing Point", "Ferrari"]')->execute();

$coll->modify('name = "Mariangela"')
	->set('duties', ["Driver", "Mechanic", "Tester", "Media", "Pit-lane"])->execute();

print_data("Mariangela");

// unset data
$coll->modify('name = "Mariangela"')
	->unset('$.tracks."2017"')
	->execute();

$coll->modify('name = "Mariangela"')
	->unset('$.tracks."2018"[1]')
	->execute();

$coll->modify('name = "Mariangela"')
	->unset('$.seasons."2018"')
	->execute();

$coll->modify('name = "Mariangela"')
	->unset('$.seasons."2017".points')
	->execute();

$coll->modify('name = "Mariangela"')
	->unset('teams[3]')->execute();

$coll->modify('name = "Mariangela"')
	->unset('duties[1]')->execute();

$coll->modify('name = "Mariangela"')
	->unset('duties[3]')->execute();

$coll->modify('name = "Mariangela"')
	->unset('age')->execute();

$coll->modify('name = "Mariangela"')
	->unset('job')->execute();

print_data("Mariangela");

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
            [_id] => 6
            [age] => 41
            [job] => Programmatrice
            [name] => Mariangela
            [teams] => Array
                (
                    [0] => Alfa Romeo
                    [1] => Toro Rosso
                    [2] => Haas
                    [3] => Racing Point
                    [4] => Ferrari
                )

            [duties] => Array
                (
                    [0] => Driver
                    [1] => Mechanic
                    [2] => Tester
                    [3] => Media
                    [4] => Pit-lane
                )

            [tracks] => Array
                (
                    [2017] => Array
                        (
                            [0] => Russia
                            [1] => Singapure
                        )

                    [2018] => Array
                        (
                            [0] => Italy
                            [1] => Belgium
                            [2] => Great Britain
                            [3] => Germany
                        )

                )

            [ordinal] => 6
            [seasons] => Array
                (
                    [2017] => Array
                        (
                            [points] => 40
                            [classified] => 8
                        )

                    [2018] => Array
                        (
                            [points] => 45
                            [classified] => 7
                        )

                )

        )

)
Array
(
    [0] => Array
        (
            [_id] => 6
            [name] => Mariangela
            [teams] => Array
                (
                    [0] => Alfa Romeo
                    [1] => Toro Rosso
                    [2] => Haas
                    [3] => Ferrari
                )

            [duties] => Array
                (
                    [0] => Driver
                    [1] => Tester
                    [2] => Media
                )

            [tracks] => Array
                (
                    [2018] => Array
                        (
                            [0] => Italy
                            [1] => Great Britain
                            [2] => Germany
                        )

                )

            [ordinal] => 6
            [seasons] => Array
                (
                    [2017] => Array
                        (
                            [classified] => 8
                        )

                )

        )

)
done!%A
