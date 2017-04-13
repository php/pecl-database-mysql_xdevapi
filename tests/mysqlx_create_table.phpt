--TEST--
mysqlx create table
--SKIPIF--
--FILE--
<?php
	require("connect.inc");

	use mysql_xdevapi\ColumnDef as ColumnDef;
	use mysql_xdevapi\GeneratedColumnDef as GeneratedColumnDef;
	use mysql_xdevapi\ForeignKeyDef as ForeignKeyDef;

	// -------------
	// execution
	// -------------

	function exec_create_table($createTableCmd, $expect_success = true) {
		try {
			print $createTableCmd->getSqlQuery().PHP_EOL;
			$createTableCmd->execute();
			expect_true($expect_success);
		} catch (Exception $e) {
			expect_false($expect_success);
		}
		echo "--------", PHP_EOL;
	}

	// -------------
	// init
	// -------------

	$nodeSession = create_test_db();
	fill_db_table();
	create_test_view($nodeSession);
	$schema = $nodeSession->getSchema($db);

	// -------------
	// columns
	// -------------

	$numericTypes = $schema->createTable('numericTypes')
		->addColumn((new ColumnDef('num_bit', 'BIT', 2)))
		->addColumn((new ColumnDef('num_tinyint', 'TINYINT', 8))->unsigned())
		->addColumn((new ColumnDef('num_smallint', 'SMALLINT', 16))->unsigned())
		->addColumn((new ColumnDef('num_mediumint', 'MEDIUMINT', 32))->unsigned())
		->addColumn((new ColumnDef('num_int', 'INT', 32))->unsigned())
		->addColumn((new ColumnDef('num_integer', 'INTEGER', 32))->unsigned())
		->addColumn((new ColumnDef('num_bigint', 'BIGINT', 64))->unsigned())
		->addColumn((new ColumnDef('num_real', 'REAL', 64))->decimals(10)->unsigned())
		->addColumn((new ColumnDef('num_double', 'DOUBLE', 80))->decimals(16)->unsigned())
		->addColumn((new ColumnDef('num_float', 'FLOAT', 32))->decimals(8)->unsigned())
		->addColumn((new ColumnDef('num_decimal', 'DECIMAL', 24))->decimals(20)->unsigned())
		->addColumn((new ColumnDef('num_numeric', 'NUMERIC', 32))->decimals(30)->unsigned());
	exec_create_table($numericTypes);

	$timeTypes = $schema->createTable('timeTypes')
		->addColumn((new ColumnDef('time_date', 'DATE')))
		->addColumn((new ColumnDef('time_time', 'TIME')))
		->addColumn((new ColumnDef('time_timestamp', 'TIMESTAMP'))->defaultCurrentTimestamp())
		->addColumn((new ColumnDef('time_datetime', 'DATETIME')))
		->addColumn((new ColumnDef('time_year', 'YEAR')));
	exec_create_table($timeTypes);

	$textTypes = $schema->createTable('textTypes')
		->addColumn((new ColumnDef('text_char', 'CHAR'))->binary()->charset('latin1')->collation('latin1_swedish_ci'))
		->addColumn((new ColumnDef('text_varchar', 'VARCHAR', 32))->binary()->charset('latin2')->collation('latin2_general_ci'))
		->addColumn((new ColumnDef('text_tinytext', 'TINYTEXT'))->binary()->charset('latin5')->collation('latin5_turkish_ci'))
		->addColumn((new ColumnDef('text_text', 'TEXT'))->binary()->charset('latin7')->collation('latin7_general_ci'))
		->addColumn((new ColumnDef('text_mediumtext', 'MEDIUMTEXT'))->binary()->charset('latin1')->collation('latin1_german2_ci'))
		->addColumn((new ColumnDef('text_longtext', 'LONGTEXT'))->binary()->charset('latin1')->collation('latin1_german1_ci'));
	exec_create_table($textTypes);

	$dataTypes = $schema->createTable('dataTypes')
		->addColumn(new ColumnDef('data_binary', 'BINARY', 16))
		->addColumn(new ColumnDef('data_varbinary', 'VARBINARY', 24))
		->addColumn(new ColumnDef('data_tinyblob', 'TINYBLOB'))
		->addColumn(new ColumnDef('data_blob', 'BLOB'))
		->addColumn(new ColumnDef('data_mediumblob', 'MEDIUMBLOB'))
		->addColumn(new ColumnDef('data_longblob', 'LONGBLOB'));
	exec_create_table($dataTypes);

	$otherTypes = $schema->createTable('otherTypes')
		->addColumn((new ColumnDef('rating', 'Enum'))->values('G', 'PG', 'PG-13', 'R', 'NC-17')->setDefault('G')
			->charset('ucs2')->collation('ucs2_general_ci'))
		->addColumn((new ColumnDef('special_features', 'Set'))
			->values('Trailers', 'Commentaries', 'Deleted Scenes', 'Behind the Scenes' )->setDefault(null)
			->charset('utf8mb4')->collation('utf8mb4_general_ci'))
		->addColumn((new ColumnDef('json_doc', 'json')));
	exec_create_table($otherTypes);

	// -------------
	// columns errors
	// -------------

	$numericTypesError = $schema->createTable('numericTypesError')
		->addColumn((new ColumnDef('num_bit', 'BIT', 33))->charset('latin1'));
	exec_create_table($numericTypesError, false);

	$numericTypesError = $schema->createTable('numericTypesError')
		->addColumn((new ColumnDef('num_tinyint', 'TINYINT', -1))->unsigned());
	exec_create_table($numericTypesError, false);

	$numericTypesError = $schema->createTable('numericTypesError')
		->addColumn((new ColumnDef('num_smallint', 'SMALLINT', 16))->unsigned()->setDefault('xyz')->values('x','y','z'));
	exec_create_table($numericTypesError, false);

	$numericTypesError = $schema->createTable('numericTypesError')
		->addColumn((new ColumnDef('num_integer', 'INTEGER', 512))->unsigned()->collation('latin2_general_ci'));
	exec_create_table($numericTypesError, false);

	$numericTypesError = $schema->createTable('numericTypesError')
		->addColumn((new ColumnDef('num_float', 'FLOAT', 256))->decimals(8)->unsigned());
	exec_create_table($numericTypesError, false);

	$numericTypesError = $schema->createTable('numericTypesError')
		->addColumn((new ColumnDef('num_double', 'DOUBLE', 80))->binary()->unsigned());
	exec_create_table($numericTypesError, false);

	$numericTypesError = $schema->createTable('numericTypesError')
		->addColumn((new ColumnDef('num_numeric', 'NUMERIC', 32))->charset('latin1')->decimals(30)->unsigned());
	exec_create_table($numericTypesError, false);

	// -------------

	$timeTypesError = $schema->createTable('timeTypesError')
		->addColumn((new ColumnDef('time_date', 'DATE', 5)));
	exec_create_table($timeTypesError, false);

	$timeTypesError = $schema->createTable('timeTypesError')
		->addColumn((new ColumnDef('time_time', 'TIME'))->unsigned());
	exec_create_table($timeTypesError, false);

	$timeTypesError = $schema->createTable('timeTypesError')
		->addColumn((new ColumnDef('time_timestamp', 'TIMESTAMP'))->charset('latin5')->defaultCurrentTimestamp());
	exec_create_table($timeTypesError, false);

	$timeTypesError = $schema->createTable('timeTypesError')
		->addColumn((new ColumnDef('time_datetime', 'DATETIME'))->notNull()
			->binary()->collation('latin5_turkish_ci'));
	exec_create_table($timeTypesError, false);

	$timeTypesError = $schema->createTable('timeTypesError')
		->addColumn((new ColumnDef('time_year', 'YEAR', 2))->decimals(30));
	exec_create_table($timeTypesError, false);

	// -------------

	$textTypesError = $schema->createTable('textTypesError')
		->addColumn((new ColumnDef('text_char', 'CHAR'))->binary()->charset('latin1')->collation('non_existing_coll'));
	exec_create_table($textTypesError, false);

	$textTypesError = $schema->createTable('textTypesError')
		->addColumn((new ColumnDef('text_varchar', 'VARCHAR', 32))->binary()->charset('non_existing_charset')->collation('latin2_general_ci'));
	exec_create_table($textTypesError, false);

	$textTypesError = $schema->createTable('textTypesError')
		->addColumn((new ColumnDef('text_tinytext', 'TINYTEXT', 3))->binary()->charset('latin5')->collation('latin5_turkish_ci'));
	exec_create_table($textTypesError, false);

	$textTypesError = $schema->createTable('textTypesError')
		->addColumn((new ColumnDef('text_text', 'TEXT', 10))->decimals(30));
	exec_create_table($textTypesError, false);

	$textTypesError = $schema->createTable('textTypesError')
		->addColumn((new ColumnDef('text_mediumtext', 'MEDIUMTEXT'))->unsigned()->charset('latin1')->collation('latin1_german2_ci'));
	exec_create_table($textTypesError, false);

	// -------------

	$dataTypesError = $schema->createTable('dataTypesError')
		->addColumn((new ColumnDef('data_binary', 'BINARY', 16))->decimals(30));
	exec_create_table($dataTypesError, false);

	$dataTypesError = $schema->createTable('dataTypesError')
		->addColumn((new ColumnDef('data_varbinary', 'VARBINARY'))->collation('latin1_german1_ci'));
	exec_create_table($dataTypesError, false);

	$dataTypesError = $schema->createTable('dataTypesError')
		->addColumn((new ColumnDef('data_tinyblob', 'TINYBLOB'))->charset('latin1'));
	exec_create_table($dataTypesError, false);

	$dataTypesError = $schema->createTable('dataTypesError')
		->addColumn((new ColumnDef('data_blob', 'BLOB'))->unsigned());
	exec_create_table($dataTypesError, false);

	$dataTypesError = $schema->createTable('dataTypesError')
		->addColumn((new ColumnDef('data_mediumblob', 'MEDIUMBLOB'))->binary());
	exec_create_table($dataTypesError, false);

	$dataTypesError = $schema->createTable('dataTypesError')
		->addColumn((new ColumnDef('data_longblob', 'LONGBLOB'))->setDefault('abc'));
	exec_create_table($dataTypesError, false);

	// -------------

	$otherTypesError = $schema->createTable('otherTypesError')
		->addColumn((new ColumnDef('rating', 'Enum'))->values('G', 'PG', 'PG-13', 'R', 'NC-17')->setDefault('T'));
	exec_create_table($otherTypesError, false);

	$otherTypesError = $schema->createTable('otherTypesError')
		->addColumn((new ColumnDef('rating', 'Enum'))->unsigned());
	exec_create_table($otherTypesError, false);

	$otherTypesError = $schema->createTable('otherTypesError')
		->addColumn((new ColumnDef('rating', 'Enum'))->values(1, 2, 3)->setDefault(1));
	exec_create_table($otherTypesError, false);

	$otherTypesError = $schema->createTable('otherTypesError')
		->addColumn((new ColumnDef('special_features', 'SET'))
			->values('Trailers', 'Commentaries', 'Deleted Scenes', 'Behind the Scenes' )->setDefault('XYZ'));
	exec_create_table($otherTypesError, false);

	$otherTypesError = $schema->createTable('otherTypesError')
		->addColumn(new ColumnDef('special_features', 'SET', 5));
	exec_create_table($otherTypesError, false);

	$otherTypesError = $schema->createTable('otherTypesError')
		->addColumn((new ColumnDef('doc', 'json'))->charset('utf8'));
	exec_create_table($otherTypesError, false);

	// -------------
	// create table AS
	// -------------

	$selectAsTestTable = $schema->createTable('select_as_'.$test_table_name)
		->addColumn((new ColumnDef('copied_name', 'text')))
		->addColumn((new ColumnDef('copied_age', 'int')))
		->as('SELECT name, age FROM '.$db.'.'.$test_table_name);
	exec_create_table($selectAsTestTable);

	$selectAsTestCollection = $schema->createTable('select_as_'.$test_collection_name)
		->addColumn((new ColumnDef('copied_id', 'VARCHAR', 32))->notNull()->uniqueIndex()->setDefault('0'))
		->addColumn((new ColumnDef('copied_doc', 'json')))
		->as('SELECT _id, doc FROM '.$db.'.'.$test_collection_name);
	exec_create_table($selectAsTestCollection);

	$selectAsTestView = $schema->createTable('select_as_'.$test_view_name)
		->addColumn((new ColumnDef('copied_name', 'text')))
		->as('SELECT name FROM '.$db.'.'.$test_view_name);
	exec_create_table($selectAsTestView);

	// -------------
	// create table AS errors
	// -------------

	$selectAsTestTableError = $schema->createTable('selectAsTestTableError')
		->addColumn((new ColumnDef('copied_name', 'VARCHAR')))
		->addColumn((new ColumnDef('copied_age', 'real')))
		->as('SELECT name, age FROM '.$db.'.'.$test_table_name);
	exec_create_table($selectAsTestTableError, false);

	$selectAsTestCollectionError = $schema->createTable('selectAsTestCollectionError')
		->addColumn((new ColumnDef('copied_id', 'VARCHAR', 32))->notNull()->uniqueIndex()->setDefault('0'))
		->addColumn((new ColumnDef('copied_doc', 'json')))
		->as('SELECT _id, doc, non_existing_column FROM '.$db.'.'.$test_collection_name);
	exec_create_table($selectAsTestCollectionError, false);

	$selectAsTestViewError = $schema->createTable('selectAsTestViewError')
		->addColumn((new ColumnDef('PostalCode', 'int')))
		->as('SELECT PostalCode FROM '.$db.'.'.$test_view_name);
	exec_create_table($selectAsTestViewError, false);

	// -------------
	// create table LIKE
	// -------------

	$testTableClone = $schema->createTable($test_table_name.'_clone')
		->like($db.'.'.$test_table_name);
	exec_create_table($testTableClone);

	$testCollectionClone = $schema->createTable($test_collection_name.'_clone')
		->like($db.'.'.$test_collection_name);
	exec_create_table($testCollectionClone);

	$testViewClone = $schema->createTable($test_view_name.'_clone')
		->like($db.'.'.$test_view_name);
	exec_create_table($testViewClone, false);

	// -------------
	// create table LIKE errors
	// -------------

	$testTableCloneError = $schema->createTable('testTableCloneError')
		->like('non_existing_table');
	exec_create_table($testTableCloneError, false);

	$testCollectionCloneError = $schema->createTable('testCollectionCloneError')
		->addColumn((new ColumnDef('id', 'SMALLINT'))->unsigned()->notNull()->autoIncrement())
		->like($db.'.'.$test_collection_name.'_non_existing');
	exec_create_table($testCollectionCloneError, false);

	// -------------
	// table options
	// -------------

	$rowIdColumn = new ColumnDef('row_id', 'Smallint');
	$rowIdColumn->primaryKey()->autoIncrement()->unsigned();
	$optionsTable = $schema->createTable('optionsTable', true)
		->addColumn($rowIdColumn)
		->setInitialAutoIncrement(2017)
		->setDefaultCharset('utf8')
		->setDefaultCollation('utf8_general_ci')
		->setComment('this is comment for table options')
		->temporary();
	exec_create_table($optionsTable);

	// -------------
	// table options errors
	// -------------

	$optionsTableError = $schema->createTable('optionsTableError', true)
		->setInitialAutoIncrement(-1);
	exec_create_table($optionsTableError, false);

	$optionsTableError = $schema->createTable('optionsTableError', true)
		->addColumn($rowIdColumn)
		->temporary()
		->setDefaultCharset('unreal-charset');
	exec_create_table($optionsTableError, false);

	$optionsTableError = $schema->createTable('optionsTableError', true)
		->addColumn($rowIdColumn)
		->setDefaultCollation('invalid_collation');
	exec_create_table($optionsTableError, false);

	// -------------
	// primary key / foreign key
	// -------------

	$usersTable = $schema->createTable('users')
		->addColumn((new ColumnDef('user_id', 'INT'))->autoIncrement()->primaryKey())
		->addColumn((new ColumnDef('username', 'VARCHAR', 40)))
		->addColumn((new ColumnDef('password', 'VARCHAR', 255)))
		->addColumn((new ColumnDef('email', 'VARCHAR', 255)));
	exec_create_table($usersTable);

	$rolesTable = $schema->createTable('roles')
		->addColumn((new ColumnDef('role_id', 'INT'))->notNull()->autoIncrement())
		->addColumn((new ColumnDef('role_name', 'VARCHAR', 50))->notNull())
		->addPrimaryKey('role_id', 'role_name');
	exec_create_table($rolesTable);

	// -------------

	$personTable = $schema->createTable('person')
		->addColumn((new ColumnDef('id', 'SMALLINT'))->unsigned()->notNull()->autoIncrement())
		->addColumn((new ColumnDef('name', 'CHAR', 60))->notNull())
		->addPrimaryKey('id');
	exec_create_table($personTable);

	$shirtTable = $schema->createTable('shirt')
		->addColumn((new ColumnDef('id', 'SMALLINT'))->unsigned()->notNull()->autoIncrement())
		->addColumn((new ColumnDef('style', 'ENUM'))->values('t-shirt', 'polo', 'dress')->notNull())
		->addColumn((new ColumnDef('color', 'ENUM'))->values('red', 'blue', 'orange', 'white', 'black')->notNull())
		->addColumn((new ColumnDef('owner', 'SMALLINT'))->unsigned()->notNull()->foreignKey('person', 'id'))
		->addPrimaryKey('id');
	exec_create_table($shirtTable);

	// -------------

	$employeeTable = $schema->createTable('employee')
		->addColumn((new ColumnDef('name', 'VARCHAR', 20))->notNull()->comment('employee\'s name'))
		->addColumn((new ColumnDef('surname', 'VARCHAR', 20))->notNull()->comment('employee\'s surname'))
		->addColumn((new ColumnDef('PESEL', 'CHAR', 11))->notNull()->comment('employee\'s PESEL'))
		->addColumn((new ColumnDef('position', 'TinyText'))->comment('employee\'s Position'))
		->addPrimaryKey('name', 'surname', 'PESEL');
	exec_create_table($employeeTable);

	$positionTable = $schema->createTable('position')
		->addColumn((new ColumnDef('emp_name', 'VARCHAR', 20))->foreignKey('employee', 'name'))
		->addColumn((new ColumnDef('emp_surname', 'STRING', 30))->foreignKey('employee', 'surname'))
		->addColumn((new ColumnDef('emp_PESEL', 'CHAR', 11))->foreignKey('employee', 'PESEL'))
		->addColumn((new ColumnDef('description', 'TinyText'))->comment('employee\'s Position'));
	exec_create_table($positionTable);

	// -------------

	$carTable = $schema->createTable('car')
		->addColumn((new ColumnDef('VIN', 'CHAR', 20))->notNull()->primaryKey())
		->addColumn((new ColumnDef('brand', 'VARCHAR', 30))->notNull())
		->addColumn((new ColumnDef('model', 'VARCHAR', 50))->notNull())
		->addUniqueIndex('VIN_index', 'VIN', 'brand', 'model');
	exec_create_table($carTable);

	$driverCarTable = $schema->createTable('driverCar')
		->addColumn((new ColumnDef('driver', 'VARCHAR', 40)))
		->addColumn((new ColumnDef('car_VIN', 'CHAR', 20))->notNull())
		->addForeignKey('fk_car_VIN',
			(new ForeignKeyDef())->fields('car_VIN')->refersTo($db.'.'.'car', 'VIN')
				->onDelete('no action')->onUpdate('Cascade'));
	exec_create_table($driverCarTable);

	// -------------
	// primary key / foreign key errors
	// -------------

	$usersTableError = $schema->createTable('usersTableError')
		->addColumn((new ColumnDef('user_id', 'INT'))->autoIncrement()->primaryKey())
		->addColumn((new ColumnDef('username', 'VARCHAR', 40))->notNull()->primaryKey())
		->addColumn((new ColumnDef('password', 'VARCHAR', 255)));
	exec_create_table($usersTableError, false);

	$rolesTableError = $schema->createTable('rolesTableError')
		->addColumn((new ColumnDef('role_id', 'INT'))->notNull()->autoIncrement())
		->addColumn((new ColumnDef('role_name', 'VARCHAR', 50)))
		->addPrimaryKey('role_id', 'role_name');
	exec_create_table($rolesTableError, false);

	// -------------

	$personTableError = $schema->createTable('personTableError')
		->addColumn((new ColumnDef('id', 'SMALLINT'))->unsigned())
		->addColumn((new ColumnDef('name', 'CHAR', 60))->notNull())
		->addPrimaryKey('id');
	exec_create_table($personTableError, false);

	$shirtTableError = $schema->createTable('shirtTableError')
		->addColumn((new ColumnDef('id', 'SMALLINT'))->unsigned()->notNull()->autoIncrement())
		->addColumn((new ColumnDef('style', 'ENUM'))->values('t-shirt', 'polo', 'dress')->notNull())
		->addColumn((new ColumnDef('color', 'ENUM'))->values('red', 'blue', 'orange', 'white', 'black')->notNull())
		->addColumn((new ColumnDef('owner', 'SMALLINT'))->unsigned()->notNull()->foreignKey('person', 'name'));
	exec_create_table($shirtTableError, false);

	// -------------

	$employeeTableError = $schema->createTable('employeeTableError')
		->addColumn((new ColumnDef('name', 'VARCHAR', 20))->notNull()->comment('employee\'s name'))
		->addColumn((new ColumnDef('surname', 'VARCHAR', 20))->notNull()->comment('employee\'s surname'))
		->addColumn((new ColumnDef('PESEL', 'CHAR', 11))->comment('employee\'s PESEL'))
		->addColumn((new ColumnDef('position', 'TinyText'))->comment('employee\'s Position'))
		->addPrimaryKey('surname', 'PESEL', 'position');
	exec_create_table($employeeTableError, false);

	$positionTableError = $schema->createTable('positionTableError')
		->addColumn((new ColumnDef('emp_name', 'SMALLINT', 20))->foreignKey('non_existing_table', 'non_existing_name'))
		->addColumn((new ColumnDef('emp_surname', 'int', 30))->charset('utf8')->foreignKey('non_existing_table', 'surname'))
		->addColumn((new ColumnDef('emp_PESEL', 'CHAR', 11))->foreignKey('non_existing_table', 'PESEL'))
		->addColumn((new ColumnDef('description', 'TinyText'))->comment('employee\'s Position'));
	exec_create_table($positionTableError, false);

	// -------------

	$driverCarTableError = $schema->createTable('driverCarTableError')
		->addColumn((new ColumnDef('driver', 'VARCHAR', 40)))
		->addColumn((new ColumnDef('car_VIN', 'CHAR', 20))->notNull())
		->addForeignKey('fk_car_VIN',
			(new ForeignKeyDef())->fields('car_VIN')->refersTo($db.'.'.'car', 'VIN')
				->onDelete('Set Null')->onUpdate('Restrict'));
	exec_create_table($driverCarTableError, false);

	// -------------
	// indexes
	// -------------

	$citizensTable = $schema->createTable('citizens')
		->addColumn((new ColumnDef('PersonID', 'int'))->autoIncrement()->primaryKey())
		->addColumn((new ColumnDef('LastName', 'varchar', 40)))
		->addColumn((new ColumnDef('FirstName', 'varchar', 30)))
		->addColumn((new ColumnDef('Address', 'varchar', 255)))
		->addColumn((new ColumnDef('City', 'varchar', 100)))
		->addUniqueIndex('citizenIndex', 'PersonID', 'FirstName', 'LastName')
		->addIndex('addressIndex', 'PersonID', 'Address', 'City');
	exec_create_table($citizensTable);

	$cityTable = $schema->createTable('cityTable')
		->addColumn((new ColumnDef('PostalCode', 'varchar', 10))->notNull()->uniqueIndex())
		->addColumn((new ColumnDef('Name', 'varchar', 100)))
		->addColumn((new ColumnDef('Location_longitude', 'double'))->notNull())
		->addColumn((new ColumnDef('Location_latitude', 'double'))->notNull())
		->addIndex('cityIndex', 'PostalCode', 'Name');
	exec_create_table($cityTable);

	// -------------
	// indexes errors
	// -------------

	$citizensTableError = $schema->createTable('citizensTableError')
		->addColumn((new ColumnDef('LastName', 'blob', 40))->setDefault(null))
		->addColumn((new ColumnDef('FirstName', 'varchar', 30))->setDefault('noname'))
		->addUniqueIndex('citizenIndex', 'FirstName', 'LastName');
	exec_create_table($citizensTableError, false);

	$citizensTableError = $schema->createTable('citizensTableError')
		->addColumn((new ColumnDef('Address', 'varchar', 255)))
		->addColumn((new ColumnDef('City', 'varchar', 100)))
		->addUniqueIndex('citizenIndex', 'City', 'PostalCode');
	exec_create_table($citizensTableError, false);

	$citizensTableError = $schema->createTable('citizensTableError')
		->addColumn((new ColumnDef('LastName', 'varchar', 40)))
		->addColumn((new ColumnDef('FirstName', 'varchar', 30)))
		->addIndex('addressIndex', 'FirstName', 'LastName', 'Address');
	exec_create_table($citizensTableError, false);

	$citizensTableError = $schema->createTable('citizensTableError')
		->addColumn((new ColumnDef('LastName', 'varchar', 40)))
		->addColumn((new ColumnDef('FirstName', 'varchar', 30)))
		->addIndex('nonExistingFieldIndex', 'PersonID', 'FirstName', 'LastName');
	exec_create_table($citizensTableError, false);


	$cityTableError = $schema->createTable('cityTableError')
		->addColumn((new ColumnDef('Name', 'text', 100))->binary()->setDefault(5)->uniqueIndex());
	exec_create_table($cityTableError, false);

	$cityTableError = $schema->createTable('cityTableError')
		->addColumn((new ColumnDef('PostalCode', 'varchar', 10))->uniqueIndex())
		->addColumn((new ColumnDef('Name', 'mediumblob', 100))->uniqueIndex())
		->addColumn((new ColumnDef('Location_longitude', 'double'))->uniqueIndex())
		->addColumn((new ColumnDef('Location_latitude', 'double'))->uniqueIndex())
		->addIndex('cityIndex', 'PostalCode', 'Name', 'Location_longitude');
	exec_create_table($cityTableError, false);

	// -------------
	// time defaults
	// -------------

	$timestampTable = $schema->createTable('timestampTable')
		->addColumn((new ColumnDef('id', 'INT'))->notNull()->autoIncrement())
		->addColumn((new ColumnDef('data', 'VARCHAR', 100))->notNull())
		->addColumn((new ColumnDef('time', 'TIMESTAMP'))->defaultCurrentTimestamp())
		->addPrimaryKey('id');
	exec_create_table($timestampTable);

	$timestampTableError = $schema->createTable('timestampTableError')
		->addColumn((new ColumnDef('id', 'INT'))->notNull()->defaultCurrentTimestamp())
		->addColumn((new ColumnDef('data', 'VARCHAR', 100))->notNull())
		->addPrimaryKey('id');
	exec_create_table($timestampTableError, false);

	// -------------

	foreach ($schema->getTables() as $key => $value) {
		echo "{$key}".PHP_EOL;
	}

	verify_expectations();
	print "done!\n";
