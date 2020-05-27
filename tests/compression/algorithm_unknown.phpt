--TEST--
mysqlx compression algorithm unknown
--SKIPIF--
--FILE--
<?php
require("compression_utils.inc");

setup_compression_session('compression=preferred&compression-algorithms=unknown_algorithm', true, false);
setup_compression_session('compression=disabled&compression-algorithms=[unknown_algorithm]', true, false);
setup_compression_session('compression=required&compression-algorithms=unknown_algorithm', false, true);
setup_compression_session('compression-algorithms=[unknown_algorithm]', true, false);

setup_compression_session('compression=preferred&compression-algorithms=zip', true, false);
setup_compression_session('compression=disabled&compression-algorithms=[tgz]', true, false);
setup_compression_session('compression=required&compression-algorithms=[rar,gzip]', false, true);
setup_compression_session('compression-algorithms=[tgz,bz2,lzma]', true, false);

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
