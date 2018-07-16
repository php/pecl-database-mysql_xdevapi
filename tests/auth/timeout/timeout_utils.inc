<?php

require_once(__DIR__."/../../connect.inc");
require_once(__DIR__."/../auth_utils.inc");

$Default_connection_time_out = 10;
$Non_routable_host = "10.255.255.1";
$Non_routable_host1 = "192.168.0.0";
$Non_routable_host2 = "192.168.255.255";
$Non_routable_host3 = "172.16.0.0";
$Non_routable_host4 = "172.31.255.255";

function prepare_uri_with_timeout($host, $port, $timeout) {
	global $scheme;
	global $user;
	global $passwd;
	$uri = $scheme . '://' . $user . ':' . $passwd . '@' . $host;
	if ($port) $uri .=  ':' . $port;
	if ($timeout) $uri .= '/?connect-timeout=' . $timeout;
	return $uri;
}

function test_incorrect_timeout($host, $port, $timeout) {
	$uri = prepare_uri_with_timeout($host, $port, $timeout);
	test_connection($uri, null, false, true);
}

function verify_connecting_time($connecting_time, $timeout) {
	if (!$timeout) return true;

	if ($timeout == 0) $timeout = $Default_connection_time_out;

	expect_true(($timeout - 1.0 < $connecting_time) && ($connecting_time < $timeout + 1.0));
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
