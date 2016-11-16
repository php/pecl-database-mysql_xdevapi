--TEST--
mysqlx Field Metadata
--SKIPIF--
--FILE--
<?php
        require("connect.inc");

        $nodeSession = create_test_db();

        $schema = $nodeSession->getSchema($db);
	$table = $schema->getTable("test_table");

        fill_db_table();

        $res = $table->select(['name','age'])->execute();
	$cols = $res->getColumns();

        expect_eq(count($cols), 2);
	$fmd1 = $cols[0];
	expect_eq($fmd1->type, 7); //BYTES
	expect_eq($fmd1->type_name, 'BYTES');
	expect_eq($fmd1->name, 'name');
	expect_eq($fmd1->original_name, 'name');
	expect_eq($fmd1->table, 'test_table');
	expect_eq($fmd1->original_table, 'test_table');
	expect_eq($fmd1->schema, $db);
	expect_eq($fmd1->collation, 8);
	expect_eq($fmd1->content_type, 0);
	/*
	        Not checking the other fields since I'm not
		sure whether those are platform dependent.
	*/

        $fmd2 = $cols[1];
	expect_eq($fmd2->type, 1); //SINT
	expect_eq($fmd2->type_name, 'SINT');
	expect_eq($fmd2->name, 'age');
	expect_eq($fmd2->original_name, 'age');
	expect_eq($fmd2->table, 'test_table');
	expect_eq($fmd2->original_table, 'test_table');
	expect_eq($fmd2->schema, $db);
	expect_eq($fmd2->collation, 0);
	expect_eq($fmd2->content_type, 0);

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