?>
--CLEAN--
<?php
	require("connect.inc");
	clean_test_db();
?>
--EXPECTF--
CREATE TABLE IF NOT EXISTS testx.numericTypes ( num_bit BIT(2) NULL , num_tinyint TINYINT(8) UNSIGNED NULL , num_smallint SMALLINT(16) UNSIGNED NULL , num_mediumint MEDIUMINT(32) UNSIGNED NULL , num_int INTEGER(32) UNSIGNED NULL , num_integer INTEGER(32) UNSIGNED NULL , num_bigint BIGINT(64) UNSIGNED NULL , num_real REAL(64,10) UNSIGNED NULL , num_double DOUBLE(80,16) UNSIGNED NULL , num_float FLOAT(32,8) UNSIGNED NULL , num_decimal DECIMAL(24,20) UNSIGNED NULL , num_numeric NUMERIC(32,30) UNSIGNED NULL ) 
--------
CREATE TABLE IF NOT EXISTS testx.timeTypes ( time_date DATE NULL , time_time TIME NULL , time_timestamp TIMESTAMP NULL DEFAULT CURRENT_TIMESTAMP , time_datetime DATETIME NULL , time_year YEAR NULL ) 
--------
CREATE TABLE IF NOT EXISTS testx.textTypes ( text_char CHARACTER BINARY CHARACTER SET latin1 COLLATE latin1_swedish_ci NULL , text_varchar VARCHAR(32) BINARY CHARACTER SET latin2 COLLATE latin2_general_ci NULL , text_tinytext TINYTEXT BINARY CHARACTER SET latin5 COLLATE latin5_turkish_ci NULL , text_text TEXT BINARY CHARACTER SET latin7 COLLATE latin7_general_ci NULL , text_mediumtext MEDIUMTEXT BINARY CHARACTER SET latin1 COLLATE latin1_german2_ci NULL , text_longtext LONGTEXT BINARY CHARACTER SET latin1 COLLATE latin1_german1_ci NULL ) 
--------
CREATE TABLE IF NOT EXISTS testx.dataTypes ( data_binary BINARY(16) NULL , data_varbinary VARBINARY(24) NULL , data_tinyblob TINYBLOB NULL , data_blob BLOB NULL , data_mediumblob MEDIUMBLOB NULL , data_longblob LONGBLOB NULL ) 
--------
CREATE TABLE IF NOT EXISTS testx.otherTypes ( rating ENUM('G','PG','PG-13','R','NC-17') CHARACTER SET ucs2 COLLATE ucs2_general_ci NULL DEFAULT 'G' , special_features SET('Trailers','Commentaries','Deleted Scenes','Behind the Scenes') CHARACTER SET utf8mb4 COLLATE utf8mb4_general_ci NULL DEFAULT NULL , json_doc JSON NULL ) 
--------
CREATE TABLE IF NOT EXISTS testx.numericTypesError ( num_bit BIT(33) CHARACTER SET latin1 NULL ) 
--------
CREATE TABLE IF NOT EXISTS testx.numericTypesError ( num_tinyint TINYINT(-1) UNSIGNED NULL ) 
--------
CREATE TABLE IF NOT EXISTS testx.numericTypesError ( num_smallint SMALLINT(16) UNSIGNED NULL DEFAULT 'xyz' ) 
--------
CREATE TABLE IF NOT EXISTS testx.numericTypesError ( num_integer INTEGER(512) UNSIGNED COLLATE latin2_general_ci NULL ) 
--------
CREATE TABLE IF NOT EXISTS testx.numericTypesError ( num_float FLOAT(256,8) UNSIGNED NULL ) 
--------
CREATE TABLE IF NOT EXISTS testx.numericTypesError ( num_double DOUBLE(80) UNSIGNED BINARY NULL ) 
--------
CREATE TABLE IF NOT EXISTS testx.numericTypesError ( num_numeric NUMERIC(32,30) UNSIGNED CHARACTER SET latin1 NULL ) 
--------
CREATE TABLE IF NOT EXISTS testx.timeTypesError ( time_date DATE(5) NULL ) 
--------
CREATE TABLE IF NOT EXISTS testx.timeTypesError ( time_time TIME UNSIGNED NULL ) 
--------
CREATE TABLE IF NOT EXISTS testx.timeTypesError ( time_timestamp TIMESTAMP CHARACTER SET latin5 NULL DEFAULT CURRENT_TIMESTAMP ) 
--------
CREATE TABLE IF NOT EXISTS testx.timeTypesError ( time_datetime DATETIME BINARY COLLATE latin5_turkish_ci NOT NULL ) 
--------
CREATE TABLE IF NOT EXISTS testx.timeTypesError ( time_year YEAR(2,30) NULL ) 
--------
CREATE TABLE IF NOT EXISTS testx.textTypesError ( text_char CHARACTER BINARY CHARACTER SET latin1 COLLATE non_existing_coll NULL ) 
--------
CREATE TABLE IF NOT EXISTS testx.textTypesError ( text_varchar VARCHAR(32) BINARY CHARACTER SET non_existing_charset COLLATE latin2_general_ci NULL ) 
--------
CREATE TABLE IF NOT EXISTS testx.textTypesError ( text_tinytext TINYTEXT(3) BINARY CHARACTER SET latin5 COLLATE latin5_turkish_ci NULL ) 
--------
CREATE TABLE IF NOT EXISTS testx.textTypesError ( text_text TEXT(10,30) NULL ) 
--------
CREATE TABLE IF NOT EXISTS testx.textTypesError ( text_mediumtext MEDIUMTEXT UNSIGNED CHARACTER SET latin1 COLLATE latin1_german2_ci NULL ) 
--------
CREATE TABLE IF NOT EXISTS testx.dataTypesError ( data_binary BINARY(16,30) NULL ) 
--------
CREATE TABLE IF NOT EXISTS testx.dataTypesError ( data_varbinary VARBINARY COLLATE latin1_german1_ci NULL ) 
--------
CREATE TABLE IF NOT EXISTS testx.dataTypesError ( data_tinyblob TINYBLOB CHARACTER SET latin1 NULL ) 
--------
CREATE TABLE IF NOT EXISTS testx.dataTypesError ( data_blob BLOB UNSIGNED NULL ) 
--------
CREATE TABLE IF NOT EXISTS testx.dataTypesError ( data_mediumblob MEDIUMBLOB BINARY NULL ) 
--------
CREATE TABLE IF NOT EXISTS testx.dataTypesError ( data_longblob LONGBLOB NULL DEFAULT 'abc' ) 
--------
CREATE TABLE IF NOT EXISTS testx.otherTypesError ( rating ENUM('G','PG','PG-13','R','NC-17') NULL DEFAULT 'T' ) 
--------
CREATE TABLE IF NOT EXISTS testx.otherTypesError ( rating ENUM UNSIGNED NULL ) 
--------
CREATE TABLE IF NOT EXISTS testx.otherTypesError ( rating ENUM(1,2,3) NULL DEFAULT 1 ) 
--------
CREATE TABLE IF NOT EXISTS testx.otherTypesError ( special_features SET('Trailers','Commentaries','Deleted Scenes','Behind the Scenes') NULL DEFAULT 'XYZ' ) 
--------
CREATE TABLE IF NOT EXISTS testx.otherTypesError ( special_features SET(5) NULL ) 
--------
CREATE TABLE IF NOT EXISTS testx.otherTypesError ( doc JSON CHARACTER SET utf8 NULL ) 
--------
CREATE TABLE IF NOT EXISTS testx.select_as_test_table ( copied_name TEXT NULL , copied_age INTEGER NULL ) AS SELECT name, age FROM testx.test_table 
--------
CREATE TABLE IF NOT EXISTS testx.select_as_test_collection ( copied_id VARCHAR(32) NOT NULL DEFAULT '0' UNIQUE KEY , copied_doc JSON NULL ) AS SELECT _id, doc FROM testx.test_collection 
--------
CREATE TABLE IF NOT EXISTS testx.select_as_test_view ( copied_name TEXT NULL ) AS SELECT name FROM testx.test_view 
--------
CREATE TABLE IF NOT EXISTS testx.selectAsTestTableError ( copied_name VARCHAR NULL , copied_age REAL NULL ) AS SELECT name, age FROM testx.test_table 
--------
CREATE TABLE IF NOT EXISTS testx.selectAsTestCollectionError ( copied_id VARCHAR(32) NOT NULL DEFAULT '0' UNIQUE KEY , copied_doc JSON NULL ) AS SELECT _id, doc, non_existing_column FROM testx.test_collection 
--------
CREATE TABLE IF NOT EXISTS testx.selectAsTestViewError ( PostalCode INTEGER NULL ) AS SELECT PostalCode FROM testx.test_view 
--------
CREATE TABLE IF NOT EXISTS testx.test_table_clone LIKE testx.test_table 
--------
CREATE TABLE IF NOT EXISTS testx.test_collection_clone LIKE testx.test_collection 
--------
CREATE TABLE IF NOT EXISTS testx.test_view_clone LIKE testx.test_view 
--------
CREATE TABLE IF NOT EXISTS testx.testTableCloneError LIKE non_existing_table 
--------
CREATE TABLE IF NOT EXISTS testx.testCollectionCloneError LIKE testx.test_collection_non_existing 
--------
CREATE TEMPORARY TABLE testx.optionsTable ( row_id SMALLINT UNSIGNED NOT NULL AUTO_INCREMENT PRIMARY KEY ) AUTO_INCREMENT 2017 DEFAULT CHARACTER SET 'utf8' DEFAULT COLLATE 'utf8_general_ci' COMMENT 'this is comment for table options' 
--------
CREATE TABLE testx.optionsTableError ( ) AUTO_INCREMENT -1 
--------
CREATE TEMPORARY TABLE testx.optionsTableError ( row_id SMALLINT UNSIGNED NOT NULL AUTO_INCREMENT PRIMARY KEY ) DEFAULT CHARACTER SET 'unreal-charset' 
--------
CREATE TABLE testx.optionsTableError ( row_id SMALLINT UNSIGNED NOT NULL AUTO_INCREMENT PRIMARY KEY ) DEFAULT COLLATE 'invalid_collation' 
--------
CREATE TABLE IF NOT EXISTS testx.users ( user_id INTEGER NOT NULL AUTO_INCREMENT PRIMARY KEY , username VARCHAR(40) NULL , password VARCHAR(255) NULL , email VARCHAR(255) NULL ) 
--------
CREATE TABLE IF NOT EXISTS testx.roles ( role_id INTEGER NOT NULL AUTO_INCREMENT , role_name VARCHAR(50) NOT NULL , PRIMARY KEY (role_id,role_name) ) 
--------
CREATE TABLE IF NOT EXISTS testx.person ( id SMALLINT UNSIGNED NOT NULL AUTO_INCREMENT , name CHARACTER(60) NOT NULL , PRIMARY KEY (id) ) 
--------
CREATE TABLE IF NOT EXISTS testx.shirt ( id SMALLINT UNSIGNED NOT NULL AUTO_INCREMENT , style ENUM('t-shirt','polo','dress') NOT NULL , color ENUM('red','blue','orange','white','black') NOT NULL , owner SMALLINT UNSIGNED NOT NULL REFERENCES person (id) , PRIMARY KEY (id) ) 
--------
CREATE TABLE IF NOT EXISTS testx.employee ( name VARCHAR(20) NOT NULL COMMENT "employee's name" , surname VARCHAR(20) NOT NULL COMMENT "employee's surname" , PESEL CHARACTER(11) NOT NULL COMMENT "employee's PESEL" , position TINYTEXT NULL COMMENT "employee's Position" , PRIMARY KEY (name,surname,PESEL) ) 
--------
CREATE TABLE IF NOT EXISTS testx.position ( emp_name VARCHAR(20) NULL REFERENCES employee (name) , emp_surname VARCHAR(30) NULL REFERENCES employee (surname) , emp_PESEL CHARACTER(11) NULL REFERENCES employee (PESEL) , description TINYTEXT NULL COMMENT "employee's Position" ) 
--------
CREATE TABLE IF NOT EXISTS testx.car ( VIN CHARACTER(20) NOT NULL PRIMARY KEY , brand VARCHAR(30) NOT NULL , model VARCHAR(50) NOT NULL , UNIQUE INDEX VIN_index (VIN,brand,model) ) 
--------
CREATE TABLE IF NOT EXISTS testx.driverCar ( driver VARCHAR(40) NULL , car_VIN CHARACTER(20) NOT NULL , FOREIGN KEY fk_car_VIN (car_VIN) REFERENCES testx.car (VIN) ON DELETE NO ACTION ON UPDATE CASCADE ) 
--------
CREATE TABLE IF NOT EXISTS testx.usersTableError ( user_id INTEGER NOT NULL AUTO_INCREMENT PRIMARY KEY , username VARCHAR(40) NOT NULL PRIMARY KEY , password VARCHAR(255) NULL ) 
--------
CREATE TABLE IF NOT EXISTS testx.rolesTableError ( role_id INTEGER NOT NULL AUTO_INCREMENT , role_name VARCHAR(50) NULL , PRIMARY KEY (role_id,role_name) ) 
--------
CREATE TABLE IF NOT EXISTS testx.personTableError ( id SMALLINT UNSIGNED NULL , name CHARACTER(60) NOT NULL , PRIMARY KEY (id) ) 
--------
CREATE TABLE IF NOT EXISTS testx.shirtTableError ( id SMALLINT UNSIGNED NOT NULL AUTO_INCREMENT , style ENUM('t-shirt','polo','dress') NOT NULL , color ENUM('red','blue','orange','white','black') NOT NULL , owner SMALLINT UNSIGNED NOT NULL REFERENCES person (name) ) 
--------
CREATE TABLE IF NOT EXISTS testx.employeeTableError ( name VARCHAR(20) NOT NULL COMMENT "employee's name" , surname VARCHAR(20) NOT NULL COMMENT "employee's surname" , PESEL CHARACTER(11) NULL COMMENT "employee's PESEL" , position TINYTEXT NULL COMMENT "employee's Position" , PRIMARY KEY (surname,PESEL,position) ) 
--------
CREATE TABLE IF NOT EXISTS testx.positionTableError ( emp_name SMALLINT(20) NULL REFERENCES non_existing_table (non_existing_name) , emp_surname INTEGER(30) CHARACTER SET utf8 NULL REFERENCES non_existing_table (surname) , emp_PESEL CHARACTER(11) NULL REFERENCES non_existing_table (PESEL) , description TINYTEXT NULL COMMENT "employee's Position" ) 
--------
CREATE TABLE IF NOT EXISTS testx.driverCarTableError ( driver VARCHAR(40) NULL , car_VIN CHARACTER(20) NOT NULL , FOREIGN KEY fk_car_VIN (car_VIN) REFERENCES testx.car (VIN) ON DELETE SET NULL ON UPDATE RESTRICT ) 
--------
CREATE TABLE IF NOT EXISTS testx.citizens ( PersonID INTEGER NOT NULL AUTO_INCREMENT PRIMARY KEY , LastName VARCHAR(40) NULL , FirstName VARCHAR(30) NULL , Address VARCHAR(255) NULL , City VARCHAR(100) NULL , UNIQUE INDEX citizenIndex (PersonID,FirstName,LastName) , INDEX addressIndex (PersonID,Address,City) ) 
--------
CREATE TABLE IF NOT EXISTS testx.cityTable ( PostalCode VARCHAR(10) NOT NULL UNIQUE KEY , Name VARCHAR(100) NULL , Location_longitude DOUBLE NOT NULL , Location_latitude DOUBLE NOT NULL , INDEX cityIndex (PostalCode,Name) ) 
--------
CREATE TABLE IF NOT EXISTS testx.citizensTableError ( LastName BLOB(40) NULL DEFAULT NULL , FirstName VARCHAR(30) NULL DEFAULT 'noname' , UNIQUE INDEX citizenIndex (FirstName,LastName) ) 
--------
CREATE TABLE IF NOT EXISTS testx.citizensTableError ( Address VARCHAR(255) NULL , City VARCHAR(100) NULL , UNIQUE INDEX citizenIndex (City,PostalCode) ) 
--------
CREATE TABLE IF NOT EXISTS testx.citizensTableError ( LastName VARCHAR(40) NULL , FirstName VARCHAR(30) NULL , INDEX addressIndex (FirstName,LastName,Address) ) 
--------
CREATE TABLE IF NOT EXISTS testx.citizensTableError ( LastName VARCHAR(40) NULL , FirstName VARCHAR(30) NULL , INDEX nonExistingFieldIndex (PersonID,FirstName,LastName) ) 
--------
CREATE TABLE IF NOT EXISTS testx.cityTableError ( Name TEXT(100) BINARY NULL DEFAULT 5 UNIQUE KEY ) 
--------
CREATE TABLE IF NOT EXISTS testx.cityTableError ( PostalCode VARCHAR(10) NULL UNIQUE KEY , Name MEDIUMBLOB(100) NULL UNIQUE KEY , Location_longitude DOUBLE NULL UNIQUE KEY , Location_latitude DOUBLE NULL UNIQUE KEY , INDEX cityIndex (PostalCode,Name,Location_longitude) ) 
--------
CREATE TABLE IF NOT EXISTS testx.timestampTable ( id INTEGER NOT NULL AUTO_INCREMENT , data VARCHAR(100) NOT NULL , time TIMESTAMP NULL DEFAULT CURRENT_TIMESTAMP , PRIMARY KEY (id) ) 
--------
CREATE TABLE IF NOT EXISTS testx.timestampTableError ( id INTEGER NOT NULL DEFAULT CURRENT_TIMESTAMP , data VARCHAR(100) NOT NULL , PRIMARY KEY (id) ) 
--------
car
citizens
citytable
datatypes
drivercar
employee
numerictypes
othertypes
person
position
roles
select_as_test_collection
select_as_test_table
select_as_test_view
shirt
test_table
test_table_clone
test_view
texttypes
timestamptable
timetypes
users
done!%A
