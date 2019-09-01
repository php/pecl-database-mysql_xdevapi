--TEST--
mysqlx table delete/where
--SKIPIF--
--FILE--
<?php
	require("connect.inc");

	function dump_all_row($table) {
		$res = $table->select(['age', 'name'])->execute();
		$all_row = $res->fetchAll();
		var_dump($all_row);
	}

	$session = create_test_db();

	fill_db_table();

	$schema = $session->getSchema($db);
	$table = $schema->getTable($test_table_name);

	$table->delete()->where("name in ('Romy', 'Caspian', 'Olympia', 'Mamie') and age > :age_limit")->bind(['age_limit' => 13])->execute();
	$table->delete()->where("name = 'bad_name'")->limit(1)->execute(); //Shall do nothing
	$table->delete()->orderby('age desc')->where('age < 20 and age > 12 and name != :name')->bind(['name' => 'Tierney'])->limit(2)->execute();
	dump_all_row($table);

	print "done!\n";
?>
--CLEAN--
<?php
	require("connect.inc");
	clean_test_db();
?>
--EXPECTF--0
array(7) {
  [0]=>
  array(2) {
    ["age"]=>
    int(11)
    ["name"]=>
    string(5) "Mamie"
  }
  [1]=>
  array(2) {
    ["age"]=>
    int(11)
    ["name"]=>
    string(7) "Eulalia"
  }
  [2]=>
  array(2) {
    ["age"]=>
    int(12)
    ["name"]=>
    string(5) "Polly"
  }
  [3]=>
  array(2) {
    ["age"]=>
    int(12)
    ["name"]=>
    string(5) "Rufus"
  }
  [4]=>
  array(2) {
    ["age"]=>
    int(13)
    ["name"]=>
    string(7) "Cassidy"
  }
  [5]=>
  array(2) {
    ["age"]=>
    int(14)
    ["name"]=>
    string(3) "Lev"
  }
  [6]=>
  array(2) {
    ["age"]=>
    int(15)
    ["name"]=>
    string(7) "Tierney"
  }
}
done!%A
