<?php

require_once(__DIR__."/../connect.inc");

function verify_op_ps( $op_idx, $expected, $sql_idx, $count ) {
    global $perf_schema_table;
	global $sql_strings;
	$obj = $perf_schema_table->select(true)->execute();
	$res = $obj->fetchAll();
	expect_eq( count($res), $count );
	expect_eq($res[$op_idx]["STATEMENT_ID"], $expected);

    if( $sql_idx > 0 ){
	    expect_eq($res[$op_idx]["SQL_TEXT"], $sql_strings[$sql_idx]);
	}
}

?>
