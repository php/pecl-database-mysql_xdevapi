--TEST--
mysqlx collection modify.arrayInsert vs json documents
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
$coll->modify('name = "Giulio"')
	->set('races', '{"2018" : [ "Australia", "China" ], "2019": [ "Italy", "Belgium" ] }')
	->execute();

$coll->modify('name = "Giulio"')
	->set('teams', '["Red Bull", "Renault"]')->execute();

$coll->modify('name = "Giulio"')
	->set('duties', ["Driver"])->execute();

// insert to arrays
$coll->modify('name = "Giulio"')
	->arrayInsert('$.races."2018"[1]', "Hungary")
	->execute();

$coll->modify('name = "Giulio"')
	->arrayInsert('$.races."2019"[0]', '{"testing" : ["Spain", "Canada"]}')
	->execute();

$coll->modify('name = "Giulio"')
	->arrayInsert('$.races."2019"[1]', '{"tyres" : "Pirelli"}')
	->execute();

$coll->modify('name = "Giulio"')
	->arrayInsert('teams[1]', 'McLaren')->execute();

$coll->modify('name = "Giulio"')
	->arrayInsert('teams[3]', '["Williams", "Ferrari"]')->execute();

$coll->modify('name = "Giulio"')
	->arrayInsert('teams[2]', ["Mercedes", "Haas"])->execute();

$coll->modify('name = "Giulio"')
	->arrayInsert('duties[0]', "Mechanic")->execute();

$coll->modify('name = "Giulio"')
	->arrayInsert('duties[0]', '{"others" : ["Tester", "Celebrity"]}')->execute();

print_data("Giulio");

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
            [_id] => 10
            [age] => 29
            [job] => Disoccupato
            [name] => Giulio
            [races] => Array
                (
                    [2018] => Array
                        (
                            [0] => Australia
                            [1] => Hungary
                            [2] => China
                        )

                    [2019] => Array
                        (
                            [0] => Array
                                (
                                    [testing] => Array
                                        (
                                            [0] => Spain
                                            [1] => Canada
                                        )

                                )

                            [1] => Array
                                (
                                    [tyres] => Pirelli
                                )

                            [2] => Italy
                            [3] => Belgium
                        )

                )

            [teams] => Array
                (
                    [0] => Red Bull
                    [1] => McLaren
                    [2] => Array
                        (
                            [0] => Mercedes
                            [1] => Haas
                        )

                    [3] => Renault
                    [4] => Array
                        (
                            [0] => Williams
                            [1] => Ferrari
                        )

                )

            [duties] => Array
                (
                    [0] => Array
                        (
                            [others] => Array
                                (
                                    [0] => Tester
                                    [1] => Celebrity
                                )

                        )

                    [1] => Mechanic
                    [2] => Driver
                )

            [ordinal] => 10
        )

)
done!%A
