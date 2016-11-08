--TEST--
mysqlx complex query
--SKIPIF--
--FILE--
<?php
	require("connect.inc");

	$nodeSession = create_test_db();

	fill_db_table();

	$schema = $nodeSession->getSchema($db);
	$table = $schema->getTable("test_table");

	$sel = $table->select(['age as age_group', 'count(name) as cnt'])->groupBy('age_group');
	$sel = $sel->where("age > 11 and 1 < 2 and 40 between 30 and 900");
	$sel = $sel->having('cnt > 1');
	$sel = $sel->orderBy('age_group desc');
	$res = $sel->limit(2)->offset(1)->execute();
	//$sel2 = $sel->groupBy('age_group');
	$data = $res->fetchAll();
	var_dump($data);
	print "done!\n";
?>
--CLEAN--
<?php
	require("connect.inc");
	clean_test_db();
?>
--EXPECTF--
array(2) {
  [0]=>
  array(2) {
    ["age_group"]=>
    int(15)
    ["cnt"]=>
    int(2)
  }
  [1]=>
  array(2) {
    ["age_group"]=>
    int(14)
    ["cnt"]=>
    int(2)
  }
}
done!%A
