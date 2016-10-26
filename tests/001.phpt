--TEST--
xmysqlnd getNodeSession (success/fail)
--SKIPIF--
--INI--
error_reporting=0
--FILE--
<?php
        require_once("connect.inc");

        $test = "10000";
        $nodeSession = false;

        try{
            $nodeSession = mysql_xdevapi\getNodeSession($host, $user, $passwd);
        }catch(Exception $e) {
            $test[0] = "0";
        }

        if(false == $nodeSession)
            $test[0] = "0";

        try{
            $nodeSession = mysql_xdevapi\getNodeSession("bad_host", $user, $passwd);
        }catch(Exception $e) {
            $test[1] = "1";
        }

        try{
            $nodeSession = mysql_xdevapi\getNodeSession($host, "bad_user", $passwd);
        }catch(Exception $e) {
            $test[2] = "1";
        }

        try{
            $nodeSession = mysql_xdevapi\getNodeSession($host, $user, "some_password");
        }catch(Exception $e) {
            $test[3] = "1";
        }

        $nodeSession = mysql_xdevapi\getNodeSession("", $user, $passwd);
        if(is_bool($nodeSession) && $nodeSession == false)
            $test[4] = "1";

	var_dump($test);
        print "done!\n";
?>
--EXPECTF--
%s(5) "11111"
done!
%a
