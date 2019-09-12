--TEST--
mysqlx collection modify.set vs json documents
--SKIPIF--
--INI--
error_reporting=E_ALL
--FILE--
<?php
require(__DIR__."/crud_json_utils.inc");

$session = create_test_db();
$schema = $session->getSchema($db);
$coll = fill_test_collection();

$coll->modify('name = "Marco"')
	->set('tasks', '{"orabug" : [2, 5, 7], "wl" : [8, 10, 12], "log-monkey" : "yes" }')
	->execute();

$coll->modify('name = "Marco"')
	->set('projects', '["node.js", "php", "python"]')->execute();

$coll->modify('name = "Marco"')
	->set('duties', ["coding", "log-monkey"])->execute();

$coll->modify('name = "Marco"')
	->set('room', 1313)->execute();

print_data("Marco");

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
            [_id] => 1
            [age] => 19
            [job] => Programmatore
            [name] => Marco
            [room] => 1313
            [tasks] => Array
                (
                    [wl] => Array
                        (
                            [0] => 8
                            [1] => 10
                            [2] => 12
                        )

                    [orabug] => Array
                        (
                            [0] => 2
                            [1] => 5
                            [2] => 7
                        )

                    [log-monkey] => yes
                )

            [duties] => Array
                (
                    [0] => coding
                    [1] => log-monkey
                )

            [ordinal] => 1
            [projects] => Array
                (
                    [0] => node.js
                    [1] => php
                    [2] => python
                )

        )

)
done!%A
