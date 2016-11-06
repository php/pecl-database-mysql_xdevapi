--TEST--
xmysqlnd update
--SKIPIF--
--FILE--
<?php
        require("connect.inc");

function dump_all_row($table){
    $res = $table->select(['age','name'])->execute();
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

	$table->update()->set('name','Alfonso')->where('name = :name and age > 2000')->bind(['name' => 'Oracila'])->execute();
	$upd = $table->update()->orderBy('age desc')->set('age', 1)->set('name','Toddler');
	$upd->where('age > :param1 and age < :param2')->bind(['param1' => 500, 'param2' => 1901])->limit(2)->execute();
	dump_all_row($table);

        print "done!\n";
?>
--CLEAN--
<?php
    require("connect.inc");
    clean_test_db();
?>
--EXPECTF--
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
    string(7) "Alfonso"
  }
  [8]=>
  array(2) {
    ["age"]=>
    int(1)
    ["name"]=>
    string(7) "Toddler"
  }
  [9]=>
  array(2) {
    ["age"]=>
    int(1)
    ["name"]=>
    string(7) "Toddler"
  }
}
done!
%a

