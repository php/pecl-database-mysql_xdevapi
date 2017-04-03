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
		->addColumn((new ColumnDef('num_numeric', 'NUMERIC', 32))->decimals(30)->unsigned())
		->execute();

	$timeTypes = $schema->createTable('timeTypes')
		->addColumn((new ColumnDef('time_date', 'DATE')))
		->addColumn((new ColumnDef('time_time', 'TIME')))
		->addColumn((new ColumnDef('time_timestamp', 'TIMESTAMP'))->defaultCurrentTimestamp())
		->addColumn((new ColumnDef('time_datetime', 'DATETIME')))
		->addColumn((new ColumnDef('time_year', 'YEAR')))
		->execute();

	$textTypes = $schema->createTable('textTypes')
		->addColumn((new ColumnDef('text_char', 'CHAR'))->binary()->charset('latin1')->collation('latin1_swedish_ci'))
		->addColumn((new ColumnDef('text_varchar', 'VARCHAR', 32))->binary()->charset('latin2')->collation('latin2_general_ci'))
		->addColumn((new ColumnDef('text_tinytext', 'TINYTEXT'))->binary()->charset('latin5')->collation('latin5_turkish_ci'))
		->addColumn((new ColumnDef('text_text', 'TEXT'))->binary()->charset('latin7')->collation('latin7_general_ci'))
		->addColumn((new ColumnDef('text_mediumtext', 'MEDIUMTEXT'))->binary()->charset('latin1')->collation('latin1_german2_ci'))
		->addColumn((new ColumnDef('text_longtext', 'LONGTEXT'))->binary()->charset('latin1')->collation('latin1_german1_ci'))
		->execute();

	$dataTypes = $schema->createTable('dataTypes')
		->addColumn((new ColumnDef('data_binary', 'BINARY', 16)))
		->addColumn((new ColumnDef('data_varbinary', 'VARBINARY', 24)))
		->addColumn((new ColumnDef('data_tinyblob', 'TINYBLOB')))
		->addColumn((new ColumnDef('data_blob', 'BLOB')))
		->addColumn((new ColumnDef('data_mediumblob', 'MEDIUMBLOB')))
		->addColumn((new ColumnDef('data_longblob', 'LONGBLOB')))
		->execute();

	// -------------
	// create table AS
	// -------------

	$selectAsTestTable = $schema->createTable('select_as_'.$test_table_name)
		->addColumn((new ColumnDef('copied_name', 'text')))
		->addColumn((new ColumnDef('copied_age', 'int')))
		->as('SELECT name, age FROM '.$db.'.'.$test_table_name)
		->execute();

	$selectAsTestCollection = $schema->createTable('select_as_'.$test_collection_name)
		->addColumn((new ColumnDef('copied_id', 'VARCHAR', 32))->notNull()->uniqueIndex()->setDefault('0'))
		->addColumn((new ColumnDef('copied_doc', 'json')))
		->as('SELECT _id, doc FROM '.$db.'.'.$test_collection_name)
		->execute();

	$selectAsTestView = $schema->createTable('select_as_'.$test_view_name)
		->addColumn((new ColumnDef('copied_name', 'text')))
		->as('SELECT name FROM '.$db.'.'.$test_view_name)
		->execute();

	// -------------
	// create table LIKE
	// -------------

	$testTableClone = $schema->createTable($test_table_name.'_clone')
		->like($db.'.'.$test_table_name)
		->execute();

	$testCollectionClone = $schema->createTable($test_collection_name.'_clone')
		->like($db.'.'.$test_collection_name)
		->execute();

	try {
		$testViewClone = $schema->createTable($test_view_name.'_clone')
			->like($db.'.'.$test_view_name)
			->execute();
		test_step_failed();
	} catch (Exception $e) {
		test_step_ok();
	}

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
		->temporary()
		->execute();

	// -------------
	// primary key / foreign key
	// -------------

	$usersTable = $schema->createTable('users')
		->addColumn((new ColumnDef('user_id', 'INT'))->autoIncrement()->primaryKey())
		->addColumn((new ColumnDef('username', 'VARCHAR', 40)))
		->addColumn((new ColumnDef('password', 'VARCHAR', 255)))
		->addColumn((new ColumnDef('email', 'VARCHAR', 255)))
		->execute();

	$rolesTable = $schema->createTable('roles')
		->addColumn((new ColumnDef('role_id', 'INT'))->notNull()->autoIncrement())
		->addColumn((new ColumnDef('role_name', 'VARCHAR', 50))->notNull())
		->addPrimaryKey('role_id', 'role_name')
		->execute();

	// -------------

	$personTable = $schema->createTable('person')
		->addColumn((new ColumnDef('id', 'SMALLINT'))->unsigned()->notNull()->autoIncrement())
		->addColumn((new ColumnDef('name', 'CHAR', 60))->notNull())
		->addPrimaryKey('id')
		->execute();

	$shirtTable = $schema->createTable('shirt')
		->addColumn((new ColumnDef('id', 'SMALLINT'))->unsigned()->notNull()->autoIncrement())
		->addColumn((new ColumnDef('style', 'ENUM'))->values('t-shirt', 'polo', 'dress')->notNull())
		->addColumn((new ColumnDef('color', 'ENUM'))->values('red', 'blue', 'orange', 'white', 'black')->notNull())
		->addColumn((new ColumnDef('owner', 'SMALLINT'))->unsigned()->notNull()->foreignKey('person', 'id'))
		->addPrimaryKey('id')
		->execute();

	// -------------

	$employeeTable = $schema->createTable('employee')
		->addColumn((new ColumnDef('name', 'VARCHAR', 20))->notNull()->comment('employee\'s name'))
		->addColumn((new ColumnDef('surname', 'VARCHAR', 20))->notNull()->comment('employee\'s surname'))
		->addColumn((new ColumnDef('PESEL', 'CHAR', 11))->notNull()->comment('employee\'s PESEL'))
		->addColumn((new ColumnDef('position', 'TinyText'))->comment('employee\'s Position'))
		->addPrimaryKey('name', 'surname', 'PESEL')
		->execute();

	$positionTable = $schema->createTable('position')
		->addColumn((new ColumnDef('emp_name', 'VARCHAR', 20))->foreignKey('employee', 'name'))
		->addColumn((new ColumnDef('emp_surname', 'STRING', 30))->foreignKey('employee', 'surname'))
		->addColumn((new ColumnDef('emp_PESEL', 'CHAR', 11))->foreignKey('employee', 'PESEL'))
		->addColumn((new ColumnDef('description', 'TinyText'))->comment('employee\'s Position'))
		->execute();

	// -------------

	$carTable = $schema->createTable('car')
		->addColumn((new ColumnDef('VIN', 'CHAR', 20))->notNull()->primaryKey())
		->addColumn((new ColumnDef('brand', 'VARCHAR', 30))->notNull())
		->addColumn((new ColumnDef('model', 'VARCHAR', 50))->notNull())
		->addUniqueIndex('VIN_index', 'VIN', 'brand', 'model')
		->execute();

	$driverCarTable = $schema->createTable('driverCar')
		->addColumn((new ColumnDef('driver', 'VARCHAR', 40)))
		->addColumn((new ColumnDef('car_VIN', 'CHAR', 20))->notNull())
		->addForeignKey('fk_car_VIN', 
			(new ForeignKeyDef())->fields('car_VIN')->refersTo($db.'.'.'car', 'VIN')
				->onDelete('no action')->onUpdate('Cascade'))
		->execute();

	// -------------
	// indexes
	// -------------

	$personsTable = $schema->createTable('persons')
		->addColumn((new ColumnDef('PersonID', 'int'))->autoIncrement()->primaryKey())
		->addColumn((new ColumnDef('LastName', 'varchar', 40)))
		->addColumn((new ColumnDef('FirstName', 'varchar', 30)))
		->addColumn((new ColumnDef('Address', 'varchar', 255)))
		->addColumn((new ColumnDef('City', 'varchar', 100)))
		->addUniqueIndex('personIndex', 'PersonID', 'FirstName', 'LastName')
		->addIndex('addressIndex', 'PersonID', 'Address', 'City')
		->execute();

	$cityTable = $schema->createTable('cityTable')
		->addColumn((new ColumnDef('PostalCode', 'varchar', 10))->notNull()->uniqueIndex())
		->addColumn((new ColumnDef('Name', 'varchar', 100)))
		->addColumn((new ColumnDef('Location_longitude', 'double'))->notNull())
		->addColumn((new ColumnDef('Location_latitude', 'double'))->notNull())
		->addIndex('cityIndex', 'PostalCode', 'Name')
		->execute();

	// -------------
	// time defaults
	// -------------

	$timestampTable = $schema->createTable('timestamp')
		->addColumn((new ColumnDef('id', 'INT'))->notNull()->autoIncrement())
		->addColumn((new ColumnDef('data', 'VARCHAR', 100))->notNull())
		->addColumn((new ColumnDef('time', 'TIMESTAMP'))->defaultCurrentTimestamp())
		->addPrimaryKey('id')
		->execute();

	// -------------

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
