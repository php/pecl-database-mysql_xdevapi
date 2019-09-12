<?php

require_once(__DIR__."/../connect.inc");

function print_data($name) {
	global $coll;
	$res = $coll->find('name = "'.$name.'"')->execute();
	print_r($res->fetchAll());
}

?>
