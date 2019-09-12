--TEST--
mysqlx collection modify.arrayAppend vs json documents
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
$coll->modify('name = "Lucia"')
	->set('skills', '{"programming" : [ "Golang", "Python" ], "languages": [ "English", "Italian" ] }')
	->execute();

$coll->modify('name = "Lucia"')
	->set('companies', '["Oracle", "MongoDB"]')->execute();

$coll->modify('name = "Lucia"')
	->set('duties', ["DevOps"])->execute();

// append to arrays
$coll->modify('name = "Lucia"')
	->arrayAppend('$.skills.programming', [ "Node.js", "PHP" ])
	->execute();

$coll->modify('name = "Lucia"')
	->arrayAppend('$.skills.languages', '[ "Polish", "Silesian" ]')
	->execute();

$coll->modify('name = "Lucia"')
	->arrayAppend('companies', '{"MySQL": "Connector-PHP"}')->execute();

$coll->modify('name = "Lucia"')
	->arrayAppend('companies', 'Percona')->execute();

$coll->modify('name = "Lucia"')
	->arrayAppend('duties', "pb2")->execute();

$coll->modify('name = "Lucia"')
	->arrayAppend('duties', '{"mysqlaas" : "operator"}')->execute();

$coll->modify('name = "Lucia"')
	->arrayAppend('duties', ["code reviews", "releases"])->execute();

print_data("Lucia");

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
            [_id] => 11
            [age] => 47
            [job] => Barista
            [name] => Lucia
            [duties] => Array
                (
                    [0] => DevOps
                    [1] => pb2
                    [2] => Array
                        (
                            [mysqlaas] => operator
                        )

                    [3] => Array
                        (
                            [0] => code reviews
                            [1] => releases
                        )

                )

            [skills] => Array
                (
                    [languages] => Array
                        (
                            [0] => English
                            [1] => Italian
                            [2] => Array
                                (
                                    [0] => Polish
                                    [1] => Silesian
                                )

                        )

                    [programming] => Array
                        (
                            [0] => Golang
                            [1] => Python
                            [2] => Array
                                (
                                    [0] => Node.js
                                    [1] => PHP
                                )

                        )

                )

            [ordinal] => 11
            [companies] => Array
                (
                    [0] => Oracle
                    [1] => MongoDB
                    [2] => Array
                        (
                            [MySQL] => Connector-PHP
                        )

                    [3] => Percona
                )

        )

)
done!%A
