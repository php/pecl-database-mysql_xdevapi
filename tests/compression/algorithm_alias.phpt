--TEST--
mysqlx compression algorithm aliases
--SKIPIF--
--FILE--
<?php
require("compression_utils.inc");

setup_compression_session('compression=preferred&compression-algorithms=[zlib,lz4,zstd]', true, false);
setup_compression_session('compression=disabled&compression-algorithms=[zlib,lz4]', true, false);
setup_compression_session('compression=required&compression-algorithms=[deflate,zstd]', true, true);
setup_compression_session('compression-algorithms=[zstd,zlib,lz4]', true, false);

setup_compression_session('compression=preferred&compression-algorithms=zlib', true, false);
setup_compression_session('compression=disabled&compression-algorithms=[deflate]', true, false);
setup_compression_session('compression=required&compression-algorithms=lz4', true, true);
setup_compression_session('compression-algorithms=[zstd]', true, false);

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
