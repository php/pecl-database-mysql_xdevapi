<?php

require_once(__DIR__."/../auth_utils.inc");

const Default_connection_time_out = 10;
const Timeout_test_user = "testuser";

const Envar_non_routable_host = "MYSQLX_TEST_NON_ROUTABLE_HOST";
const Envar_non_routable_host_port = "MYSQLX_TEST_NON_ROUTABLE_HOST_PORT";

$log_filepath = "";

//non-routable IP addresses basing on https://en.wikipedia.org/wiki/IPv4#Special_addresses
$Non_routable_hosts = [
	"198.51.100.255",
	"192.0.2.255",
	"10.255.255.1",
	"192.0.2.0",
	"203.0.113.255",
	"10.255.255.255",
	"192.168.255.255",
	"203.0.113.4",
	"192.168.0.0",
	"172.16.0.0",
	"10.255.255.251",
	"172.31.255.255",
	"198.51.100.23",
	"172.16.255.255",
	"198.51.100.8",
	"192.0.2.254",
];

$Non_routable_ports = [
	null,
	80,
	81,
	3306,
	83,
	33060,
	84,
];

function prepare_uri_with_timeout($host, $port, $timeout) {
	global $scheme;
	global $passwd;
	$uri = $scheme . '://' . Timeout_test_user . ':' . $passwd . '@' . $host;
	if ($port !== null) $uri .=  ':' . $port;
	if ($timeout !== null) $uri .= '/?connect-timeout=' . $timeout;
	return $uri;
}

function test_incorrect_timeout($host, $port, $timeout) {
	$uri = prepare_uri_with_timeout($host, $port, $timeout);
	test_connection($uri, null, false, true);
}

function test_successful_not_elapsed_timeout($host, $timeout) {
	global $port;
	$uri = prepare_uri_with_timeout($host, $port, $timeout);
	test_connection($uri, null, true, true);
}

// ----------------

function init_log($master_file_path) {
	global $log_filepath;
	$log_filepath = $master_file_path . ".stdout";
	if (file_exists($log_filepath)) unlink($log_filepath);
}

function log_connection_info($info) {
	global $log_filepath;
	file_put_contents($log_filepath, $info . PHP_EOL, FILE_APPEND);
}

function apply_env() {
	global $Non_routable_hosts;
	global $Non_routable_ports;

	$env_non_routable_host = getenv(Envar_non_routable_host);
	if ($env_non_routable_host) {
		array_unshift($Non_routable_hosts, $env_non_routable_host);
	}

	$env_non_routable_host_port = getenv(Envar_non_routable_host_port);
	if ($env_non_routable_host_port) {
		array_unshift($Non_routable_ports, $env_non_routable_host_port);
	}
}

function try_connect($uri) {
	$start =  microtime(true);
	try {
		$session = mysql_xdevapi\getSession($uri);
		log_connection_info("connection successful");
	} catch(Exception $e) {
		log_connection_info($e->getMessage());
	}
	$connecting_time = microtime(true) - $start;
	return $connecting_time;
}

function resolve_timeout($timeout) {
	if ($timeout === null) return Default_connection_time_out;
	if ($timeout === 0) return intval(ini_get('default_socket_timeout'));
	return $timeout;
}

function verify_connecting_time($connecting_time, $timeout) {
	$timeout = resolve_timeout($timeout);
	log_connection_info("expected timeout: $timeout, connecting time: $connecting_time");
	return is_in_range($connecting_time, $timeout - 1.0, $timeout + 1.0);
}

function test_timeout_connection($host, $port, $timeout) {
	$uri = prepare_uri_with_timeout($host, $port, $timeout);
	log_connection_info($uri);

	$connecting_time = try_connect($uri);
	return verify_connecting_time($connecting_time, $timeout);
}

function test_elapsed_timeout($timeout, $master_file_path) {
	global $Non_routable_hosts;
	global $Non_routable_ports;
	global $log_filepath;

	init_log($master_file_path);
	apply_env();

	foreach ($Non_routable_ports as $port) {
		foreach ($Non_routable_hosts as $host) {
			if (test_timeout_connection($host, $port, $timeout)) {
				test_step_ok();
				return;
			}
			log_connection_info("----------------------");
		}
	}
	echo "All non-routable addresses failed, please check ",
		$log_filepath, " for more details. ",
		PHP_EOL,
		"You can also define environment variables ", Envar_non_routable_host,
		", and (optionally) ", Envar_non_routable_host_port,
		". They will be tested first.",
		PHP_EOL;
	test_step_failed();
}

?>
