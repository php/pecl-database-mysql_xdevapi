--TEST--
mysqlx compression algorithm empty
--SKIPIF--
--FILE--
<?php
require("compression_utils.inc");

setup_compression_session('compression=preferred&compression-algorithms=[]', true, false);
setup_compression_session('compression=disabled&compression-algorithms=[]', true, false);
setup_compression_session('compression=required&compression-algorithms=[]', false, true);
setup_compression_session('compression-algorithms=[]', true, false);

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
