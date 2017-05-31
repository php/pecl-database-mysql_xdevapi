--TEST--
mysqlx Session Configuration Manager Part A
--SKIPIF--
--FILE--
<?php
        require("connect.inc");
	$session = mysql_xdevapi\sessions();

        //Make sure to clean any existing session
	$sessions = $session->list();
	for( $i = 0 ; $i < count( $sessions ) ; $i++ ) {
	    expect_true( $session->delete( $sessions[ $i ] ) );
	}

function expect_list($amount) {
        global $session;
	$sessions = $session->list();
	expect_eq( count($sessions), $amount );
	return $sessions;
}

        $session_uri_name1 = 'sessionUri1';
	$session_uri_name2 = 'sessionUri2';
	$session_uri1 = 'mysqlx://mike:@10.55.120.48:33060';
	$session_uri2 = 'mysqlx://Arnold:@localhost:1001/?ca-path=path/to/ca&'.$disable_ssl_opt;

        $new_uri_config = $session->save($session_uri_name1,$session_uri1);
	$new_uri_config2 = $session->save($session_uri_name2,$session_uri2);

        $json = '{ "host": "10.55.120.48", "port": 33060, "user": "mike", "ssl-mode":"disabled", "appdata": { "alias":"Master", "test":"valid"} }';
	$json2 = '{ "host": "superhost", "port": 333, "user": "Charlie", "ssl-mode":"disabled", "appdata": { "animal":"Dog", "stuff":"a lot", "alias": "Important"} }';
	$json3 = '{ "host": "localhost", "port": 1001, "user": "Arnold", "ssl-mode":"disabled", "important":"field", "moreImportant":"field", "appdata": { "cool":"Yes", "stuff":"not that much", "alias": "nope"} }';

        $new_config;
	$new_config2;
	$new_config3;

        try {
	    $new_config = $session->save('name',$json);
	} catch(Exception $e ) {
	    test_step_failed();
	}
	try {
	    $new_config2 = $session->save('name2',$json2);
	} catch(Exception $e ) {
	    test_step_failed();
	}
	try {
	    $new_config3 = $session->save('superName',$json3);
	} catch(Exception $e ) {
	    test_step_failed();
	}

        $sessions = expect_list( 5 );
	$config1 = $session->get( 'name' );
	$config2 = $session->get( 'name2' );
	$config3 = $session->get( 'superName' );

        expect_eq( $new_config->getName(), $config1->getName() );
	expect_eq( $new_config2->getName(), $config2->getName() );
	expect_eq( $new_config3->getName(), $config3->getName() );

        expect_eq( $new_config->getUri(), $config1->getUri() );
	expect_eq( $new_config2->getUri(), $config2->getUri() );
	expect_eq( $new_config3->getUri(), $config3->getUri() );

        expect_eq( $config1->getUri(), "mysqlx://mike:@10.55.120.48:33060/?".$disable_ssl_opt );
	expect_eq( $config2->getUri(), "mysqlx://Charlie:@superhost:333/?".$disable_ssl_opt );
	expect_eq( $config3->getUri(), "mysqlx://Arnold:@localhost:1001/?important=field&moreImportant=field&".$disable_ssl_opt );

        expect_eq( $new_config->getAppData('alias'), $config1->getAppData('alias') );
	expect_eq( $new_config->getAppData('test'), $config1->getAppData('test') );
	expect_eq( $new_config->getAppData('alias'), 'Master');
	expect_eq( $new_config->getAppData('test'), 'valid');

        expect_eq( $new_config2->getAppData('animal'), $config2->getAppData('animal') );
	expect_eq( $new_config2->getAppData('stuff'), $config2->getAppData('stuff') );
	expect_eq( $new_config2->getAppData('alias'), $config2->getAppData('alias') );
	expect_eq( $new_config2->getAppData('animal'), 'Dog');
	expect_eq( $new_config2->getAppData('stuff'), 'a lot');
	expect_eq( $new_config2->getAppData('alias'), 'Important');

        expect_eq( $new_config3->getAppData('animal'), $config3->getAppData('animal') );
	expect_eq( $new_config3->getAppData('stuff'), $config3->getAppData('stuff') );
	expect_eq( $new_config3->getAppData('alias'), $config3->getAppData('alias') );
	expect_eq( $new_config3->getAppData('cool'), 'Yes');
	expect_eq( $new_config3->getAppData('stuff'), 'not that much');
	expect_eq( $new_config3->getAppData('alias'), 'nope');

        //Delete all the configuration data!
	expect_true( $session->delete( 'name') );
	expect_true( $session->delete( 'name2' ) );
	expect_true( $session->delete( 'superName' ) );

        $sessions = expect_list( 2 );

        try {
	    $config1 = $session->save('aaa',$json);
	} catch(Exception $e ) {
	    test_step_failed();
	}
	try {
	    $config2 = $session->save('bbb',$json2);
	} catch(Exception $e ) {
	    test_step_failed();
	}
	try {
	    $config3 = $session->save('ccc',$json3);
	} catch(Exception $e ) {
	    test_step_failed();
	}

        $sessions = expect_list( 5 );

        $new_uri = "mysqlx://Murphy:topsecret@tigerhost:666/?".$disable_ssl_opt;
	$config2->setUri($new_uri);
	$config2_before_update = $session->get('bbb');

        expect_eq( $config1->getUri(), "mysqlx://mike:@10.55.120.48:33060/?".$disable_ssl_opt );
	expect_eq( $config2_before_update->getUri(), "mysqlx://Charlie:@superhost:333/?".$disable_ssl_opt );
	expect_eq( $config3->getUri(), "mysqlx://Arnold:@localhost:1001/?important=field&moreImportant=field&".$disable_ssl_opt );

        $session->save( $config2 );
	$config2_after_save = $session->get('bbb');

        expect_eq( $config1->getUri(), "mysqlx://mike:@10.55.120.48:33060/?".$disable_ssl_opt );
	expect_eq( $config2_after_save->getUri(), $new_uri );
	expect_eq( $config3->getUri(), "mysqlx://Arnold:@localhost:1001/?important=field&moreImportant=field&".$disable_ssl_opt );

        $config2 = $session->get( 'bbb' );

        expect_true( $config2->setAppData('KeyData','HereWeGo') );
	expect_true( $config2->deleteAppData('stuff') );
	expect_true( $config2->deleteAppData('alias') );

        expect_eq( $config2->getAppData('KeyData'), 'HereWeGo');
	expect_eq( $config2->getAppData('animal'), 'Dog');
	expect_false( $config2->deleteAppData('stuff') );
	expect_false( $config2->deleteAppData('alias') );

        expect_true( $session->delete( 'aaa' ) );

