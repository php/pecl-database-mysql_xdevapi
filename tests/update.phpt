--TEST--
mysqlx update
--SKIPIF--
--FILE--
<?php
	require("connect.inc");

	function dump_all_row($table){
		$res = $table->select(['age', 'name'])->execute();
		$all_row = $res->fetchAll();
		var_dump($all_row);
	}

	$session = create_test_db();

	$schema = $session->getSchema($db);
	$table = $schema->getTable("test_table");

    $table->insert("name", "age")->values(["Sakila", 128],["Sakila", 512])->execute();
	$table->insert("name", "age")->values(["Oracila", 1024],["Sakila", 2048])->execute();
	$table->insert("name", "age")->values(["SuperSakila", 4096],["SuperOracila", 8192])->execute();
	$table->insert("name", "age")->values(["Oracila", 2000])->values(["Oracila", 3000])->execute();
	$table->insert("name", "age")->values(["Oracila", 1900])->values(["Oracila", 1800])->execute();

	$res = $table->update()->set('name', 'Alfonso')->where('name = :name and age > 2000')->bind(['name' => 'Oracila'])->execute();
	expect_eq($res->getAffectedItemsCount(), 1);
	$upd = $table->update()->orderBy('age desc')->set('age', 1)->set('name', 'Toddler');
	$res = $upd->where('age > :param1 and age < :param2')->bind(['param1' => 500, 'param2' => 1901])->limit(2)->execute();
	expect_eq($res->getAffectedItemsCount(), 2);
	$res = $table->select(['age', 'name'])->execute();
	$all_row = $res->fetchAll();

	expect_eq(count($all_row), 10);
	//Would be better to check them all...
	expect_eq($all_row[0]['name'], 'Sakila');
	expect_eq($all_row[0]['age'], 128);
	expect_eq($all_row[5]['name'], 'SuperOracila');
	expect_eq($all_row[5]['age'], 8192);
	expect_eq($all_row[9]['name'], 'Toddler');
	expect_eq($all_row[9]['age'], 1);

	fill_db_table_use_dup();

	$res = $table->update()->set('age',69)->where('age > 15 and age < 22')->limit(4)->orderby(['age asc','name desc'])->execute();
	expect_eq($res->getAffectedItemsCount(), 4);
	$res = $table->select(['name','age'])->where('age = 69')->execute()->fetchAll();
	expect_eq(count($res), 4);
	expect_eq($res[0]['name'], 'BRomy');
	expect_eq($res[0]['age'], 69);
	expect_eq($res[3]['name'], 'ERomy');
	expect_eq($res[3]['age'], 69);

        try{
                $res = $table->update()->set('age',50)->where('age > 15 and age < 100')->limit(2)->orderby(['fail asc','name desc'])->execute();
                test_step_failed();
        } catch( Exception $ex ) {
                test_step_ok();
        }

	verify_expectations();
	print "done!\n";
?>
--CLEAN--
<?php
	require("connect.inc");
	clean_test_db();
?>
--EXPECTF--
done!%A
