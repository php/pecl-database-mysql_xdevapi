--TEST--
mysqlx session attributes
--SKIPIF--
--INI--
error_reporting=0
--FILE--
<?php
        require("connect.inc");

function find_in_attr_arr( $attr, $key ) {
    foreach ($attr as $item) {
	    if($item["ATTR_NAME"] == $key) {
		    return $item["ATTR_VALUE"];
		}
	}
	return null;
}

function verify_connection_attrib( $session, $expected ) {
		$res = $session->sql('select * from performance_schema.session_account_connect_attrs')->execute();
		$data = $res->fetchAll();
		expect_eq(count($expected), count($data));
		if( $expected == null ) {
		    expect_eq(count($data),0);
		}
		else{
		    foreach ($expected as $key => $value) {
			   if( $value != "?" ) {
			        $ret_val = find_in_attr_arr($data,$key);
					expect_not_null($ret_val);
					if($ret_val != null) {
					    expect_eq($ret_val,$value);
						if( $ret_val != $value ) {
						    return false;
						}
					}
					else {
					    return false;
					}
				}
			}
		}
		return true;
}

		$expected = [
		    "_pid" => (string)getmypid(),
			"_platform" => "?",
			"_os" => "?",
			"_source_host" => gethostname(),
			"_client_name" => "mysql-connector-php",
			"_client_version" => MYSQLX_VERSION,
			"_client_license" => "PHP License, version 3.01",
		];

        $session = mysql_xdevapi\getSession($scheme.'://'.$user.':'.$passwd.'@'.$host.':'.$port);
		verify_connection_attrib($session,$expected);
		$session->close();

        $session = mysql_xdevapi\getSession($scheme.'://'.$user.':'.$passwd.'@'.$host.':'.$port.'/?connection-attributes=false');
		verify_connection_attrib($session, null);
		$session->close();

        $session = mysql_xdevapi\getSession($scheme.'://'.$user.':'.$passwd.'@'.$host.':'.$port.'/?connection-attributes=true');
		verify_connection_attrib($session,$expected);
		$session->close();

        $session = mysql_xdevapi\getSession($scheme.'://'.$user.':'.$passwd.'@'.$host.':'.$port.'/?connection-attributes=[test=value]');
		$expected += [ "test" => "value" ];
		verify_connection_attrib($session,$expected);
		$session->close();

        $session = mysql_xdevapi\getSession($scheme.'://'.$user.':'.$passwd.'@'.$host.':'.$port.'/?connection-attributes=[test=value,crap=yes,toggle=no]');
		$expected += [ "crap" => "yes" ];
		$expected += [ "toggle" => "no" ];
		verify_connection_attrib($session,$expected);
		$session->close();

        //Some failing scenarios
		try{
		    $session = mysql_xdevapi\getSession($scheme.'://'.$user.':'.$passwd.'@'.$host.':'.$port.'/?connection-attributes=[_wrong=value]');
			test_step_fail();
		}catch(Exception $e){
		    test_step_ok();
		}

		try{
		    $session = mysql_xdevapi\getSession($scheme.'://'.$user.':'.$passwd.'@'.$host.':'.$port.'/?connection-attributes=wrong');
			test_step_failed("connection attributes can only be [], true or false.");
		}catch(Exception $e){
		    test_step_ok();
		}

        try{
		    $session = mysql_xdevapi\getSession($scheme.'://'.$user.':'.$passwd.'@'.$host.':'.$port.'/?connection-attributes=1.2');
			test_step_failed("connection attributes can only be [], true or false.");
		}catch(Exception $e){
		    test_step_ok();
		}

        try{
		    $session = mysql_xdevapi\getSession($scheme.'://'.$user.':'.$passwd.'@'.$host.':'.$port.'0/?connection-attributes=[foo=bar,test=value,crap=yes,test=value2]');
			test_step_failed("connection-attributes cannot have duplicated keys.");
		}catch(Exception $e){
		    test_step_ok();
		}

        verify_expectations();
		print "done!\n";
?>
--CLEAN--
--EXPECTF--
done!%A
