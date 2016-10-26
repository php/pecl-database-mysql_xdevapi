--TEST--
xmysqlnd getTable with wrong table / insert
--SKIPIF--
--FILE--
<?php
        require("connect.inc");

        $test = "000";

        $nodeSession = create_test_db();

        $schema = $nodeSession->getSchema("test");
        $table = $schema->getTable("wrong_table");

        #existsInDatabase will raise an exception if the implementation is missing
        try{
            $table_exist = $table->existsInDatabase();
            if($table_exist == false)
                $test[0] = "1";
        }catch(Exception $e){
            #no nothing
        }

        try{
            $table->insert(["name", "age"])->values(["Jackie", 256])->execute();
        }catch(Exception $e){
            $test[1] = "1";
        }
        $table = $schema->getTable("");
        if(is_bool($table) && $table == false)
            $test[2] = "1";

        var_dump($test);
        print "done!\n";
?>
--CLEAN--
<?php
    require("connect.inc");
    clean_test_db();
?>
--EXPECTF--
string(3) "111"
done!
%a

