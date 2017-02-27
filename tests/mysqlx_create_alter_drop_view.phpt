--TEST--
mysqlx create/alter/drop view
--SKIPIF--
--FILE--
<?php
	require("connect.inc");

	$counter = 0;

	function print_separator() {
		global $counter;
		echo "---- ", $counter++, " ----", PHP_EOL;
	}

	function dump_view($columns) {
		global $schema;
		global $test_view_name;

		$view = $schema->getTable($test_view_name);
		expect_true($view->isView());

		$res = $view->select($columns)->execute();
		$all_rows = $res->fetchAll();

		print_separator();
		var_dump($all_rows);
	}

	function drop_view() {
		global $schema;
		global $test_view_name;

		$schema->dropView($test_view_name)->execute();

		$view = $schema->getTable($test_view_name);
		expect_false($view->existsInDatabase());
	}

	// -------------
	// init
	// -------------
	$nodeSession = create_test_db();
	fill_db_table();
	$schema = $nodeSession->getSchema($db);

	$table = $schema->getTable($test_table_name);
	expect_false($table->isView());

	$coll_as_table = $schema->getCollectionAsTable($test_collection_name);
	expect_false($coll_as_table->isView());

	$view = create_test_view($nodeSession);
	expect_true($view->isView());

	// -------------
	// getTables
	// -------------
	function assert_table($name, $is_view) {
		global $tables;
		expect_true($tables[$name]->existsInDatabase());
		expect_eq($tables[$name]->getName(), $name);
		expect_eq($tables[$name]->isView(), $is_view);
	}

	$tables = $schema->getTables();
	assert_table($test_table_name, false);
	assert_table($test_view_name, true);

	drop_view();

	// -------------
	// common options #0
	// -------------
	$select = $table
		->select('name', 'age')
		->where('(:age_min <= age) and (age < :age_max)')
		->bind(['age_min' => 12, 'age_max' => 14])
		->orderBy('age desc');
	$schema->createView($test_view_name, false)
		->definer('CURRENT_USER')
		->algorithm('UNDEFINED')
		->security('INVOKER')
		->withCheckOption('LOCAL')
		->columns(['xname', 'age'])
		->definedAs($select)
		->execute();
	dump_view(['xname', 'age']);


	$select = $table
		->select('name', 'age')
		->where('(:age_min <= age) and (age <= :age_max)')
		->bind(['age_min' => 14, 'age_max' => 16])
		->orderBy('age asc');
	$schema->alterView($test_view_name)
		->definer('current_user')
		->algorithm('undefined')
		->security('invoker')
		->withCheckOption('local')
		->columns(['name', 'xage'])
		->definedAs($select)
		->execute();
	dump_view(['name', 'xage']);

	drop_view();

	// -------------
	// common options #1
	// -------------
	$select = $table
		->select('name', 'age')
		->where('age =  13')
		->orderBy('name asc');
	$schema->createView($test_view_name, false)
		->definer('root@localhost')
		->algorithm('MERGE')
		->security('DEFINER')
		->withCheckOption('CASCADED')
		->columns(['vname', 'vage'])
		->definedAs($select)
		->execute();
	dump_view('vname');


	$schema->alterView($test_view_name)
		->definer('root@localhost')
		->algorithm('temptable')
		->security('definer')
		->columns(['vname', 'vage'])
		->definedAs($select)
		->execute();
	dump_view('vage');

	// -------------
	// replace view
	// -------------
	$select = $table
		->select('name')
		->where('name = "Eulalia"');
	try {
		$schema->createView($test_view_name, false)
			->algorithm('Merge')
			->security('Invoker')
			->withCheckOption('Cascaded')
			->columns('zname')
			->definedAs($select)
			->execute();
		test_step_failed();
	} catch (Exception $e) {
		print_separator();
		print "expected createView failure ".$e->getCode()." : ".$e->getMessage().PHP_EOL;
	}
	$schema->createView($test_view_name, true)
		->columns('yname')
		->definedAs($select)
		->execute();
	dump_view('yname');


	$select = $table
		->select('age')
		->where('name = "Eulalia"');
	$schema->alterView($test_view_name)
		->definer('root@localhost')
		->algorithm('Temptable')
		->security('Definer')
		//->withCheckOption('Local')
		->columns('age')
		->definedAs($select)
		->execute();
	dump_view(['age']);
	drop_view();

	// -------------
	// drop view
	// -------------
	try {
		$schema->dropView($test_view_name)->execute();
		test_step_failed();
	} catch (Exception $e) {
		print_separator();
		print "expected dropView failure ".$e->getCode()." : ".$e->getMessage().PHP_EOL;
	}

	try {
		$schema->dropView($test_view_name)->ifExists()->execute();
	} catch (Exception $e) {
		print "unexpected dropView failure ".$e->getCode()." : ".$e->getMessage().PHP_EOL;
		test_step_failed();
	}

	$view = $schema->getTable($test_view_name);
	expect_false($view->existsInDatabase());
	expect_false($view->isView());

	print "done!\n";
?>
--CLEAN--
<?php
	require("connect.inc");
	clean_test_db();
?>
--EXPECTF--
---- 0 ----
array(3) {
  [0]=>
  array(2) {
    ["xname"]=>
    string(7) "Cassidy"
    ["age"]=>
    int(13)
  }
  [1]=>
  array(2) {
    ["xname"]=>
    string(5) "Polly"
    ["age"]=>
    int(12)
  }
  [2]=>
  array(2) {
    ["xname"]=>
    string(5) "Rufus"
    ["age"]=>
    int(12)
  }
}
---- 1 ----
array(5) {
  [0]=>
  array(2) {
    ["name"]=>
    string(7) "Olympia"
    ["xage"]=>
    int(14)
  }
  [1]=>
  array(2) {
    ["name"]=>
    string(3) "Lev"
    ["xage"]=>
    int(14)
  }
  [2]=>
  array(2) {
    ["name"]=>
    string(7) "Tierney"
    ["xage"]=>
    int(15)
  }
  [3]=>
  array(2) {
    ["name"]=>
    string(7) "Octavia"
    ["xage"]=>
    int(15)
  }
  [4]=>
  array(2) {
    ["name"]=>
    string(6) "Vesper"
    ["xage"]=>
    int(16)
  }
}
---- 2 ----
array(1) {
  [0]=>
  array(1) {
    ["vname"]=>
    string(7) "Cassidy"
  }
}
---- 3 ----
array(1) {
  [0]=>
  array(1) {
    ["vage"]=>
    int(13)
  }
}
---- 4 ----
expected createView failure 10000 : [HY000] Couldn't fetch data
---- 5 ----
array(1) {
  [0]=>
  array(1) {
    ["yname"]=>
    string(7) "Eulalia"
  }
}
---- 6 ----
array(1) {
  [0]=>
  array(1) {
    ["age"]=>
    int(11)
  }
}
---- 7 ----
expected dropView failure 10000 : [HY000] Couldn't fetch data
done!%A
