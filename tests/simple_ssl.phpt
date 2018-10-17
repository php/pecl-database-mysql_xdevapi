--TEST--
mysqlx simple SSL connection
--SKIPIF--
--FILE--
<?php
	require(__DIR__."/connect.inc");
	$session = mysql_xdevapi\getSession($connection_uri);

	$ssl_info_query = $session->sql("show variables like '%ssl%'")->execute();
	$ssl_info = $ssl_info_query->fetchAll();

	$mysql_datadir = get_mysql_variable($session, 'datadir');

	if( false == is_dir( $mysql_datadir ) ) {
		$mysql_datadir = __DIR__.'/ssl';
	}


	$rsa_key_path = $mysql_datadir.DIRECTORY_SEPARATOR;

	//Attempt a connection using these ssl settings
	$new_uri = $scheme.'://'.$user.':'.$passwd.'@'.$host.':'.$port.'/?';
	$new_uri .= 'ssl-key=' . $rsa_key_path . 'client-key.pem&';
	$new_uri .= 'ssl-cert=' . $rsa_key_path . 'client-cert.pem&';
	$new_uri .= 'ssl-ca=' . $rsa_key_path . 'ca.pem';

	$session = mysql_xdevapi\getSession($new_uri);
	$res = $session->sql('SELECT USER()')->execute();
	$userdata = $res->fetchOne();

	expect_eq($userdata['USER()'], $user.'@'.$host);
	verify_expectations();
	print "done!\n";
?>
--CLEAN--
<?php
	require(__DIR__."/connect.inc");
	clean_test_db();
?>
--EXPECTF--
done!%A
