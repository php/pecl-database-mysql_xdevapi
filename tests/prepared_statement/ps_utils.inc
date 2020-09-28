<?php

require_once(__DIR__."/../connect.inc");

function get_prepared_statements() {
    global $perf_schema_table;
	$obj = $perf_schema_table->select(true)->execute();
	return $obj->fetchAll();
}

function get_stmt_id($op_idx) {
	$prep_stmts = get_prepared_statements();
	return $prep_stmts[$op_idx]["STATEMENT_ID"];
}

function verify_op_ps( $op_idx, $expected, $sql_idx, $count ) {
	global $sql_strings;
	$prep_stmts = get_prepared_statements();
	expect_eq( count($prep_stmts), $count );
	$prep_stmt = $prep_stmts[$op_idx];
	expect_eq($prep_stmt["STATEMENT_ID"], $expected);

    if( $sql_idx > 0 ){
	    expect_eq($prep_stmt["SQL_TEXT"], $sql_strings[$sql_idx]);
	}
}

?>