function verification_step() {
        global $session;
	global $session_uri_name1;
	global $session_uri_name2;
	global $session_uri1;
	global $session_uri2;

        $session_list = $session->list();
	expect_eq( count( $session_list ), 4 );

        $config = $session->get( $session_uri_name1 );
	$config2 = $session->get( $session_uri_name2 );

        expect_eq( $config->getName(), $session_uri_name1 );
	expect_eq( $config2->getName(), $session_uri_name2 );

        expect_eq( $config->getUri(), $session_uri1 );
	expect_eq( $config2->getUri(), $session_uri2 );
}

        verification_step();

        expect_true( $session->delete( $session_uri_name1 ) );
	expect_true( $session->delete( $session_uri_name2 ) );

        expect_list( 2 );

        $session->save( $new_uri_config );
	$session->save( $new_uri_config2 );

        verification_step();

        $new_uri_config->setUri('thisIsANewURI');
	$new_uri_config2->setUri('thisIsANewURI2');

        $session->save( $new_uri_config );
	$session->save( $new_uri_config2 );

        $config = $session->get( $session_uri_name1 );
	$config2 = $session->get( $session_uri_name2 );

        expect_eq( $config->getUri(), 'thisIsANewURI' );
	expect_eq( $config2->getUri(), 'thisIsANewURI2' );

        expect_true( $session->delete( $session_uri_name1 ) );
	expect_true( $session->delete( $session_uri_name2 ) );

        expect_list( 2 );

        $session_name = 'coolSession';
	$session_uri = 'mysqlx://charlie:password@92.11.110.48:22000/'.$disable_ssl_opt;
	$app_data_json = '{ "name":"Carlos", "stuff":"tons and tons", "alias": "important"}';

        $config = $session->save( $session_name, $session_uri, $app_data_json );
	expect_list( 3 );

        expect_eq( $config->getAppData('name'), 'Carlos');
	expect_eq( $config->getAppData('stuff'), 'tons and tons');
	expect_eq( $config->getAppData('alias'), 'important');
	expect_eq( $config->getName(), $session_name );
	expect_eq( $config->getUri(), $session_uri );

        expect_true( $session->delete( $session_name ) );
	expect_list( 2 );

        verify_expectations();
	print "done!\n";

?>
--CLEAN--
<?php
        require("connect.inc");
?>
--EXPECTF--
done!%A
