<?php
	function on_row($row, $meta) {
		echo " -------------BEGIN ON_ROW --------------\n";
		var_dump(func_get_args()[1]);
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
	$a->pid=getmypid(); $a->foo="bar";$a->b->c="d";$a->b->d=[1,2];
	
	var_dump($c->generateUUID());
//	var_dump($clients = $c->listClients());
//	$client_id = $clients[0]["client_id"];
//	var_dump("client_id=",$client_id);
//	var_dump($c->killClient($client_id));

//	$collection =  $c->createSchema("test")->createCollection("newcollection");
//	$collection =  $c->getSchema("test")->createCollection("newcollection");
	$collection =  $c->getSchema("test")->getCollection("newcollection");
	var_dump($collection);
	if (0) {
		if (0) {
			$remover = $collection->remove("pid = :foo");
			var_dump($remover);
			var_dump("bind:", $remover->bind(["foo" => 15895]));
//			var_dump($remover->sort(["_id DESC", "pid DESC"]));
//			var_dump($remover->limit(1));
			var_dump("execute:", $remover->execute());
		} else {
			$collection->add(json_encode($a))->execute();
		}
		exit;
	} else if (0) {
//		$modifier = $collection->modify("pid = :foobar");
		$modifier = $collection->modify("pid = \"a8fc6daff27c11e5a8d17c7a913074ba\"");
		var_dump("modifier",$modifier);
//		var_dump("bind:", $modifier->bind(["foobar" => 15889]));
//		var_dump("expression", $expr = mysqlx\expression(".pid - 20"));
//		var_dump("set:", $modifier->set(".foo", $expr));
//		var_dump("unset:", $modifier->unset([".foo"]));
		var_dump("arrayInsert:", $modifier->arrayInsert("[*]", 3));
		var_dump("execute:", $modifier->execute());
		exit;
	} else {
		$finder = $collection->find();
		var_dump("finder:", $finder);
		var_dump("execute:", $res = $finder->execute());
		var_dump($res->fetchAll());
		exit;
	}
	//	var_dump($collection->drop());
	if (0) {
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
//		var_dump($schema->drop());
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
	exit;
}
	
	
if (1) {
	var_dump("server_version:", $c->getServerVersion());
	var_dump("client_id", $c->getClientId());
//	var_dump("create_schema(test2):",$c->createSchema("test2"));
//	var_dump("create_schema(test2):",$c->createSchema("test2"));
//	var_dump("drop_schema(test2):",$c->dropSchema("test2"));
	$stmt = $c->createStatement("SELECT a,b,c,d,tm,size FROM test.t3 LIMIT 5");
//	$stmt = $c->createStatement("SELECT * FROM test.t_insert LIMIT 2");
//	$stmt = $c->createStatement("SELECT CAST('a' AS UNSIGNED)");
//	$stmt = $c->createStatement("CALL test.p1");
	echo "-------------------------- executeWithCallback ------------------\n";
	$stmt->execute(Mysqlx\NodeSqlStatement::EXECUTE_ASYNC);
	$res = $stmt->getResult("on_row", "on_warning", "on_error", "on_rset_end", "on_stmt_ok", $stmt);
	var_dump($res);
	exit;
}
if (1) {
	define("ASYNC_RUN", 1);
	define("BUFFERED", 0);
	define("USE_FOREACH", 1);

	var_dump("quoteName(abc):", $c->quoteName("abc"));
	var_dump("quoteName(a`b`c):", $c->quoteName("a`b`c"));
//	$schema = $c->getSchema("test");
//	var_dump($schema);
//	var_dump($c->createStatement("COMMIT")->execute());
//	$stmt = $c->createStatement("CREATE TABLE IF NOT EXISTS test.t_insert(a int)");
//	$stmt = $c->createStatement("INSERT INTO test.t_insert VALUES(?)");
	$stmt = $c->createStatement("SELECT a,b,c,d,tm,size FROM test.t3 LIMIT 5");
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
}
?>