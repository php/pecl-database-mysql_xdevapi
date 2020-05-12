--TEST--
mysqlx compression algorithm mix names and aliases
--SKIPIF--
--FILE--
<?php
require("compression_utils.inc");

setup_compression_session('compression=preferred&compression-algorithms=[lz4_message,zstd]', true, false);
setup_compression_session('compression=disabled&compression-algorithms=[deflate,lz4_message,zstd_stream]', true, false);
setup_compression_session('compression=required&compression-algorithms=[lz4,zstd,deflate_stream]', true, true);
setup_compression_session('compression-algorithms=[lz4_message,zlib,zstd_stream]', true, false);

// the same algorithm is allowed more than once
setup_compression_session('compression=preferred&compression-algorithms=[lz4,zstd_stream,lz4_message,zstd]', true, false);
setup_compression_session('compression=disabled&compression-algorithms=[deflate,lz4_message,zlib,zstd_stream]', true, false);
setup_compression_session('compression=required&compression-algorithms=[lz4,zstd,deflate_stream,zstd_stream]', true, true);
setup_compression_session('compression-algorithms=[zlib,zstd_stream,lz4_message,deflate_stream]', true, false);

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
