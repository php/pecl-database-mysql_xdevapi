--TEST--
mysqlx compression algorithm names
--SKIPIF--
--FILE--
<?php
require("compression_utils.inc");

setup_compression_session('compression=preferred&compression-algorithms=[lz4_message,zstd_stream]', true, false);
setup_compression_session('compression=disabled&compression-algorithms=[deflate_stream,lz4_message,zstd_stream]', true, false);
setup_compression_session('compression=required&compression-algorithms=[zstd_stream,deflate_stream]', true, true);
setup_compression_session('compression-algorithms=[lz4_message,zstd_stream]', true, false);

setup_compression_session('compression=preferred&compression-algorithms=lz4_message', true, false);
setup_compression_session('compression=disabled&compression-algorithms=[deflate_stream]', true, false);
setup_compression_session('compression=required&compression-algorithms=zstd_stream', true, true);
setup_compression_session('compression-algorithms=[deflate_stream]', true, false);

verify_expectations();
print "done!\n";
?>
--CLEAN--
<?php
require("connect.inc");
clean_test_db();
?>
--EXPECTF--
done!%A
