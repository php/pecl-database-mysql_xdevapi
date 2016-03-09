<?php
	function on_row($row, $meta) {
		echo " -------------BEGIN ON_ROW --------------\n";
		var_dump(func_get_args());
		printf(" -------------END ON_ROW %d --------------\n", $GLOBALS['$row_count']++);
		return 4;//4;
	}
	function on_warning($stmt) {
		echo " -------------BEGIN ON_WARNING--------------\n";
		var_dump(func_get_args());
		echo " -------------END ON_WARNING --------------\n";
	}
	function on_error($stmt) {
		echo " -------------BEGIN ON_ERROR --------------\n";
		var_dump(func_get_args());
		echo " -------------END ON_ERROR --------------\n";
	}
	function on_rset_end($stmt) {
		echo " -------------BEGIN ON_RSET_END --------------\n";
		var_dump(func_get_args());
		echo " -------------END ON_RSET_END --------------\n";
	}
	function on_stmt_ok($stmt) {
		echo " -------------BEGIN ON_STMT_OK --------------\n";
		var_dump(func_get_args());
		echo " -------------END ON_STMT_OK --------------\n";
	}

	$row_count = 0;

	define("ASYNC_RUN", 0);
	define("BUFFERED", 1);
	define("USE_FOREACH", 0);

	$c=mysqlx\getNodeSession("127.0.0.1", "root","");

	var_dump($c);
if (0) {
//	$res = $c->executeSql("INSERT INTO test.t_insert VALUES(?)", NULL, 42);
//	$res = $c->executeSql("SELECT * FROM test.t_insert LIMIT 2");
	$res = $c->executeSql("SELECT CAST('a' AS UNSIGNED)");
	var_dump($res);
//	while ($res->hasData()) {
//		var_dump($res->fetchOne());
//	}
	exit;
}
if (1) {
	$a = new stdclass();
	$a->b = new stdclass();
	$a->__id=1; $a->foo="bar";$a->b->c="d";$a->b->d=[1,2];
	
	var_dump($c->generateUUID());
//	var_dump($clients = $c->listClients());
//	$client_id = $clients[0]["client_id"];
//	var_dump("client_id=",$client_id);
//	var_dump($c->killClient($client_id));

	$collection =  $c->getSchema("test")->createCollection("newcollection");
	$collection->add($a)->execute();
//	$collection->add(json_encode($a))->execute();
	var_dump($collection->drop());
exit;
	var_dump($c->getSchemas());

	$schema = $c->getSchema("test");

	$mycollection = $schema->createCollection("mycollection");
	var_dump("created_collection:", $mycollection);

	$tables = $schema->getTables();
	var_dump("tables:", $tables);

	$collections = $schema->getCollections();
	var_dump("collections:", $collections);

	var_dump($mycollection->drop());
exit;
//	var_dump($schema->drop());
	var_dump($schema);
	var_dump($schema->getName());
	$mycollection = $schema->createCollection("mycollection");
	var_dump("created_collection:", $mycollection);
	var_dump($mycollection->drop());

	$collection = $schema->getCollection("t_insert");
	var_dump("collection:", $collection);
	$table = $schema->getTable("t_insert");
	var_dump("table:", $table);
	$collections = $schema->getCollections();
	var_dump("collections:", $collections);
	$tables = $schema->getTables();
	var_dump("tables:", $tables);
	$inserter = $table->insert();
	var_dump("inserter:", $inserter);
	$deleter = $table->delete();
	var_dump("deleter:", $deleter);
	$updater = $table->update();
	var_dump("updater:", $updater);
	$selecter = $table->select();
	var_dump("selecter:", $selecter);
	exit;
}
	
	
if (0) {
	var_dump("server_version:", $c->getServerVersion());
	var_dump("client_id", $c->getClientId());
	var_dump("create_schema(test2):",$c->createSchema("test2"));
	var_dump("create_schema(test2):",$c->createSchema("test2"));
	var_dump("drop_schema(test2):",$c->dropSchema("test2"));
	$stmt = $c->createStatement("SELECT * FROM test.t_insert LIMIT 2");
//	$stmt = $c->createStatement("SELECT CAST('a' AS UNSIGNED)");
//	$stmt = $c->createStatement("CALL test.p1");
	echo "-------------------------- executeWithCallback ------------------\n";
	$stmt->execute(Mysqlx\NodeSqlStatement::EXECUTE_ASYNC);
	$res = $stmt->getResult("on_row", "on_warning", "on_error", "on_rset_end", "on_stmt_ok", $stmt);
	var_dump($res);
	exit;
}
	var_dump("quoteName(abc):", $c->quoteName("abc"));
	var_dump("quoteName(a`b`c):", $c->quoteName("a`b`c"));
//	$schema = $c->getSchema("test");
//	var_dump($schema);
//	var_dump($c->createStatement("COMMIT")->execute());
//	$stmt = $c->createStatement("CREATE TABLE IF NOT EXISTS test.t_insert(a int)");
//	$stmt = $c->createStatement("INSERT INTO test.t_insert VALUES(?)");
	$stmt = $c->createStatement("SELECT c,size FROM test.t3 LIMIT 5");
//	var_dump($stmt->bind(0,NULL));
//	var_dump($stmt->bind(0,"large"));
//	var_dump($stmt->bind(0,"bar"));
//	var_dump($stmt->bind(0, 1)->bind(0, 1)->bind(0, 1)->bind(0, 1)->bind(0, 1));
//	$stmt = $c->createStatement("call test.p1()");
	var_dump($stmt);

	echo BUFFERED? "BUFFERED":"FORWARD ONLY", "\n";
	echo USE_FOREACH? "FOREACH":"DevAPI iteration", "\n";
	if (ASYNC_RUN) {
		echo "ASYNC\n";
		var_dump($stmt->execute(Mysqlx\NodeSqlStatement::EXECUTE_ASYNC | (BUFFERED? Mysqlx\NodeSqlStatement::BUFFERED : 0)));
		do {
			$res = $stmt->getResult();
			var_dump("get_result:",$res);
			var_dump("getLastInsertId:",$res->getLastInsertId());
			var_dump("getAffectedItemsCount:",$res->getAffectedItemsCount());
			if (USE_FOREACH) {
				foreach ($res as $row) {
					var_dump($row);
				}
			} else {
				while ($res->hasData()) {
					var_dump($res->fetchOne());
				}
//				var_dump($res->fetchAll());
			}
		} while ($stmt->hasMoreResults() && ($res = $stmt->nextResult()));
	} else {
		echo "SYNCHRONOUS\n";
		$res = $stmt->execute(BUFFERED? Mysqlx\NodeSqlStatement::BUFFERED:0);
		var_dump($res);
		do {
			var_dump("getLastInsertId:",$res->getLastInsertId());
			var_dump("getAffectedItemsCount:",$res->getAffectedItemsCount());
			if (USE_FOREACH) {
				foreach ($res as $row) {
					unset($res);
					var_dump($row);
				}			
			} else {
				while ($res->hasData()) {
					var_dump($res->fetchOne());
				}
				if (BUFFERED) {
//					var_dump($res->fetchAll());
				}
			}
			var_dump("has_more_results=", $stmt->hasMoreResults());
		} while ($stmt->hasMoreResults() && ($res = $stmt->nextResult()));
	}
?>