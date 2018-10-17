<?php
	require_once(__DIR__.DIRECTORY_SEPARATOR."../connect.inc");
	require_once(__DIR__.DIRECTORY_SEPARATOR."../worker_utils.inc");

	// ------------------------------------------------------------------------

	function create_test_collection($session) {
		global $test_schema_name;
		global $test_collection_name;

		$session->createSchema($test_schema_name);
		$schema = $session->getSchema($test_schema_name);

		$coll = $schema->createCollection($test_collection_name);

		$coll->add('{"_id": "1", "n": 1}')->execute();
		$coll->add('{"_id": "2", "n": 2}')->execute();
		$coll->add('{"_id": "3", "n": 3}')->execute();
		$coll->add('{"_id": "4", "n": 4}')->execute();
		$coll->add('{"_id": "5", "n": 5}')->execute();
		$coll->add('{"_id": "6", "n": 6}')->execute();

		return $coll;
	}

	function create_test_table($session) {
		global $db;
		global $test_schema_name;
		global $test_table_name;

		$session->createSchema($test_schema_name);
		$schema = $session->getSchema($test_schema_name);

		$session->sql(
			"create table $db.$test_table_name(".
				"_id varchar(8), n int, PRIMARY KEY(_id))")->execute();

		$tab = $schema->getTable($test_table_name);

		$tab->insert(["_id", "n"])->values(["1", 1])->execute();
		$tab->insert(["_id", "n"])->values(["2", 2])->execute();
		$tab->insert(["_id", "n"])->values(["3", 3])->execute();
		$tab->insert(["_id", "n"])->values(["4", 4])->execute();
		$tab->insert(["_id", "n"])->values(["5", 5])->execute();
		$tab->insert(["_id", "n"])->values(["6", 6])->execute();

		return $tab;
	}

	// ------------------------------------------------------------------------

	$Msg_let_worker_modify = "let worker modify";
	$Msg_let_worker_run_cmd = "let worker run cmd";
	$Msg_worker_ran_cmd = "worker ran cmd";
	$Msg_let_worker_commit = "let worker commit";
	$Msg_worker_committed = "worker committed";
	$Msg_let_worker_rollback = "let worker rollback";
	$Msg_worker_rolled_back = "worker rolled back";

	// -------

	function send_let_worker_modify() {
		global $Msg_let_worker_modify;
		send_msg_to_worker($Msg_let_worker_modify);
	}

	function recv_let_worker_modify() {
		global $Msg_let_worker_modify;
		$msg = fgets(STDIN);
		check_msg($msg, $Msg_let_worker_modify);
	}

	// -------

	function send_let_worker_run_cmd() {
		global $Msg_let_worker_run_cmd;
		send_msg_to_worker($Msg_let_worker_run_cmd);
	}

	function recv_let_worker_run_cmd() {
		global $Msg_let_worker_run_cmd;
		$msg = fgets(STDIN);
		check_msg($msg, $Msg_let_worker_run_cmd);
	}

	// -------

	function notify_worker_ran_cmd() {
		global $Msg_worker_ran_cmd;
		echo $Msg_worker_ran_cmd, "\n";
	}

	function recv_worker_ran_cmd() {
		global $Msg_worker_ran_cmd;
		recv_msg_from_worker($Msg_worker_ran_cmd);
	}

	// -------

	function send_let_worker_commit() {
		global $Msg_let_worker_commit;
		send_msg_to_worker($Msg_let_worker_commit);
	}

	function recv_let_worker_commit() {
		global $Msg_let_worker_commit;
		$msg = fgets(STDIN);
		check_msg($msg, $Msg_let_worker_commit);
	}

	// -------

	function notify_worker_committed() {
		global $Msg_worker_committed;
		echo $Msg_worker_committed, "\n";
	}

	function recv_worker_committed() {
		global $Msg_worker_committed;
		recv_msg_from_worker($Msg_worker_committed);
	}

	// -------

	function send_let_worker_rollback() {
		global $Msg_let_worker_rollback;
		send_msg_to_worker($Msg_let_worker_rollback);
	}

	function recv_let_worker_rollback() {
		global $Msg_let_worker_rollback;
		$msg = fgets(STDIN);
		check_msg($msg, $Msg_let_worker_rollback);
	}

	// -------

	function notify_worker_rolled_back() {
		global $Msg_worker_rolled_back;
		echo $Msg_worker_rolled_back, "\n";
	}

	function recv_worker_rolled_back() {
		global $Msg_worker_rolled_back;
		recv_msg_from_worker($Msg_worker_rolled_back);
	}

	// ------------------------------------------------------------------------

	$Lock_none = "none";
	$Lock_shared = "shared";
	$Lock_exclusive = "exclusive";

	function check_value($val, $expected_id, $expected_n) {
		if ($expected_n != null) {
			expect_eq($val['_id'], $expected_id, '_id');
			expect_eq($val['n'], $expected_n, 'n');
		} else {
			expect_false($val && is_array($val) && array_key_exists($expected_id, $val));
		}
	}

	function check_one($res, $expected_id, $expected_n) {
		check_value($res->fetchOne(), $expected_id, $expected_n);
	}

	function exec_lock_cmd($cmd, $lock, $waiting_option = null) {
		global $Lock_none;
		global $Lock_shared;
		global $Lock_exclusive;
		if ($lock == $Lock_shared) {
			$waiting_option ? $cmd->lockShared($waiting_option) : $cmd->lockShared();
		} else if ($lock == $Lock_exclusive) {
			$waiting_option ? $cmd->lockExclusive($waiting_option) : $cmd->lockExclusive();
		} else {
			assert(($lock == $Lock_none) && ($waiting_option == null));
		}
		return $cmd->execute();
	}

	function check_lock_one($cmd, $id, $expected_n, $lock, $waiting_option = null) {
		$res = exec_lock_cmd($cmd, $lock, $waiting_option);
		$val = $res->fetchOne();
		check_value($val, $id, $expected_n);
	}

	function check_lock_all($cmd, $ids, $expected_vals, $lock) {
		assert(count($ids) == count($expected_vals));

		$res = exec_lock_cmd($cmd, $lock);
		$vals = $res->fetchAll();
		expect_eq(count($vals), count($expected_vals), "count($vals)");

		for ($i = 0; $i < count($ids); $i++) {
			check_value($vals[$i], $ids[$i], $expected_vals[$i]);
		}
	}

	// -------

	function find_lock_one($coll, $id, $lock) {
		$cmd = $coll->find("_id = '$id'");
		$res = exec_lock_cmd($cmd, $lock);
		return $res->fetchOne();
	}

	function check_find_lock_one($coll, $id, $expected_n, $lock, $waiting_option = null) {
		$cmd = $coll->find("_id = '$id'");
		check_lock_one($cmd, $id, $expected_n, $lock, $waiting_option);
	}

	function check_find_lock_all($coll, $ids, $expected_vals, $lock) {
		$joined_ids = implode("', '", $ids);
		$cmd = $coll->find("_id in ('".$joined_ids."')");
		check_lock_all($cmd, $ids, $expected_vals, $lock);
	}

	function modify_row($coll, $id, $n) {
		$coll->modify("_id = '$id'")->set('n', $n)->execute();
	}

	// -------

	function select_lock_one($tab, $id, $lock) {
		$cmd = $tab->select('_id', 'n')->where("_id = '$id'");
		$res = exec_lock_cmd($cmd, $lock);
		return $res->fetchOne();
	}

	function check_select_lock_one($tab, $id, $expected_n, $lock, $waiting_option = null) {
		$cmd = $tab->select('_id', 'n')->where("_id = '$id'");
		check_lock_one($cmd, $id, $expected_n, $lock, $waiting_option);
	}

	function check_select_lock_all($tab, $ids, $expected_vals, $lock) {
		$joined_ids = implode("', '", $ids);
		$cmd = $tab->select('_id', 'n')->where("_id in ('".$joined_ids."')");
		check_lock_all($cmd, $ids, $expected_vals, $lock);
	}

	function update_row($tab, $id, $n) {
		$tab->update()->set('n', $n)->where("_id = '$id'")->execute();
	}

?>
