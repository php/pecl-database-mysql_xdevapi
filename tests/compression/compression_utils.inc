<?php

require_once(__DIR__.DIRECTORY_SEPARATOR."../connect.inc");

const Compression_disabled_msg =
	"[10079][HY000] To enable given compression algorithm please build mysql_xdevapi with proper ".
	"switch like --with-(zlib|lz4|zstd)=[DIR].";

function setup_compression_session($compression_options, $expect_success, $expect_failure) {
	try {
		global $base_uri;
		$session = mysql_xdevapi\getSession($base_uri.'/?'.$compression_options);
		if ($expect_success) {
			test_step_ok();
		} else {
			test_step_failed();
		}
	} catch(Exception $e) {
		$msg = $e->getMessage();
		if ($expect_failure) {
			test_step_ok();
			if ($msg != Compression_disabled_msg) {
				echo $msg, "\n";
			}
		} else {
			test_step_failed();
			echo $msg, "\n";
		}
	}
}

?>
