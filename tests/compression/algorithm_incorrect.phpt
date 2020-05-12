--TEST--
mysqlx compression algorithm incorrect
--SKIPIF--
--FILE--
<?php
require("compression_utils.inc");

setup_compression_session('compression=preferred&compression-algorithms=', false, true);
setup_compression_session('compression=disabled&compression-algorithms=[', false, true);
setup_compression_session('compression=required&compression-algorithms=]', false, true);
setup_compression_session('compression-algorithms', false, true);
setup_compression_session('compression=preferred&compression-algorithms={}', false, true);
setup_compression_session('compression=disabled&compression-algorithms=*', false, true);
setup_compression_session('compression=required&compression-algorithms==', false, true);
setup_compression_session('compression-algorithms=[x!y]', false, true);
setup_compression_session('compression-algorithms=[<.>]', false, true);

verify_expectations();
print "done!\n";
?>
--CLEAN--
<?php
require("connect.inc");
clean_test_db();
?>
--EXPECTF--
[10052][HY000] Invalid argument. The argument to compression-algorithms cannot be empty.
[10081][HY000] Invalid algorithm name: [ not recognized as a correct name of compression algorithm
[10081][HY000] Invalid algorithm name: ] not recognized as a correct name of compression algorithm
[10052][HY000] Invalid argument. The argument to compression-algorithms cannot be empty.
[10081][HY000] Invalid algorithm name: {} not recognized as a correct name of compression algorithm
[10081][HY000] Invalid algorithm name: * not recognized as a correct name of compression algorithm
[10081][HY000] Invalid algorithm name: = not recognized as a correct name of compression algorithm
[10081][HY000] Invalid algorithm name: x!y not recognized as a correct name of compression algorithm
[10081][HY000] Invalid algorithm name: <.> not recognized as a correct name of compression algorithm
done!%A
