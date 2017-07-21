<?php
	require_once(__DIR__.DIRECTORY_SEPARATOR."../connect.inc");

	// ------------------------------------------------------------------------

	function log_err($msg) {
		fputs(STDERR, $msg."\n" );
	}

	// ------------------------------------------------------------------------

	function createTestCollection($session) {
		global $test_schema_name;
		global $test_collection_name;

		$session->createSchema($test_schema_name);
		$schema = $session->getSchema($test_schema_name);

		$coll = $schema->createCollection($test_collection_name);

		$coll->add('{"id": "1", "n": 1}')->execute();
		$coll->add('{"id": "2", "n": 2}')->execute();
		$coll->add('{"id": "3", "n": 3}')->execute();

		return $coll;
	}

	function createTestTable($session) {
		global $test_schema_name;
		global $test_table_name;

		$session->createSchema($test_schema_name);
		$schema = $session->getSchema($test_schema_name);

		$schema->createTable($test_table_name)
			->addColumn(new mysql_xdevapi\ColumnDef('id', 'varchar', 8))
			->addColumn(new mysql_xdevapi\ColumnDef('n', 'int'))
			->execute();

		$tab = $schema->getTable($test_table_name);

		$tab->insert(["id", "n"])->values(["1", 1])->execute();
		$tab->insert(["id", "n"])->values(["2", 2])->execute();
		$tab->insert(["id", "n"])->values(["3", 3])->execute();

		return $tab;
	}

	// ------------------------------------------------------------------------

	$worker_pipes = array();

	function resolve_extension_dir() {
		$extension_dir = ini_get("extension_dir");
		if (!strlen($extension_dir)) {
			// may be needed for Win, for Unix systems it may be
			// necessary to search other dirs, but so far didn't meet such 
			// case yet
			$extension_dir = pathinfo(PHP_BINARY, PATHINFO_DIRNAME);
		}
		return $extension_dir;
	}

	function prepare_extension_filename() {
		$ext_lib_name_prefix = is_os_windows() ? "php_" : "";
		global $Devapi_ext_name;
		$extension_fname = $ext_lib_name_prefix.$Devapi_ext_name.'.'.PHP_SHLIB_SUFFIX;
		return $extension_fname;
	}

	function prepare_worker_cmdline($worker_path) {
		$worker_cmd = PHP_BINARY;

		$ini_path = php_ini_loaded_file();
		if (file_exists($ini_path)) {
			$worker_cmd .= " -c ".$ini_path;
		} else {
			$worker_cmd .= " -n";
		}

		$worker_cmd .= " -d extension_dir=".resolve_extension_dir();
		$worker_cmd .= " -d extension=".prepare_extension_filename();

		$worker_cmd .= " ".$worker_path;
		return $worker_cmd;
	}

	function run_worker($master_path) {
		$master_fname = pathinfo($master_path, PATHINFO_FILENAME);
		$worker_path = __DIR__.DIRECTORY_SEPARATOR.$master_fname."_worker.php";
		$worker_cmd = prepare_worker_cmdline($worker_path);

		$descriptorspec = array(
			0 => array("pipe", "r"), // stdin
			1 => array("pipe", "w"), // stdout
			2 => array("file", $worker_path.".stderr", "w") // stderr
		);

		echo "worker cmd-line: ", $worker_cmd, "\n";

		global $worker_pipes;
		return proc_open($worker_cmd, $descriptorspec, $worker_pipes);
	}

	// ------------------------------------------------------------------------

	$Msg_worker_started = "worker started";
	$Msg_let_worker_modify = "let worker modify";
	$Msg_let_worker_commit = "let worker commit";
	$Msg_worker_committed = "worker committed";
	$Msg_let_worker_end = "let worker end";

	// -------

	function check_msg($raw_msg, $expected_msg) {
		$msg = trim($raw_msg);
		if ($msg != $expected_msg) {
			exit("received: '".$msg."', expected: '".$expected_msg."'");
		}
	}

	function send_msg_to_worker($msg) {
		global $worker_pipes;
		$msg_line = $msg."\n";
		fputs($worker_pipes[0], $msg_line);
		echo $msg_line;
	}

	function recv_msg_from_worker($expected_msg) {
		global $worker_pipes;
		$msg = fgets($worker_pipes[1]);
		check_msg($msg, $expected_msg);
		echo $msg;
	}

	// -------

	function notify_worker_started() {
		global $Msg_worker_started;
		echo $Msg_worker_started, "\n";
	}

	function recv_worker_started() {
		global $Msg_worker_started;
		recv_msg_from_worker($Msg_worker_started);
	}

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

	function send_let_worker_end() {
		global $Msg_let_worker_end;
		send_msg_to_worker($Msg_let_worker_end);
	}

	function recv_let_worker_end() {
		global $Msg_let_worker_end;
		$msg = fgets(STDIN);
		check_msg($msg, $Msg_let_worker_end);
	}

	// ------------------------------------------------------------------------

	$Lock_shared = "shared";
	$Lock_exclusive = "exclusive";

	function check_value($val, $expected_id, $expected_n, $stack_depth = 4) {
		expect_eq($val['id'], $expected_id, 'id', $stack_depth);
		expect_eq($val['n'], $expected_n, 'n', $stack_depth);
	}

	function check_one($res, $expected_id, $expected_n) {
		check_value($res->fetchOne(), $expected_id, $expected_n, $stack_depth = 3);
	}

	function exec_lock_cmd($cmd, $lock) {
		global $Lock_shared;
		global $Lock_exclusive;
		if ($lock == $Lock_shared) {
			$cmd->lockShared();
		} else {
			assert($lock == $Lock_exclusive);
			$cmd->lockExclusive();
		}
		return $cmd->execute();
	}

	function check_lock_one($cmd, $id, $expected_n, $lock) {
		$res = exec_lock_cmd($cmd, $lock);
		$val = $res->fetchOne();
		check_value($val, $id, $expected_n);
	}

	function check_lock_all($cmd, $ids, $expected_vals, $lock) {
		assert(count($ids), count($expected_vals));

		$res = exec_lock_cmd($cmd, $lock);
		$vals = $res->fetchAll();
		$stack_depth = 3;
		expect_eq(count($vals), count($expected_vals), "count($vals)", $stack_depth);

		for ($i = 0; $i < count($ids); $i++) {
			check_value($vals[$i], $ids[$i], $expected_vals[$i]);
		}
	}

	// -------

	function find_lock_one($coll, $id, $lock) {
		$cmd = $coll->find("id like '$id'");
		$res = exec_lock_cmd($cmd, $lock);
		return $res->fetchOne();
	}

	function check_find_lock_one($coll, $id, $expected_n, $lock) {
		$cmd = $coll->find("id like '$id'");
		check_lock_one($cmd, $id, $expected_n, $lock);
	}

	function check_find_lock_all($coll, $ids, $expected_vals, $lock) {
		$joined_ids = implode("', '", $ids);
		$cmd = $coll->find("id in ('".$joined_ids."')");
		check_lock_all($cmd, $ids, $expected_vals, $lock);
	}

	function modify_row($coll, $id, $n) {
		$coll->modify("id = '$id'")->set('n', $n)->execute();
	}

	// -------

	function select_lock_one($tab, $id, $lock) {
		$cmd = $tab->select('id', 'n')->where("id like '$id'");
		$res = exec_lock_cmd($cmd, $lock);
		return $res->fetchOne();
	}

	function check_select_lock_one($tab, $id, $expected_n, $lock) {
		$cmd = $tab->select('id', 'n')->where("id like '$id'");
		check_lock_one($cmd, $id, $expected_n, $lock);
	}

	function check_select_lock_all($tab, $ids, $expected_vals, $lock) {
		$joined_ids = implode("', '", $ids);
		$cmd = $tab->select('id', 'n')->where("id in ('".$joined_ids."')");
		check_lock_all($cmd, $ids, $expected_vals, $lock);
	}

	function update_row($tab, $id, $n) {
		$tab->update()->set('n', $n)->where("id = '$id'")->execute();
	}

?>
