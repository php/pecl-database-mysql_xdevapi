--TEST--
mysqlx table delete/limit/orderBy
--SKIPIF--
--FILE--
<?php
	require("connect.inc");

	function dump_all_row($table) {
		$res = $table->select(['age', 'name'])->execute();
		$all_row = $res->fetchAll();
		var_dump($all_row);
	}

	$nodeSession = create_test_db();

	$schema = $nodeSession->getSchema($db);
	$table = $schema->getTable("test_table");

	$table->insert(["name", "age"])->values(["Sakila", 128])->values(["Sakila", 512])->execute();
	$table->insert(["name", "age"])->values(["Oracila", 1024])->values(["Sakila", 2048])->execute();
	$table->insert(["name", "age"])->values(["SuperSakila", 4096])->values(["SuperOracila", 8192])->execute();
	$table->insert(["name", "age"])->values(["Oracila", 2000])->values(["Oracila", 3000])->execute();
	$table->insert(["name", "age"])->values(["Oracila", 1900])->values(["Oracila", 1800])->execute();

	try {
	    $table->delete()->where("name = :name")->orderby("id DESC")->limit(2)->bind(['name' => 'Sakila'])->execute();
	} catch(Exception $e) {
		print "exception!\n";
	}

	dump_all_row($table);

	$table->delete()->where("name = :name")->orderby("age DESC")->limit(2)->bind(['name' => 'Sakila'])->execute();

	dump_all_row($table);

	try {
	    $table->delete()->where("name = :name")->orderby("age DESC")->limit(-1)->bind(['name' => 'Oracila'])->execute();
	} catch(Exception $e) {
		print "exception!\n";
	}

	$table->delete()->where("name = :name")->orderby("age ASC")->limit(3)->bind(['name' => 'Oracila'])->execute();

	dump_all_row($table);

	print "done!\n";
?>
--CLEAN--
<?php
    require("connect.inc");
    clean_test_db();
?>
--EXPECTF--
exception!
array(10) {
  [0]=>
  array(2) {
    ["age"]=>
    int(128)
    ["name"]=>
    string(6) "Sakila"
  }
  [1]=>
  array(2) {
    ["age"]=>
    int(512)
    ["name"]=>
    string(6) "Sakila"
  }
  [2]=>
  array(2) {
    ["age"]=>
    int(1024)
    ["name"]=>
    string(7) "Oracila"
  }
  [3]=>
  array(2) {
    ["age"]=>
    int(2048)
    ["name"]=>
    string(6) "Sakila"
  }
  [4]=>
  array(2) {
    ["age"]=>
    int(4096)
    ["name"]=>
    string(11) "SuperSakila"
  }
  [5]=>
  array(2) {
    ["age"]=>
    int(8192)
    ["name"]=>
    string(12) "SuperOracila"
  }
  [6]=>
  array(2) {
    ["age"]=>
    int(2000)
    ["name"]=>
    string(7) "Oracila"
  }
  [7]=>
  array(2) {
    ["age"]=>
    int(3000)
    ["name"]=>
    string(7) "Oracila"
  }
  [8]=>
  array(2) {
    ["age"]=>
    int(1900)
    ["name"]=>
    string(7) "Oracila"
  }
  [9]=>
  array(2) {
    ["age"]=>
    int(1800)
    ["name"]=>
    string(7) "Oracila"
  }
}
array(8) {
  [0]=>
  array(2) {
    ["age"]=>
    int(128)
    ["name"]=>
    string(6) "Sakila"
  }
  [1]=>
  array(2) {
    ["age"]=>
    int(1024)
    ["name"]=>
    string(7) "Oracila"
  }
  [2]=>
  array(2) {
    ["age"]=>
    int(4096)
    ["name"]=>
    string(11) "SuperSakila"
  }
  [3]=>
  array(2) {
    ["age"]=>
    int(8192)
    ["name"]=>
    string(12) "SuperOracila"
  }
  [4]=>
  array(2) {
    ["age"]=>
    int(2000)
    ["name"]=>
    string(7) "Oracila"
  }
  [5]=>
  array(2) {
    ["age"]=>
    int(3000)
    ["name"]=>
    string(7) "Oracila"
  }
  [6]=>
  array(2) {
    ["age"]=>
    int(1900)
    ["name"]=>
    string(7) "Oracila"
  }
  [7]=>
  array(2) {
    ["age"]=>
    int(1800)
    ["name"]=>
    string(7) "Oracila"
  }
}
exception!
array(5) {
  [0]=>
  array(2) {
    ["age"]=>
    int(128)
    ["name"]=>
    string(6) "Sakila"
  }
  [1]=>
  array(2) {
    ["age"]=>
    int(4096)
    ["name"]=>
    string(11) "SuperSakila"
  }
  [2]=>
  array(2) {
    ["age"]=>
    int(8192)
    ["name"]=>
    string(12) "SuperOracila"
  }
  [3]=>
  array(2) {
    ["age"]=>
    int(2000)
    ["name"]=>
    string(7) "Oracila"
  }
  [4]=>
  array(2) {
    ["age"]=>
    int(3000)
    ["name"]=>
    string(7) "Oracila"
  }
}
done!%A
