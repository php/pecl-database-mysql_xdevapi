<?php

require_once(__DIR__.DIRECTORY_SEPARATOR."../connect.inc");

// -------

function prepare_uri_with_schema($schema) {
	global $base_uri;
	global $disable_ssl_opt;
	return $base_uri.'/'.$schema.'?'.$disable_ssl_opt;
}

function create_schema($session, $schema_name) {
	$session->sql("create database $schema_name")->execute();
	return $session->getSchema($schema_name);
}

function create_table($session, $schema, $table_name) {
	$schema_name = $schema->getName();
	$session->sql("create table $schema_name.$table_name(name varchar(128), age int)")->execute();
	return $schema->getTable($table_name);
}

function create_collection($session, $schema, $collection_name) {
	$schema->createCollection($collection_name);
	return $schema->getCollection($collection_name);
}

function verify_default_schema($session, $expected_tables_count) {
	$sql_result = $session->sql('show tables')->execute();
	$tables_result = $sql_result->fetchAll();
	expect_eq(count($tables_result), $expected_tables_count);
}

?>
