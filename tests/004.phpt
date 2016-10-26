--TEST--
xmysqlnd table delete/where
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

        $schema = $nodeSession->getSchema("test");
        $table = $schema->getTable("test_table");

        $table->insert(["name", "age"])->values(["Sakila", 128])->values(["Oracila", 512])->execute();
        $table->insert(["name", "age"])->values(["SuperSakila", 256])->values(["SuperOracila", 1024])->execute();


        $table->delete()->where("name = 'Sakila'")->execute();

        dump_all_row($table);

        $table->delete()->where("name = 'bad_name'")->execute();

        dump_all_row($table);


        try{
            $table->delete()->where("name = SuperSakila")->execute();
        }catch(Exception $e) {
            print "exception!\n";
        }

        dump_all_row($table);

        $table->delete()->execute();

        dump_all_row($table);

        print "done!\n";
?>
--CLEAN--
<?php
    require("connect.inc");
    clean_test_db();
?>
--EXPECTF--0
array(3) {
  [0]=>
  array(2) {
    ["age"]=>
    int(512)
    ["name"]=>
    string(7) "Oracila"
  }
  [1]=>
  array(2) {
    ["age"]=>
    int(256)
    ["name"]=>
    string(11) "SuperSakila"
  }
  [2]=>
  array(2) {
    ["age"]=>
    int(1024)
    ["name"]=>
    string(12) "SuperOracila"
  }
}
array(3) {
  [0]=>
  array(2) {
    ["age"]=>
    int(512)
    ["name"]=>
    string(7) "Oracila"
  }
  [1]=>
  array(2) {
    ["age"]=>
    int(256)
    ["name"]=>
    string(11) "SuperSakila"
  }
  [2]=>
  array(2) {
    ["age"]=>
    int(1024)
    ["name"]=>
    string(12) "SuperOracila"
  }
}
exception!
array(3) {
  [0]=>
  array(2) {
    ["age"]=>
    int(512)
    ["name"]=>
    string(7) "Oracila"
  }
  [1]=>
  array(2) {
    ["age"]=>
    int(256)
    ["name"]=>
    string(11) "SuperSakila"
  }
  [2]=>
  array(2) {
    ["age"]=>
    int(1024)
    ["name"]=>
    string(12) "SuperOracila"
  }
}
bool(false)
done!
%a

