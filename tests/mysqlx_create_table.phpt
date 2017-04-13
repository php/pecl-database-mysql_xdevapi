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
	// init
	// -------------

	$nodeSession = create_test_db();
	fill_db_table();
	create_test_view($nodeSession);
	$schema = $nodeSession->getSchema($db);

	// -------------
	// execution
	// -------------

	function make_table_full_name($tableName) {
		global $db;
		return $db.'.'.$tableName;
	}

	function db_object_exists_in_database($tableName) {
		global $schema;

		$table = $schema->getTable($tableName);
		if ($table->existsInDatabase()) return true;

		$collection = $schema->getCollection($tableName);
		return $collection->existsInDatabase();
	}

	function exec_create_table(
		$createTableCmd, 
		$tableName, 
		$expect_success = true,
		$is_temporary = false) {
		try {
			print $createTableCmd->getSqlQuery().PHP_EOL;
			$createTableCmd->execute();
			expect_true($expect_success && (db_object_exists_in_database($tableName) || $is_temporary));
		} catch (Exception $e) {
			expect_false($expect_success || db_object_exists_in_database($tableName));
		}
		echo "--------", PHP_EOL;
	}

	// -------------
	// columns
	// -------------

	$numericTypes = $schema->createTable('numeric_types')
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
	exec_create_table($numericTypes, 'numeric_types');

	$timeTypes = $schema->createTable('time_types')
		->addColumn((new ColumnDef('time_date', 'DATE')))
		->addColumn((new ColumnDef('time_time', 'TIME')))
		->addColumn((new ColumnDef('time_timestamp', 'TIMESTAMP'))->defaultCurrentTimestamp())
		->addColumn((new ColumnDef('time_datetime', 'DATETIME')))
		->addColumn((new ColumnDef('time_year', 'YEAR')));
	exec_create_table($timeTypes, 'time_types');

	$textTypes = $schema->createTable('text_types')
		->addColumn((new ColumnDef('text_char', 'CHAR'))->binary()->charset('latin1')->collation('latin1_swedish_ci'))
		->addColumn((new ColumnDef('text_varchar', 'VARCHAR', 32))->binary()->charset('latin2')->collation('latin2_general_ci'))
		->addColumn((new ColumnDef('text_tinytext', 'TINYTEXT'))->binary()->charset('latin5')->collation('latin5_turkish_ci'))
		->addColumn((new ColumnDef('text_text', 'TEXT'))->binary()->charset('latin7')->collation('latin7_general_ci'))
		->addColumn((new ColumnDef('text_mediumtext', 'MEDIUMTEXT'))->binary()->charset('latin1')->collation('latin1_german2_ci'))
		->addColumn((new ColumnDef('text_longtext', 'LONGTEXT'))->binary()->charset('latin1')->collation('latin1_german1_ci'));
	exec_create_table($textTypes, 'text_types');

	$dataTypes = $schema->createTable('data_types')
		->addColumn(new ColumnDef('data_binary', 'BINARY', 16))
		->addColumn(new ColumnDef('data_varbinary', 'VARBINARY', 24))
		->addColumn(new ColumnDef('data_tinyblob', 'TINYBLOB'))
		->addColumn(new ColumnDef('data_blob', 'BLOB'))
		->addColumn(new ColumnDef('data_mediumblob', 'MEDIUMBLOB'))
		->addColumn(new ColumnDef('data_longblob', 'LONGBLOB'));
	exec_create_table($dataTypes, 'data_types');

	$otherTypes = $schema->createTable('other_types')
		->addColumn((new ColumnDef('rating', 'Enum'))->values('G', 'PG', 'PG-13', 'R', 'NC-17')->setDefault('G')
			->charset('ucs2')->collation('ucs2_general_ci'))
		->addColumn((new ColumnDef('special_features', 'Set'))
			->values('Trailers', 'Commentaries', 'Deleted Scenes', 'Behind the Scenes' )->setDefault(null)
			->charset('utf8mb4')->collation('utf8mb4_general_ci'))
		->addColumn((new ColumnDef('json_doc', 'json')));
	exec_create_table($otherTypes, 'other_types');

	// -------------
	// columns errors
	// -------------

	$numericTypesError = $schema->createTable('numeric_types_error')
		->addColumn((new ColumnDef('num_bit', 'BIT', 33))->charset('latin1'));
	exec_create_table($numericTypesError, 'numeric_types_error', false);

	$numericTypesError = $schema->createTable('numeric_types_error')
		->addColumn((new ColumnDef('num_tinyint', 'TINYINT', -1))->unsigned());
	exec_create_table($numericTypesError, 'numeric_types_error', false);

	$numericTypesError = $schema->createTable('numeric_types_error')
		->addColumn((new ColumnDef('num_smallint', 'SMALLINT', 16))->unsigned()->setDefault('xyz')->values('x','y','z'));
	exec_create_table($numericTypesError, 'numeric_types_error', false);

	$numericTypesError = $schema->createTable('numeric_types_error')
		->addColumn((new ColumnDef('num_integer', 'INTEGER', 512))->unsigned()->collation('latin2_general_ci'));
	exec_create_table($numericTypesError, 'numeric_types_error', false);

	$numericTypesError = $schema->createTable('numeric_types_error')
		->addColumn((new ColumnDef('num_float', 'FLOAT', 256))->decimals(8)->unsigned());
	exec_create_table($numericTypesError, 'numeric_types_error', false);

	$numericTypesError = $schema->createTable('numeric_types_error')
		->addColumn((new ColumnDef('num_double', 'DOUBLE', 80))->binary()->unsigned());
	exec_create_table($numericTypesError, 'numeric_types_error', false);

	$numericTypesError = $schema->createTable('numeric_types_error')
		->addColumn((new ColumnDef('num_numeric', 'NUMERIC', 32))->charset('latin1')->decimals(30)->unsigned());
	exec_create_table($numericTypesError, 'numeric_types_error', false);

	// -------------

	$timeTypesError = $schema->createTable('time_types_error')
		->addColumn((new ColumnDef('time_date', 'DATE', 5)));
	exec_create_table($timeTypesError, 'time_types_error', false);

	$timeTypesError = $schema->createTable('time_types_error')
		->addColumn((new ColumnDef('time_time', 'TIME'))->unsigned());
	exec_create_table($timeTypesError, 'time_types_error', false);

	$timeTypesError = $schema->createTable('time_types_error')
		->addColumn((new ColumnDef('time_timestamp', 'TIMESTAMP'))->charset('latin5')->defaultCurrentTimestamp());
	exec_create_table($timeTypesError, 'time_types_error', false);

	$timeTypesError = $schema->createTable('time_types_error')
		->addColumn((new ColumnDef('time_datetime', 'DATETIME'))->notNull()
			->binary()->collation('latin5_turkish_ci'));
	exec_create_table($timeTypesError, 'time_types_error', false);

	$timeTypesError = $schema->createTable('time_types_error')
		->addColumn((new ColumnDef('time_year', 'YEAR', 2))->decimals(30));
	exec_create_table($timeTypesError, 'time_types_error', false);

	// -------------

	$textTypesError = $schema->createTable('text_types_error')
		->addColumn((new ColumnDef('text_char', 'CHAR'))->binary()->charset('latin1')->collation('non_existing_coll'));
	exec_create_table($textTypesError, 'text_types_error', false);

	$textTypesError = $schema->createTable('text_types_error')
		->addColumn((new ColumnDef('text_varchar', 'VARCHAR', 32))->binary()->charset('non_existing_charset')->collation('latin2_general_ci'));
	exec_create_table($textTypesError, 'text_types_error', false);

	$textTypesError = $schema->createTable('text_types_error')
		->addColumn((new ColumnDef('text_tinytext', 'TINYTEXT', 3))->binary()->charset('latin5')->collation('latin5_turkish_ci'));
	exec_create_table($textTypesError, 'text_types_error', false);

	$textTypesError = $schema->createTable('text_types_error')
		->addColumn((new ColumnDef('text_text', 'TEXT', 10))->decimals(30));
	exec_create_table($textTypesError, 'text_types_error', false);

	$textTypesError = $schema->createTable('text_types_error')
		->addColumn((new ColumnDef('text_mediumtext', 'MEDIUMTEXT'))->unsigned()->charset('latin1')->collation('latin1_german2_ci'));
	exec_create_table($textTypesError, 'text_types_error', false);

	// -------------

	$dataTypesError = $schema->createTable('data_types_error')
		->addColumn((new ColumnDef('data_binary', 'BINARY', 16))->decimals(30));
	exec_create_table($dataTypesError, 'data_types_error', false);

	$dataTypesError = $schema->createTable('data_types_error')
		->addColumn((new ColumnDef('data_varbinary', 'VARBINARY'))->collation('latin1_german1_ci'));
	exec_create_table($dataTypesError, 'data_types_error', false);

	$dataTypesError = $schema->createTable('data_types_error')
		->addColumn((new ColumnDef('data_tinyblob', 'TINYBLOB'))->charset('latin1'));
	exec_create_table($dataTypesError, 'data_types_error', false);

	$dataTypesError = $schema->createTable('data_types_error')
		->addColumn((new ColumnDef('data_blob', 'BLOB'))->unsigned());
	exec_create_table($dataTypesError, 'data_types_error', false);

	$dataTypesError = $schema->createTable('data_types_error')
		->addColumn((new ColumnDef('data_mediumblob', 'MEDIUMBLOB'))->binary());
	exec_create_table($dataTypesError, 'data_types_error', false);

	$dataTypesError = $schema->createTable('data_types_error')
		->addColumn((new ColumnDef('data_longblob', 'LONGBLOB'))->setDefault('abc'));
	exec_create_table($dataTypesError, 'data_types_error', false);

	// -------------

	$otherTypesError = $schema->createTable('other_types_error')
		->addColumn((new ColumnDef('rating', 'Enum'))->values('G', 'PG', 'PG-13', 'R', 'NC-17')->setDefault('T'));
	exec_create_table($otherTypesError, 'other_types_error', false);

	$otherTypesError = $schema->createTable('other_types_error')
		->addColumn((new ColumnDef('rating', 'Enum'))->unsigned());
	exec_create_table($otherTypesError, 'other_types_error', false);

	$otherTypesError = $schema->createTable('other_types_error')
		->addColumn((new ColumnDef('rating', 'Enum'))->values(1, 2, 3)->setDefault(1));
	exec_create_table($otherTypesError, 'other_types_error', false);

	$otherTypesError = $schema->createTable('other_types_error')
		->addColumn((new ColumnDef('special_features', 'SET'))
			->values('Trailers', 'Commentaries', 'Deleted Scenes', 'Behind the Scenes' )->setDefault('XYZ'));
	exec_create_table($otherTypesError, 'other_types_error', false);

	$otherTypesError = $schema->createTable('other_types_error')
		->addColumn(new ColumnDef('special_features', 'SET', 5));
	exec_create_table($otherTypesError, 'other_types_error', false);

	$otherTypesError = $schema->createTable('other_types_error')
		->addColumn((new ColumnDef('doc', 'json'))->charset('utf8'));
	exec_create_table($otherTypesError, 'other_types_error', false);

	// -------------
	// create table AS
	// -------------

	$selectAsTestTable = $schema->createTable('select_as_'.$test_table_name)
		->addColumn((new ColumnDef('copied_name', 'text')))
		->addColumn((new ColumnDef('copied_age', 'int')))
		->as('SELECT name, age FROM '.make_table_full_name($test_table_name));
	exec_create_table($selectAsTestTable, 'select_as_'.$test_table_name);

	$selectAsTestCollection = $schema->createTable('select_as_'.$test_collection_name)
		->addColumn((new ColumnDef('copied_id', 'VARCHAR', 32))->notNull()->uniqueIndex()->setDefault('0'))
		->addColumn((new ColumnDef('copied_doc', 'json')))
		->as('SELECT _id, doc FROM '.make_table_full_name($test_collection_name));
	exec_create_table($selectAsTestCollection, 'select_as_'.$test_collection_name);

	$selectAsTestView = $schema->createTable('select_as_'.$test_view_name)
		->addColumn((new ColumnDef('copied_name', 'text')))
		->as('SELECT name FROM '.make_table_full_name($test_view_name));
	exec_create_table($selectAsTestView, 'select_as_'.$test_view_name);

	// -------------
	// create table AS errors
	// -------------

	$selectAsTestTableError = $schema->createTable('select_as_test_table_error')
		->addColumn((new ColumnDef('copied_name', 'VARCHAR')))
		->addColumn((new ColumnDef('copied_age', 'real')))
		->as('SELECT name, age FROM '.make_table_full_name($test_table_name));
	exec_create_table($selectAsTestTableError, 'select_as_test_table_error', false);

	$selectAsTestCollectionError = $schema->createTable('select_as_test_collection_error')
		->addColumn((new ColumnDef('copied_id', 'VARCHAR', 32))->notNull()->uniqueIndex()->setDefault('0'))
		->addColumn((new ColumnDef('copied_doc', 'json')))
		->as('SELECT _id, doc, non_existing_column FROM '.make_table_full_name($test_collection_name));
	exec_create_table($selectAsTestCollectionError, 'select_as_test_collection_error', false);

	$selectAsTestViewError = $schema->createTable('select_as_test_view_error')
		->addColumn((new ColumnDef('PostalCode', 'int')))
		->as('SELECT PostalCode FROM '.make_table_full_name($test_view_name));
	exec_create_table($selectAsTestViewError, 'select_as_test_view_error', false);

	// -------------
	// create table LIKE
	// -------------

	$testTableClone = $schema->createTable($test_table_name.'_clone')
		->like(make_table_full_name($test_table_name));
	exec_create_table($testTableClone, $test_table_name.'_clone');

	$testCollectionClone = $schema->createTable($test_collection_name.'_clone')
		->like(make_table_full_name($test_collection_name));
	exec_create_table($testCollectionClone, $test_collection_name.'_clone');

	$testViewClone = $schema->createTable($test_view_name.'_clone')
		->like(make_table_full_name($test_view_name));
	exec_create_table($testViewClone, $test_view_name.'_clone', false);

	// -------------
	// create table LIKE errors
	// -------------

	$testTableCloneError = $schema->createTable('test_table_clone_error')
		->like('non_existing_table');
	exec_create_table($testTableCloneError, 'test_table_clone_error', false);

	$testCollectionCloneError = $schema->createTable('test_collection_clone_error')
		->addColumn((new ColumnDef('id', 'SMALLINT'))->unsigned()->notNull()->autoIncrement())
		->like(make_table_full_name($test_collection_name.'_non_existing'));
	exec_create_table($testCollectionCloneError, 'test_collection_clone_error', false);

	// -------------
	// table options
	// -------------

	$rowIdColumn = new ColumnDef('row_id', 'Smallint');
	$rowIdColumn->primaryKey()->autoIncrement()->unsigned();
	$optionsTable = $schema->createTable('options_table', true)
		->addColumn($rowIdColumn)
		->setInitialAutoIncrement(2017)
		->setDefaultCharset('utf8')
		->setDefaultCollation('utf8_general_ci')
		->setComment('this is comment for table options')
		->temporary();
	exec_create_table($optionsTable, 'options_table', true, true);

	// -------------
	// table options errors
	// -------------

	$optionsTableError = $schema->createTable('options_table_error', true)
		->setInitialAutoIncrement(-1);
	exec_create_table($optionsTableError, 'options_table_error', false);

	$optionsTableError = $schema->createTable('options_table_error', true)
		->addColumn($rowIdColumn)
		->temporary()
		->setDefaultCharset('unreal-charset');
	exec_create_table($optionsTableError, 'options_table_error', false, true);

	$optionsTableError = $schema->createTable('options_table_error', true)
		->addColumn($rowIdColumn)
		->setDefaultCollation('invalid_collation');
	exec_create_table($optionsTableError, 'options_table_error', false);

	// -------------
	// primary key / foreign key
	// -------------

	$usersTable = $schema->createTable('users_table')
		->addColumn((new ColumnDef('user_id', 'INT'))->autoIncrement()->primaryKey())
		->addColumn((new ColumnDef('username', 'VARCHAR', 40)))
		->addColumn((new ColumnDef('password', 'VARCHAR', 255)))
		->addColumn((new ColumnDef('email', 'VARCHAR', 255)));
	exec_create_table($usersTable, 'users_table');

	$rolesTable = $schema->createTable('roles_table')
		->addColumn((new ColumnDef('role_id', 'INT'))->notNull()->autoIncrement())
		->addColumn((new ColumnDef('role_name', 'VARCHAR', 50))->notNull())
		->addPrimaryKey('role_id', 'role_name');
	exec_create_table($rolesTable, 'roles_table');

	// -------------

	$personTable = $schema->createTable('person_table')
		->addColumn((new ColumnDef('id', 'SMALLINT'))->unsigned()->notNull()->autoIncrement())
		->addColumn((new ColumnDef('name', 'CHAR', 60))->notNull())
		->addPrimaryKey('id');
	exec_create_table($personTable, 'person_table');

	$shirtTable = $schema->createTable('shirt_table')
		->addColumn((new ColumnDef('id', 'SMALLINT'))->unsigned()->notNull()->autoIncrement())
		->addColumn((new ColumnDef('style', 'ENUM'))->values('t-shirt', 'polo', 'dress')->notNull())
		->addColumn((new ColumnDef('color', 'ENUM'))->values('red', 'blue', 'orange', 'white', 'black')->notNull())
		->addColumn((new ColumnDef('owner', 'SMALLINT'))->unsigned()->notNull()
			->foreignKey(make_table_full_name('person_table'), 'id'))
		->addPrimaryKey('id');
	exec_create_table($shirtTable, 'shirt_table');

	// -------------

	$employeeTable = $schema->createTable('employee_table')
		->addColumn((new ColumnDef('name', 'VARCHAR', 20))->notNull()->comment('employee\'s name'))
		->addColumn((new ColumnDef('surname', 'VARCHAR', 20))->notNull()->comment('employee\'s surname'))
		->addColumn((new ColumnDef('PESEL', 'CHAR', 11))->notNull()->comment('employee\'s PESEL'))
		->addColumn((new ColumnDef('position', 'TinyText'))->comment('employee\'s Position'))
		->addPrimaryKey('name', 'surname', 'PESEL');
	exec_create_table($employeeTable, 'employee_table');

	$positionTable = $schema->createTable('position_table')
		->addColumn((new ColumnDef('emp_name', 'VARCHAR', 20))
			->foreignKey(make_table_full_name('employee_table'), 'name'))
		->addColumn((new ColumnDef('emp_surname', 'STRING', 30))
			->foreignKey(make_table_full_name('employee_table'), 'surname'))
		->addColumn((new ColumnDef('emp_PESEL', 'CHAR', 11))
			->foreignKey(make_table_full_name('employee_table'), 'PESEL'))
		->addColumn((new ColumnDef('description', 'TinyText'))->comment('employee\'s Position'));
	exec_create_table($positionTable, 'position_table');

	// -------------

	$carTable = $schema->createTable('car_table')
		->addColumn((new ColumnDef('VIN', 'CHAR', 20))->notNull()->primaryKey())
		->addColumn((new ColumnDef('brand', 'VARCHAR', 30))->notNull())
		->addColumn((new ColumnDef('model', 'VARCHAR', 50))->notNull())
		->addUniqueIndex('VIN_index', 'VIN', 'brand', 'model');
	exec_create_table($carTable, 'car_table');

	$fk_driver_car_table = new ForeignKeyDef();
	$fk_driver_car_table->fields('car_VIN')
		->refersTo(make_table_full_name('car_table'), 'VIN')
		->onDelete('no action')->onUpdate('Cascade');

	$driverCarTable = $schema->createTable('driver_car_table')
		->addColumn((new ColumnDef('driver', 'VARCHAR', 40)))
		->addColumn((new ColumnDef('car_VIN', 'CHAR', 20))->notNull())
		->addForeignKey('fk_car_VIN', $fk_driver_car_table);
	exec_create_table($driverCarTable, 'driver_car_table');

	// -------------
	// primary key / foreign key errors
	// -------------

	$usersTableError = $schema->createTable('users_table_error')
		->addColumn((new ColumnDef('user_id', 'INT'))->autoIncrement()->primaryKey())
		->addColumn((new ColumnDef('username', 'VARCHAR', 40))->notNull()->primaryKey())
		->addColumn((new ColumnDef('password', 'VARCHAR', 255)));
	exec_create_table($usersTableError, 'users_table_error', false);

	$rolesTableError = $schema->createTable('roles_table_error')
		->addColumn((new ColumnDef('role_id', 'INT'))->notNull()->autoIncrement())
		->addColumn((new ColumnDef('role_name', 'VARCHAR', 50)))
		->addPrimaryKey('role_id', 'role_name');
	exec_create_table($rolesTableError, 'roles_table_error', false);

	// -------------

	$personTableError = $schema->createTable('person_table_error')
		->addColumn((new ColumnDef('id', 'SMALLINT'))->unsigned())
		->addColumn((new ColumnDef('name', 'CHAR', 60))->notNull())
		->addPrimaryKey('id');
	exec_create_table($personTableError, 'person_table_error', false);

	$shirtTableError = $schema->createTable('shirt_table_error')
		->addColumn((new ColumnDef('id', 'SMALLINT'))->unsigned()->notNull()->autoIncrement())
		->addColumn((new ColumnDef('style', 'ENUM'))->values('t-shirt', 'polo', 'dress')->notNull())
		->addColumn((new ColumnDef('color', 'ENUM'))->values('red', 'blue', 'orange', 'white', 'black')->notNull())
		->addColumn((new ColumnDef('owner', 'SMALLINT'))->unsigned()->notNull()
			->foreignKey(make_table_full_name('person_table'), 'name'));
	exec_create_table($shirtTableError, 'shirt_table_error', false);

	// -------------

	$employeeTableError = $schema->createTable('employee_table_error')
		->addColumn((new ColumnDef('name', 'VARCHAR', 20))->notNull()->comment('employee\'s name'))
		->addColumn((new ColumnDef('surname', 'VARCHAR', 20))->notNull()->comment('employee\'s surname'))
		->addColumn((new ColumnDef('PESEL', 'CHAR', 11))->comment('employee\'s PESEL'))
		->addColumn((new ColumnDef('position', 'TinyText'))->comment('employee\'s Position'))
		->addPrimaryKey('surname', 'PESEL', 'position');
	exec_create_table($employeeTableError, 'employee_table_error', false);

	$positionTableError = $schema->createTable('position_table_error')
		->addColumn((new ColumnDef('emp_name', 'SMALLINT', 20))->foreignKey('non_existing_table', 'non_existing_name'))
		->addColumn((new ColumnDef('emp_surname', 'int', 30))->charset('utf8')->foreignKey('non_existing_table', 'surname'))
		->addColumn((new ColumnDef('emp_PESEL', 'CHAR', 11))->foreignKey('non_existing_table', 'PESEL'))
		->addColumn((new ColumnDef('description', 'TinyText'))->comment('employee\'s Position'));
	exec_create_table($positionTableError, 'position_table_error', false);

	// -------------

	$driverCarTableError = $schema->createTable('driver_car_table_error')
		->addColumn((new ColumnDef('driver', 'VARCHAR', 40)))
		->addColumn((new ColumnDef('car_VIN', 'CHAR', 20))->notNull())
		->addForeignKey('fk_car_VIN',
			(new ForeignKeyDef())->fields('car_VIN')->refersTo(make_table_full_name('car_table'), 'VIN')
				->onDelete('Set Null')->onUpdate('Restrict'));
	exec_create_table($driverCarTableError, 'driver_car_table_error', false);

	// -------------
	// indexes
	// -------------

	$citizensTable = $schema->createTable('citizens_table')
		->addColumn((new ColumnDef('PersonID', 'int'))->autoIncrement()->primaryKey())
		->addColumn((new ColumnDef('LastName', 'varchar', 40)))
		->addColumn((new ColumnDef('FirstName', 'varchar', 30)))
		->addColumn((new ColumnDef('Address', 'varchar', 255)))
		->addColumn((new ColumnDef('City', 'varchar', 100)))
		->addUniqueIndex('citizenIndex', 'PersonID', 'FirstName', 'LastName')
		->addIndex('addressIndex', 'PersonID', 'Address', 'City');
	exec_create_table($citizensTable, 'citizens_table');

	$cityTable = $schema->createTable('city_table')
		->addColumn((new ColumnDef('PostalCode', 'varchar', 10))->notNull()->uniqueIndex())
		->addColumn((new ColumnDef('Name', 'varchar', 100)))
		->addColumn((new ColumnDef('Location_longitude', 'double'))->notNull())
		->addColumn((new ColumnDef('Location_latitude', 'double'))->notNull())
		->addIndex('cityIndex', 'PostalCode', 'Name');
	exec_create_table($cityTable, 'city_table');

	// -------------
	// indexes errors
	// -------------

	$citizensTableError = $schema->createTable('citizens_table_error')
		->addColumn((new ColumnDef('LastName', 'blob', 40))->setDefault(null))
		->addColumn((new ColumnDef('FirstName', 'varchar', 30))->setDefault('noname'))
		->addUniqueIndex('citizenIndex', 'FirstName', 'LastName');
	exec_create_table($citizensTableError, 'citizens_table_error', false);

	$citizensTableError = $schema->createTable('citizens_table_error')
		->addColumn((new ColumnDef('Address', 'varchar', 255)))
		->addColumn((new ColumnDef('City', 'varchar', 100)))
		->addUniqueIndex('citizenIndex', 'City', 'PostalCode');
	exec_create_table($citizensTableError, 'citizens_table_error', false);

	$citizensTableError = $schema->createTable('citizens_table_error')
		->addColumn((new ColumnDef('LastName', 'varchar', 40)))
		->addColumn((new ColumnDef('FirstName', 'varchar', 30)))
		->addIndex('addressIndex', 'FirstName', 'LastName', 'Address');
	exec_create_table($citizensTableError, 'citizens_table_error', false);

	$citizensTableError = $schema->createTable('citizens_table_error')
		->addColumn((new ColumnDef('LastName', 'varchar', 40)))
		->addColumn((new ColumnDef('FirstName', 'varchar', 30)))
		->addIndex('nonExistingFieldIndex', 'PersonID', 'FirstName', 'LastName');
	exec_create_table($citizensTableError, 'citizens_table_error', false);


	$cityTableError = $schema->createTable('city_table_error')
		->addColumn((new ColumnDef('Name', 'text', 100))->binary()->setDefault(5)->uniqueIndex());
	exec_create_table($cityTableError, 'city_table_error', false);

	$cityTableError = $schema->createTable('city_table_error')
		->addColumn((new ColumnDef('PostalCode', 'varchar', 10))->uniqueIndex())
		->addColumn((new ColumnDef('Name', 'mediumblob', 100))->uniqueIndex())
		->addColumn((new ColumnDef('Location_longitude', 'double'))->uniqueIndex())
		->addColumn((new ColumnDef('Location_latitude', 'double'))->uniqueIndex())
		->addIndex('cityIndex', 'PostalCode', 'Name', 'Location_longitude');
	exec_create_table($cityTableError, 'city_table_error', false);

	// -------------
	// time defaults
	// -------------

	$timestampTable = $schema->createTable('timestamp_table')
		->addColumn((new ColumnDef('id', 'INT'))->notNull()->autoIncrement())
		->addColumn((new ColumnDef('data', 'VARCHAR', 100))->notNull())
		->addColumn((new ColumnDef('time', 'TIMESTAMP'))->defaultCurrentTimestamp())
		->addPrimaryKey('id');
	exec_create_table($timestampTable, 'timestamp_table');

	$timestampTableError = $schema->createTable('timestamp_table_error')
		->addColumn((new ColumnDef('id', 'INT'))->notNull()->defaultCurrentTimestamp())
		->addColumn((new ColumnDef('data', 'VARCHAR', 100))->notNull())
		->addPrimaryKey('id');
	exec_create_table($timestampTableError, 'timestamp_table_error', false);

	// -------------

	$all_tables = array_keys($schema->getTables());
	sort($all_tables);
	foreach ($all_tables as $tableName) {
		echo "{$tableName}".PHP_EOL;
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
CREATE TABLE IF NOT EXISTS testx.numeric_types ( num_bit BIT(2) NULL , num_tinyint TINYINT(8) UNSIGNED NULL , num_smallint SMALLINT(16) UNSIGNED NULL , num_mediumint MEDIUMINT(32) UNSIGNED NULL , num_int INTEGER(32) UNSIGNED NULL , num_integer INTEGER(32) UNSIGNED NULL , num_bigint BIGINT(64) UNSIGNED NULL , num_real REAL(64,10) UNSIGNED NULL , num_double DOUBLE(80,16) UNSIGNED NULL , num_float FLOAT(32,8) UNSIGNED NULL , num_decimal DECIMAL(24,20) UNSIGNED NULL , num_numeric NUMERIC(32,30) UNSIGNED NULL ) 
--------
CREATE TABLE IF NOT EXISTS testx.time_types ( time_date DATE NULL , time_time TIME NULL , time_timestamp TIMESTAMP NULL DEFAULT CURRENT_TIMESTAMP , time_datetime DATETIME NULL , time_year YEAR NULL ) 
--------
CREATE TABLE IF NOT EXISTS testx.text_types ( text_char CHARACTER BINARY CHARACTER SET latin1 COLLATE latin1_swedish_ci NULL , text_varchar VARCHAR(32) BINARY CHARACTER SET latin2 COLLATE latin2_general_ci NULL , text_tinytext TINYTEXT BINARY CHARACTER SET latin5 COLLATE latin5_turkish_ci NULL , text_text TEXT BINARY CHARACTER SET latin7 COLLATE latin7_general_ci NULL , text_mediumtext MEDIUMTEXT BINARY CHARACTER SET latin1 COLLATE latin1_german2_ci NULL , text_longtext LONGTEXT BINARY CHARACTER SET latin1 COLLATE latin1_german1_ci NULL ) 
--------
CREATE TABLE IF NOT EXISTS testx.data_types ( data_binary BINARY(16) NULL , data_varbinary VARBINARY(24) NULL , data_tinyblob TINYBLOB NULL , data_blob BLOB NULL , data_mediumblob MEDIUMBLOB NULL , data_longblob LONGBLOB NULL ) 
--------
CREATE TABLE IF NOT EXISTS testx.other_types ( rating ENUM('G','PG','PG-13','R','NC-17') CHARACTER SET ucs2 COLLATE ucs2_general_ci NULL DEFAULT 'G' , special_features SET('Trailers','Commentaries','Deleted Scenes','Behind the Scenes') CHARACTER SET utf8mb4 COLLATE utf8mb4_general_ci NULL DEFAULT NULL , json_doc JSON NULL ) 
--------
CREATE TABLE IF NOT EXISTS testx.numeric_types_error ( num_bit BIT(33) CHARACTER SET latin1 NULL ) 
--------
CREATE TABLE IF NOT EXISTS testx.numeric_types_error ( num_tinyint TINYINT(-1) UNSIGNED NULL ) 
--------
CREATE TABLE IF NOT EXISTS testx.numeric_types_error ( num_smallint SMALLINT(16) UNSIGNED NULL DEFAULT 'xyz' ) 
--------
CREATE TABLE IF NOT EXISTS testx.numeric_types_error ( num_integer INTEGER(512) UNSIGNED COLLATE latin2_general_ci NULL ) 
--------
CREATE TABLE IF NOT EXISTS testx.numeric_types_error ( num_float FLOAT(256,8) UNSIGNED NULL ) 
--------
CREATE TABLE IF NOT EXISTS testx.numeric_types_error ( num_double DOUBLE(80) UNSIGNED BINARY NULL ) 
--------
CREATE TABLE IF NOT EXISTS testx.numeric_types_error ( num_numeric NUMERIC(32,30) UNSIGNED CHARACTER SET latin1 NULL ) 
--------
CREATE TABLE IF NOT EXISTS testx.time_types_error ( time_date DATE(5) NULL ) 
--------
CREATE TABLE IF NOT EXISTS testx.time_types_error ( time_time TIME UNSIGNED NULL ) 
--------
CREATE TABLE IF NOT EXISTS testx.time_types_error ( time_timestamp TIMESTAMP CHARACTER SET latin5 NULL DEFAULT CURRENT_TIMESTAMP ) 
--------
CREATE TABLE IF NOT EXISTS testx.time_types_error ( time_datetime DATETIME BINARY COLLATE latin5_turkish_ci NOT NULL ) 
--------
CREATE TABLE IF NOT EXISTS testx.time_types_error ( time_year YEAR(2,30) NULL ) 
--------
CREATE TABLE IF NOT EXISTS testx.text_types_error ( text_char CHARACTER BINARY CHARACTER SET latin1 COLLATE non_existing_coll NULL ) 
--------
CREATE TABLE IF NOT EXISTS testx.text_types_error ( text_varchar VARCHAR(32) BINARY CHARACTER SET non_existing_charset COLLATE latin2_general_ci NULL ) 
--------
CREATE TABLE IF NOT EXISTS testx.text_types_error ( text_tinytext TINYTEXT(3) BINARY CHARACTER SET latin5 COLLATE latin5_turkish_ci NULL ) 
--------
CREATE TABLE IF NOT EXISTS testx.text_types_error ( text_text TEXT(10,30) NULL ) 
--------
CREATE TABLE IF NOT EXISTS testx.text_types_error ( text_mediumtext MEDIUMTEXT UNSIGNED CHARACTER SET latin1 COLLATE latin1_german2_ci NULL ) 
--------
CREATE TABLE IF NOT EXISTS testx.data_types_error ( data_binary BINARY(16,30) NULL ) 
--------
CREATE TABLE IF NOT EXISTS testx.data_types_error ( data_varbinary VARBINARY COLLATE latin1_german1_ci NULL ) 
--------
CREATE TABLE IF NOT EXISTS testx.data_types_error ( data_tinyblob TINYBLOB CHARACTER SET latin1 NULL ) 
--------
CREATE TABLE IF NOT EXISTS testx.data_types_error ( data_blob BLOB UNSIGNED NULL ) 
--------
CREATE TABLE IF NOT EXISTS testx.data_types_error ( data_mediumblob MEDIUMBLOB BINARY NULL ) 
--------
CREATE TABLE IF NOT EXISTS testx.data_types_error ( data_longblob LONGBLOB NULL DEFAULT 'abc' ) 
--------
CREATE TABLE IF NOT EXISTS testx.other_types_error ( rating ENUM('G','PG','PG-13','R','NC-17') NULL DEFAULT 'T' ) 
--------
CREATE TABLE IF NOT EXISTS testx.other_types_error ( rating ENUM UNSIGNED NULL ) 
--------
CREATE TABLE IF NOT EXISTS testx.other_types_error ( rating ENUM(1,2,3) NULL DEFAULT 1 ) 
--------
CREATE TABLE IF NOT EXISTS testx.other_types_error ( special_features SET('Trailers','Commentaries','Deleted Scenes','Behind the Scenes') NULL DEFAULT 'XYZ' ) 
--------
CREATE TABLE IF NOT EXISTS testx.other_types_error ( special_features SET(5) NULL ) 
--------
CREATE TABLE IF NOT EXISTS testx.other_types_error ( doc JSON CHARACTER SET utf8 NULL ) 
--------
CREATE TABLE IF NOT EXISTS testx.select_as_test_table ( copied_name TEXT NULL , copied_age INTEGER NULL ) AS SELECT name, age FROM testx.test_table 
--------
CREATE TABLE IF NOT EXISTS testx.select_as_test_collection ( copied_id VARCHAR(32) NOT NULL DEFAULT '0' UNIQUE KEY , copied_doc JSON NULL ) AS SELECT _id, doc FROM testx.test_collection 
--------
CREATE TABLE IF NOT EXISTS testx.select_as_test_view ( copied_name TEXT NULL ) AS SELECT name FROM testx.test_view 
--------
CREATE TABLE IF NOT EXISTS testx.select_as_test_table_error ( copied_name VARCHAR NULL , copied_age REAL NULL ) AS SELECT name, age FROM testx.test_table 
--------
CREATE TABLE IF NOT EXISTS testx.select_as_test_collection_error ( copied_id VARCHAR(32) NOT NULL DEFAULT '0' UNIQUE KEY , copied_doc JSON NULL ) AS SELECT _id, doc, non_existing_column FROM testx.test_collection 
--------
CREATE TABLE IF NOT EXISTS testx.select_as_test_view_error ( PostalCode INTEGER NULL ) AS SELECT PostalCode FROM testx.test_view 
--------
CREATE TABLE IF NOT EXISTS testx.test_table_clone LIKE testx.test_table 
--------
CREATE TABLE IF NOT EXISTS testx.test_collection_clone LIKE testx.test_collection 
--------
CREATE TABLE IF NOT EXISTS testx.test_view_clone LIKE testx.test_view 
--------
CREATE TABLE IF NOT EXISTS testx.test_table_clone_error LIKE non_existing_table 
--------
CREATE TABLE IF NOT EXISTS testx.test_collection_clone_error LIKE testx.test_collection_non_existing 
--------
CREATE TEMPORARY TABLE testx.options_table ( row_id SMALLINT UNSIGNED NOT NULL AUTO_INCREMENT PRIMARY KEY ) AUTO_INCREMENT 2017 DEFAULT CHARACTER SET 'utf8' DEFAULT COLLATE 'utf8_general_ci' COMMENT 'this is comment for table options' 
--------
CREATE TABLE testx.options_table_error ( ) AUTO_INCREMENT -1 
--------
CREATE TEMPORARY TABLE testx.options_table_error ( row_id SMALLINT UNSIGNED NOT NULL AUTO_INCREMENT PRIMARY KEY ) DEFAULT CHARACTER SET 'unreal-charset' 
--------
CREATE TABLE testx.options_table_error ( row_id SMALLINT UNSIGNED NOT NULL AUTO_INCREMENT PRIMARY KEY ) DEFAULT COLLATE 'invalid_collation' 
--------
CREATE TABLE IF NOT EXISTS testx.users_table ( user_id INTEGER NOT NULL AUTO_INCREMENT PRIMARY KEY , username VARCHAR(40) NULL , password VARCHAR(255) NULL , email VARCHAR(255) NULL ) 
--------
CREATE TABLE IF NOT EXISTS testx.roles_table ( role_id INTEGER NOT NULL AUTO_INCREMENT , role_name VARCHAR(50) NOT NULL , PRIMARY KEY (role_id,role_name) ) 
--------
CREATE TABLE IF NOT EXISTS testx.person_table ( id SMALLINT UNSIGNED NOT NULL AUTO_INCREMENT , name CHARACTER(60) NOT NULL , PRIMARY KEY (id) ) 
--------
CREATE TABLE IF NOT EXISTS testx.shirt_table ( id SMALLINT UNSIGNED NOT NULL AUTO_INCREMENT , style ENUM('t-shirt','polo','dress') NOT NULL , color ENUM('red','blue','orange','white','black') NOT NULL , owner SMALLINT UNSIGNED NOT NULL REFERENCES testx.person_table (id) , PRIMARY KEY (id) ) 
--------
CREATE TABLE IF NOT EXISTS testx.employee_table ( name VARCHAR(20) NOT NULL COMMENT "employee's name" , surname VARCHAR(20) NOT NULL COMMENT "employee's surname" , PESEL CHARACTER(11) NOT NULL COMMENT "employee's PESEL" , position TINYTEXT NULL COMMENT "employee's Position" , PRIMARY KEY (name,surname,PESEL) ) 
--------
CREATE TABLE IF NOT EXISTS testx.position_table ( emp_name VARCHAR(20) NULL REFERENCES testx.employee_table (name) , emp_surname VARCHAR(30) NULL REFERENCES testx.employee_table (surname) , emp_PESEL CHARACTER(11) NULL REFERENCES testx.employee_table (PESEL) , description TINYTEXT NULL COMMENT "employee's Position" ) 
--------
CREATE TABLE IF NOT EXISTS testx.car_table ( VIN CHARACTER(20) NOT NULL PRIMARY KEY , brand VARCHAR(30) NOT NULL , model VARCHAR(50) NOT NULL , UNIQUE INDEX VIN_index (VIN,brand,model) ) 
--------
CREATE TABLE IF NOT EXISTS testx.driver_car_table ( driver VARCHAR(40) NULL , car_VIN CHARACTER(20) NOT NULL , FOREIGN KEY fk_car_VIN (car_VIN) REFERENCES testx.car_table (VIN) ON DELETE NO ACTION ON UPDATE CASCADE ) 
--------
CREATE TABLE IF NOT EXISTS testx.users_table_error ( user_id INTEGER NOT NULL AUTO_INCREMENT PRIMARY KEY , username VARCHAR(40) NOT NULL PRIMARY KEY , password VARCHAR(255) NULL ) 
--------
CREATE TABLE IF NOT EXISTS testx.roles_table_error ( role_id INTEGER NOT NULL AUTO_INCREMENT , role_name VARCHAR(50) NULL , PRIMARY KEY (role_id,role_name) ) 
--------
CREATE TABLE IF NOT EXISTS testx.person_table_error ( id SMALLINT UNSIGNED NULL , name CHARACTER(60) NOT NULL , PRIMARY KEY (id) ) 
--------
CREATE TABLE IF NOT EXISTS testx.shirt_table_error ( id SMALLINT UNSIGNED NOT NULL AUTO_INCREMENT , style ENUM('t-shirt','polo','dress') NOT NULL , color ENUM('red','blue','orange','white','black') NOT NULL , owner SMALLINT UNSIGNED NOT NULL REFERENCES testx.person_table (name) ) 
--------
CREATE TABLE IF NOT EXISTS testx.employee_table_error ( name VARCHAR(20) NOT NULL COMMENT "employee's name" , surname VARCHAR(20) NOT NULL COMMENT "employee's surname" , PESEL CHARACTER(11) NULL COMMENT "employee's PESEL" , position TINYTEXT NULL COMMENT "employee's Position" , PRIMARY KEY (surname,PESEL,position) ) 
--------
CREATE TABLE IF NOT EXISTS testx.position_table_error ( emp_name SMALLINT(20) NULL REFERENCES non_existing_table (non_existing_name) , emp_surname INTEGER(30) CHARACTER SET utf8 NULL REFERENCES non_existing_table (surname) , emp_PESEL CHARACTER(11) NULL REFERENCES non_existing_table (PESEL) , description TINYTEXT NULL COMMENT "employee's Position" ) 
--------
CREATE TABLE IF NOT EXISTS testx.driver_car_table_error ( driver VARCHAR(40) NULL , car_VIN CHARACTER(20) NOT NULL , FOREIGN KEY fk_car_VIN (car_VIN) REFERENCES testx.car_table (VIN) ON DELETE SET NULL ON UPDATE RESTRICT ) 
--------
CREATE TABLE IF NOT EXISTS testx.citizens_table ( PersonID INTEGER NOT NULL AUTO_INCREMENT PRIMARY KEY , LastName VARCHAR(40) NULL , FirstName VARCHAR(30) NULL , Address VARCHAR(255) NULL , City VARCHAR(100) NULL , UNIQUE INDEX citizenIndex (PersonID,FirstName,LastName) , INDEX addressIndex (PersonID,Address,City) ) 
--------
CREATE TABLE IF NOT EXISTS testx.city_table ( PostalCode VARCHAR(10) NOT NULL UNIQUE KEY , Name VARCHAR(100) NULL , Location_longitude DOUBLE NOT NULL , Location_latitude DOUBLE NOT NULL , INDEX cityIndex (PostalCode,Name) ) 
--------
CREATE TABLE IF NOT EXISTS testx.citizens_table_error ( LastName BLOB(40) NULL DEFAULT NULL , FirstName VARCHAR(30) NULL DEFAULT 'noname' , UNIQUE INDEX citizenIndex (FirstName,LastName) ) 
--------
CREATE TABLE IF NOT EXISTS testx.citizens_table_error ( Address VARCHAR(255) NULL , City VARCHAR(100) NULL , UNIQUE INDEX citizenIndex (City,PostalCode) ) 
--------
CREATE TABLE IF NOT EXISTS testx.citizens_table_error ( LastName VARCHAR(40) NULL , FirstName VARCHAR(30) NULL , INDEX addressIndex (FirstName,LastName,Address) ) 
--------
CREATE TABLE IF NOT EXISTS testx.citizens_table_error ( LastName VARCHAR(40) NULL , FirstName VARCHAR(30) NULL , INDEX nonExistingFieldIndex (PersonID,FirstName,LastName) ) 
--------
CREATE TABLE IF NOT EXISTS testx.city_table_error ( Name TEXT(100) BINARY NULL DEFAULT 5 UNIQUE KEY ) 
--------
CREATE TABLE IF NOT EXISTS testx.city_table_error ( PostalCode VARCHAR(10) NULL UNIQUE KEY , Name MEDIUMBLOB(100) NULL UNIQUE KEY , Location_longitude DOUBLE NULL UNIQUE KEY , Location_latitude DOUBLE NULL UNIQUE KEY , INDEX cityIndex (PostalCode,Name,Location_longitude) ) 
--------
CREATE TABLE IF NOT EXISTS testx.timestamp_table ( id INTEGER NOT NULL AUTO_INCREMENT , data VARCHAR(100) NOT NULL , time TIMESTAMP NULL DEFAULT CURRENT_TIMESTAMP , PRIMARY KEY (id) ) 
--------
CREATE TABLE IF NOT EXISTS testx.timestamp_table_error ( id INTEGER NOT NULL DEFAULT CURRENT_TIMESTAMP , data VARCHAR(100) NOT NULL , PRIMARY KEY (id) ) 
--------
car_table
citizens_table
city_table
data_types
driver_car_table
employee_table
numeric_types
other_types
person_table
position_table
roles_table
select_as_test_collection
select_as_test_table
select_as_test_view
shirt_table
test_table
test_table_clone
test_view
text_types
time_types
timestamp_table
users_table
done!%A
