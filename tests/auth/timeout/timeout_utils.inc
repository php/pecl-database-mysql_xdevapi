<?php

require_once(__DIR__."/../../connect.inc");
require_once(__DIR__."/../auth_utils.inc");

$Default_connection_time_out = 10;
//non-routable IP addresses basing on https://en.wikipedia.org/wiki/IPv4#Special_addresses
$Non_routable_host = "198.51.100.255";
$Non_routable_host1 = "192.0.2.255";
$Non_routable_host2 = "10.255.255.251";
$Non_routable_host3 = "203.0.113.255";
$Non_routable_host4 = "192.168.255.255";

function prepare_uri_with_timeout($host, $port, $timeout) {
	global $scheme;
	global $user;
	global $passwd;
	$uri = $scheme . '://' . $user . ':' . $passwd . '@' . $host;
	if ($port !== null) $uri .=  ':' . $port;
	if ($timeout !== null) $uri .= '/?connect-timeout=' . $timeout;
	return $uri;
}

function test_incorrect_timeout($host, $port, $timeout) {
	$uri = prepare_uri_with_timeout($host, $port, $timeout);
	test_connection($uri, null, false, true);
}

function verify_connecting_time($connecting_time, $timeout) {
	global $Default_connection_time_out;
	if ($timeout === null) {
		$timeout = $Default_connection_time_out;
	} else if ($timeout === 0) {
		$timeout = intval(ini_get('default_socket_timeout'));
	}

	expect_true(
		is_in_range($connecting_time, $timeout - 1.0, $timeout + 1.0),
		", timeout: $timeout, connecting time: $connecting_time");
}

function test_elapsed_timeout($host, $port, $timeout) {
	$uri = prepare_uri_with_timeout($host, $port, $timeout);
	$start =  microtime(true);
	test_connection($uri, null, false, true);
	$connecting_time = microtime(true) - $start;
	echo "connecting time ", $connecting_time, "\n";
	verify_connecting_time($connecting_time, $timeout);
	echo "______________________", "\n";
}

function test_successful_no_timeout($host, $timeout) {
	global $port;
	$uri = prepare_uri_with_timeout($host, $port, $timeout);
	test_connection($uri, null, true, true);
}

?>
