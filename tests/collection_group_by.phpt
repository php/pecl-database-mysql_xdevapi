--TEST--
mysqlx collection groupBy
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
$coll = $schema->getCollection($test_collection_name);

fill_db_collection($coll, true);

$query = $coll->find('job like :job')->groupBy('job');
$query = $query->fields([ 'job', 'count(name) as cn', 'max(age) as ma']);
$query = $query->bind(['job' => 'Programmatore']);
$data = run_query($query);
expect_eq(count($data), 1);

expect_eq($data[0]['job'], 'Programmatore');
expect_eq($data[0]['cn'], 5);
expect_eq($data[0]['ma'], '27');

// -----

$query = $coll->find(true)->fields([ 'job', 'count(name) as cn', 'min(age) as ma']);
$query = $query->groupBy('job')->limit(3);
$query = $query->sort('count(name) desc', 'job desc')->having('count(name) = 2');
$data = run_query($query);
expect_eq(count($data), 3);

expect_eq($data[0]['job'], 'Programmatrice');
expect_eq($data[0]['cn'], 2);
expect_eq($data[0]['ma'], '23');

expect_eq($data[1]['job'], 'Portiere');
expect_eq($data[1]['cn'], 2);
expect_eq($data[1]['ma'], '39');

expect_eq($data[2]['job'], 'Cantante');
expect_eq($data[2]['cn'], 2);
expect_eq($data[2]['ma'], '27');

// -----

$coll->add('{"name": "John Smith", "age": 33, "job": "WorkerA"}')->execute();
$coll->add('{"name": "John Smith", "age": 33, "job": "WorkerB"}')->execute();
$coll->add('{"name": "John Smith", "age": 33, "job": "WorkerC"}')->execute();
$coll->add('{"name": "John Smith", "age": 33, "job": "WorkerA"}')->execute();
$coll->add('{"name": "John Smith", "age": 33, "job": "WorkerB"}')->execute();
$coll->add('{"name": "John Smith", "age": 33, "job": "WorkerC"}')->execute();
$coll->add('{"name": "John Smith", "age": 33, "job": "WorkerA"}')->execute();
$coll->add('{"name": "John Smith", "age": 33, "job": "WorkerB"}')->execute();
$coll->add('{"name": "John Smith", "age": 33, "job": "WorkerA"}')->execute();

$query = $coll->find('name like :name');
$query = $query->fields(['name', 'age', 'job', 'sum(age) as sj']);
$query = $query->bind(['name' => 'John Smith']);
$query = $query->groupBy('name', 'age', 'job')->sort('job asc');
$data = run_query($query);
expect_eq(count($data), 3);

expect_eq($data[0]['name'], 'John Smith');
expect_eq($data[0]['age'], 33);
expect_eq($data[0]['job'], 'WorkerA');
expect_eq($data[0]['sj'], 132.0);

expect_eq($data[1]['name'], 'John Smith');
expect_eq($data[1]['age'], 33);
expect_eq($data[1]['job'], 'WorkerB');
expect_eq($data[1]['sj'], 99.0);

expect_eq($data[2]['name'], 'John Smith');
expect_eq($data[2]['age'], 33);
expect_eq($data[2]['job'], 'WorkerC');
expect_eq($data[2]['sj'], 66.0);

// --------------------

$sql_mode_only_full_group_by = 'ONLY_FULL_GROUP_BY';

$query = $coll->find('job like :job');
$query = $query->fields(['name', 'job', 'max(age) as ma']);
$query = $query->bind(['job' => 'Ballerino']);
// groupBy on one field only, despite there are two (name, job)
$query = $query->groupBy('job');
// so query should fail with mode 'ONLY_FULL_GROUP_BY' enabled
run_failed_query($query);


// mode 'ONLY_FULL_GROUP_BY' is disabled, so query should run successfully
disable_sql_mode($session, $sql_mode_only_full_group_by);
$data = run_query($query);
expect_eq(count($data), 1);
expect_eq($data[0]['name'], 'Monica');
expect_eq($data[0]['job'], 'Ballerino');
expect_eq($data[0]['ma'], '35');


// mode 'ONLY_FULL_GROUP_BY' is enabled again
enable_sql_mode($session, $sql_mode_only_full_group_by);
// but in groupBy are used all fields, so query should run successfully
$query = $query->groupBy('name', 'job');
$data = run_query($query);
expect_eq(count($data), 1);
expect_eq($data[0]['name'], 'Monica');
expect_eq($data[0]['job'], 'Ballerino');
expect_eq($data[0]['ma'], '35');

print "verifying";
verify_expectations();
print "done!\n";
?>
--CLEAN--
<?php
require("connect.inc");
clean_test_db();
?>
--EXPECTF--
%Averifyingdone!%A
