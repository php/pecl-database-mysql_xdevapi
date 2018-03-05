<?php
require_once(__DIR__."/../../connect.inc");
require_once(__DIR__."/../row_locking_utils.inc");

assert_mysql_xdevapi_loaded();

notify_worker_started();

$session = mysql_xdevapi\getSession($connection_uri);
$schema = $session->getSchema($test_schema_name);
$coll = $schema->getCollection($test_collection_name);

recv_let_worker_run_cmd();
// terminal/cmd 2
$session->startTransaction();
check_find_lock_one($coll, '1', null, $Lock_shared, MYSQLX_LOCK_SKIP_LOCKED);
// the execution should return immediately, and no error must be thrown. No data
// should be returned
notify_worker_ran_cmd();

recv_let_worker_rollback();
// terminal/cmd 2
// since commit is done in cmd1 then the read must be possible now and no error
// should be thrown. Data should be returned
check_find_lock_one($coll, '1', 1, $Lock_shared, MYSQLX_LOCK_SKIP_LOCKED);
$session->rollback();
// rollback the open transaction
notify_worker_rolled_back();

recv_let_worker_end();
?>
