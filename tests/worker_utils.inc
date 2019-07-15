<?php

require_once("connect.inc");

// ------------------------------------------------------------------------

function log_err($msg) {
	fputs(STDERR, $msg."\n" );
}

// ------------------------------------------------------------------------

$worker_pipes = array();

function resolve_extension_dir() {
	// assumption for end-user machine is as follows:
	// proper extension_dir is available in php.ini or passed via
	// cmd-line, explicitly or automagically inside run-tests.php
	$extension_dir = ini_get("extension_dir");

	// may make life easier while testing at development stage for php
	// built from sources, but rather not on end-user machines
	if (!strlen($extension_dir)) {
		// while building for win modules (like php_mysql_xdevapi.dll) are
		// located in the same dir as php binary
		$extension_dir = pathinfo(PHP_BINARY, PATHINFO_DIRNAME);
		if (!is_running_on_windows()) {
			// for Linux
			// php binary located in
			//		connector-php/sapi/cli or connector-php/sapi/cgi
			// while mysql_xdevapi.so in
			//		connector-php/modules
			$extension_dir .= "/../../modules";
		}
	}

	return $extension_dir;
}

function prepare_extension_filename($ext_name) {
	$ext_lib_name_prefix = is_running_on_windows() ? "php_" : "";
	$extension_fname = $ext_lib_name_prefix.$ext_name.'.'.PHP_SHLIB_SUFFIX;
	return $extension_fname;
}

function prepare_mysql_xdevapi_ext_filename() {
	return prepare_extension_filename(DEVAPI_EXT_NAME);
}

function prepare_openssl_ext_filename() {
	return prepare_extension_filename(OPENSSL_EXT_NAME);
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
	if (!is_mysql_xdevapi_builtin()) {
		$worker_cmd .= " -d extension=".prepare_mysql_xdevapi_ext_filename();
	}
	if (!is_openssl_builtin()) {
		$worker_cmd .= " -d extension=".prepare_openssl_ext_filename();
	}

	$worker_cmd .= " ".$worker_path;
	return $worker_cmd;
}

function run_worker($master_path) {
	$master_dir = pathinfo($master_path, PATHINFO_DIRNAME);
	$master_fname = pathinfo($master_path, PATHINFO_FILENAME);
	$worker_path = $master_dir.DIRECTORY_SEPARATOR.$master_fname."_worker.php";
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
$Msg_let_worker_block = "let worker block";
$Msg_worker_unblocked = "worker unblocked";
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

function send_let_worker_block() {
	global $Msg_let_worker_block;
	send_msg_to_worker($Msg_let_worker_block);
}

function recv_let_worker_block() {
	global $Msg_let_worker_block;
	$msg = fgets(STDIN);
	check_msg($msg, $Msg_let_worker_block);
}

// -------

function notify_worker_unblocked() {
	global $Msg_worker_unblocked;
	echo $Msg_worker_unblocked, "\n";
}

function recv_worker_unblocked() {
	global $Msg_worker_unblocked;
	recv_msg_from_worker($Msg_worker_unblocked);
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

function send_verification_status() {
	if (verify_expectations()) {
		$reply = "ok";
	} else {
		$reply = 'fail: '.PHP_EOL.$expected.PHP_EOL.$test_result_string.PHP_EOL;
	}
	echo $reply.PHP_EOL;
}

function recv_verification_status() {
	recv_msg_from_worker("ok");
}

?>
