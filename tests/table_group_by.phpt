--TEST--
mysqlx table groupBy
--SKIPIF--
--INI--
error_reporting=E_ALL
--FILE--
<?php
require("connect.inc");

function run_query($query) {
	$res = $query->execute();
	expect_eq($res->getWarningsCount(), 0);
	expect_eq($res->getWarnings(), []);
	return $res->fetchAll();
}

function run_failed_query($query) {
	try {
		$query->execute();
		test_step_failed();
	} catch(Exception $ex) {
		test_step_ok();
	}
}

$session = create_test_db();

$schema = $session->getSchema($db);
$table = $schema->getTable($test_table_name);

fill_db_table_use_dup();

$query = $table->select([ 'job', 'count(name) as cn', 'max(age) as ma']);
$query = $query->where('job like :job')->bind(['job' => 'driver']);
$query = $query->groupBy('job');
$data = run_query($query);
expect_eq(count($data), 1);

expect_eq($data[0]['job'], 'driver');
expect_eq($data[0]['cn'], 3);
expect_eq($data[0]['ma'], 39);

// -----

$query = $table->select([ 'job', 'count(name) as cn', 'min(age) as ma']);
$query = $query->groupBy('job')->limit(3);
$query = $query->orderBy('cn desc', 'job desc')->having('cn = 3');
$data = run_query($query);
expect_eq(count($data), 2);

expect_eq($data[0]['job'], 'driver');
expect_eq($data[0]['cn'], 3);
expect_eq($data[0]['ma'], 17);

expect_eq($data[1]['job'], 'cook');
expect_eq($data[1]['cn'], 3);
expect_eq($data[1]['ma'], 20);

// -----

$table->insert('name', 'age', 'job')->values(["Tytus", 33, "scout"])->execute();
$table->insert('name', 'age', 'job')->values(["Romek", 33, "scout"])->execute();
$table->insert('name', 'age', 'job')->values(["Atomek", 33, "scout"])->execute();
$table->insert('name', 'age', 'job')->values(["Tytus", 33, "scout"])->execute();
$table->insert('name', 'age', 'job')->values(["Romek", 33, "scout"])->execute();
$table->insert('name', 'age', 'job')->values(["Atomek", 33, "scout"])->execute();
$table->insert('name', 'age', 'job')->values(["Tytus", 33, "scout"])->execute();
$table->insert('name', 'age', 'job')->values(["Romek", 33, "scout"])->execute();
$table->insert('name', 'age', 'job')->values(["Tytus", 33, "scout"])->execute();

$query = $table->select(['name', 'age', 'job', 'sum(age) as sj']);
$query = $query->where('job like :job');
$query = $query->bind(['job' => 'scout']);
$query = $query->groupBy('name', 'age', 'job')->orderBy('name asc');
$data = run_query($query);
expect_eq(count($data), 3);

expect_eq($data[0]['name'], 'Atomek');
expect_eq($data[0]['age'], 33);
expect_eq($data[0]['job'], 'scout');
expect_eq($data[0]['sj'], '66.');

expect_eq($data[1]['name'], 'Romek');
expect_eq($data[1]['age'], 33);
expect_eq($data[1]['job'], 'scout');
expect_eq($data[1]['sj'], '99.');

expect_eq($data[2]['name'], 'Tytus');
expect_eq($data[2]['age'], 33);
expect_eq($data[2]['job'], 'scout');
expect_eq($data[2]['sj'], '132.');

// --------------------

$sql_mode_only_full_group_by = 'ONLY_FULL_GROUP_BY';

$query = $table->select(['name', 'job', 'max(age) as ma']);
$query = $query->where('age like :age');
$query = $query->bind(['age' => 27]);
// groupBy on one column only, despite there are two (name, job)
$query = $query->groupBy('job');
// so query should fail with mode 'ONLY_FULL_GROUP_BY' enabled
run_failed_query($query);


// mode 'ONLY_FULL_GROUP_BY' is disabled, so query should run successfully
disable_sql_mode($session, $sql_mode_only_full_group_by);
$data = run_query($query);
expect_eq(count($data), 1);
expect_eq($data[0]['name'], 'Polly');
expect_eq($data[0]['job'], 'tailor');
expect_eq($data[0]['ma'], 27);


// mode 'ONLY_FULL_GROUP_BY' is enabled again
enable_sql_mode($session, $sql_mode_only_full_group_by);
// but in groupBy are used all columns, so query should run successfully
$query = $query->groupBy('name', 'job');
$data = run_query($query);
expect_eq(count($data), 1);
expect_eq($data[0]['name'], 'Polly');
expect_eq($data[0]['job'], 'tailor');
expect_eq($data[0]['ma'], 27);

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
