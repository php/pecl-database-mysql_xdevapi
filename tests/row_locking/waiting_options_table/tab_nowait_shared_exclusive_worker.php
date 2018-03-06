<?php
require_once(__DIR__."/../../connect.inc");
require_once(__DIR__."/../row_locking_utils.inc");

assert_mysql_xdevapi_loaded();

notify_worker_started();

$session = mysql_xdevapi\getSession($connection_uri);
$schema = $session->getSchema($test_schema_name);
$table = $schema->getTable($test_table_name);

recv_let_worker_run_cmd();
// terminal/cmd 2
$session->startTransaction();
try {
	check_select_lock_one($table, '1', 1, $Lock_exclusive, MYSQLX_LOCK_NOWAIT);
	// the execution should return immediately, and an error must be thrown
	test_step_failed();
} catch(Exception $e) {
	log_err($e);
	notify_worker_ran_cmd();
}

recv_let_worker_rollback();
// terminal/cmd 2
// since commit is done in cmd1 then the read must be possible now and no error
// should be thrown
check_select_lock_one($table, '1', 1, $Lock_exclusive, MYSQLX_LOCK_NOWAIT);
$session->rollback();
// rollback the open transaction
notify_worker_rolled_back();

recv_let_worker_end();
?>
