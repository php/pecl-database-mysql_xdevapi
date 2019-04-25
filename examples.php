<?php
   /*
                With X DevAPI we can connect directly to a specific session by a call to getSession
		or invoke getSession to connect to one or *more* servers.

		Details: https://dev.mysql.com/doc/x-devapi-userguide/en/xsession-vs-node-session.html

                For using the X DevAPI a MySQL Server with enabled X Plugin is required.
		The X Plugin is part of MySQL 5.7.12. For instructions about installation and configuration
		please refer to the MySQL documentation at:

                http://dev.mysql.com/doc/refman/5.7/en/document-store-setting-up.html

		The XDevAPI extension for PHP export the functions used to initiate the connection,
		the syntax is: mysql_xdevapi\<name of the function>

		Where mysql_xdevapi is the 'namespace' where those functions are declared.

		If the connection failed the function will raise an exception (invalid user name or password, &c)
    */

    $session = mysql_xdevapi\getSession("localhost","root","");
    var_dump($session);

    /*
		Once we have a connection established, we might want to create a new
		schema for the purpose of storing doc's.

		We can do this by calling createSchema from the current session.

		On server side is possible to verify the existence of the schema by running
		'show databases;', on my machine this will display:

		mysql> show databases;
		+--------------------+
		| Database           |
		+--------------------+
		| information_schema |
		| mysql              |
		| performance_schema |
		| products           |
		| sys                |
		+--------------------+

		The 'products' schema exist.
    */

    $schema = $session->createSchema("products");

    /*
		Now we need create a new collection in order to store docs,
		this can be performed by a call to createCollection, the argument
		provided is the name of the new collection which will be stored in "products"
    */

    $collection = $schema->createCollection("best_products");

    /*
		Till now I've created a schema and a collection, but those might have
		been already in the database, to obtain an already existing schema
		and colletion the user can make use of getSchema and getCollection:
    */

    $schema = $session->getSchema("products");
    $collection = $schema->getCollection("best_products");


    /*
		But a document store make no sense if we don't store... documents.

		For the sake of this sample, I've created a file with some product information
		which I want to load in the document store. Clearly the data might be coming
		from any other source.
    */

    $products = file("product_data.txt",
		     FILE_IGNORE_NEW_LINES);
    foreach ($products as $key => &$product) {
	$arr = explode("\t", $product);
	$product = array('code' => $arr[0],
			 'name' => $arr[1],
			 'desc' => $arr[2],
			 'price' => floatval($arr[3]),
			 'aval' => intval($arr[4]));

	/*
		Is possible to add documents to the store in multiple ways, in this sample
		I've an array which represent a collection of products which I want add
		to the collection.

		A simple call to the function 'add' with the array as argument is sufficient,
		is worth noticing that in order to execute the 'add' operation an explicit call
		to 'execute' is required, this is true for most of the operations we're going
		to perform in this sample.
	*/

	$collection->add($product)->execute();
    }

    /*
            Is possible to add documents to the collection by providing JSON strings:
    */

    $collection->add('{"code": 44, "aval": 3, "desc": "A pen!", "name": "SuperPen", "price": 3.22}')->execute();
    $collection->add('{"code": 434, "aval": 43, "desc": "A pencil!", "name": "SuperPencil", "price": 2.22}')->execute();

    /*
		Now that we have some documents in our collection, let's lookup
		the database for the three most expensive products in our catalogue.

		For this purpose I'm calling the function 'find' without arguments,
		(we look in all the data available), then I chain a call to 'sort'
		in order to have the data sorted by the price in ascending order,
		followed by 'limit' call, which set the amount of data I want to retrieve.

		This call returns a 'DocResult' object, from which I can fetch the
		retrieved rows:
			1) fetchAll, will fetch all the rows
			2) fetchOne, will fetch just one row
    */

    $expensive = $collection->find()->sort("price desc")->limit(3)->execute();
    $exp_data = $expensive->fetchAll();

    /*
	The call to print_r will output the three most expensive items:

Array
(
    [0] => Array
	(
	    [doc] => {"_id": "df768423a59c11e69f47bcee7b785c27", "aval": 6, "code": "ABABA", "desc": "Available in two colors!", "name": "PHP Expert", "price": 2499.99}
	)

    [1] => Array
	(
	    [doc] => {"_id": "df74ea6fa59c11e69f47bcee7b785c27", "aval": 3, "code": "51A51", "desc": "Your favorite plastic surgeon", "name": "PlasticFace", "price": 1499.99}
	)

    [2] => Array
	(
	    [doc] => {"_id": "df74ea81a59c11e69f47bcee7b785c27", "aval": 2, "code": "56411", "desc": "The best of the best!", "name": "Programmer2", "price": 299.99}
	)

)

    */

    print_r($exp_data);

    /*
		I want to change the amount of available units of the product "SuperDent".
		now I have +10 produts in my stock, to perform this operation I need
		to modify the document, for this purpose I'll upload the new value with a
		'set' call from 'modify'.
    */

    $collection->modify("name like 'SuperDent'")->set("aval", 15)->execute();

    /*
		Is possible to use the binding feature instead of hardcoding
		the product name, this code is equivalen (in terms of result)
		to the previous instruction:
    */
    $productName = "SuperDent";
    $collection->modify("name like :pr_name")->bind(["pr_name" => $productName])->set("aval", 15)->execute();

    /*
		I might want to add a new attribute in one or more of my documents,
		for example all the items for which I have less than 10 products available
		shall now have a new "comment" attribute.

		Adding attributes change only the selected documents in the collection
		and does not effect the other.
    */
    $message = "Stock too low, order more items.";
    $collection->modify("aval < 10")->set("comment",$message)->execute();

    /*
		Again, parameter binding is allowed, is possible to provide
		the value directly to 'bind' instead to pointing to a variable.
    */

    $collection->modify("aval < :amount")->bind(["amount" => 10])->set("comment", $message)->execute();

    /*
		Some products are not available anymore and I want to remove
		the related documents from my products collection,

		I can perform this operation by a call to 'remove'. Let's say I
		do not have anymore "Programmers" in my catalog.
		In the DB's I have three type* of programmers and I want to remove them all:

		*Programmer1,Programmer2,Programmer3
    */

    $collection->remove("name like 'Programmer%'")->execute(); //Note the '%'

    /*
                To remove the comments for some items before one
		can make use again of the 'modify' operation, this time
		unsetting the values.

		The actual implementation of 'unset' expect as argument
		an array of names, where each name refers to an attribute to remove.
		(That's why the strange syntax ["comment"]).
    */

    $collection->modify("comment like '%'")->unset(["comment"])->execute();

    /*
                The extension allows you to run SQL query from the script
		with minimum effort, in order to do that a session object
		is required.

                Let's use SQL to create two tables for our new products:
    */

    $session = mysql_xdevapi\getSession("localhost", "root", "");
    $session->sql("create table products.new_products_table(name text,price float,description text)")->execute();
    $session->sql("create table products.new_cheap_products_table(name text,price float,description text)")->execute();

    /*
                Is possible to extract an array containing all the tables
		in the database.

                The output of the following instructions is:
		-> new_cheap_products_table
		-> new_products_table
    */

    $tables = $schema->getTables();
    print "There are ".count($tables)." tables in the DB:".PHP_EOL;
    foreach( $tables as $entry ) {
        print " -> ".$entry->getName().PHP_EOL;
    }


    /*
                And easily new elements can be inserted into the tables:
    */

    $new_prod_tbl = $tables["new_products_table"];
    $new_prod_tbl->insert(["name", "price", "description"])->values(["bPhone","605.15","The best bPhone from Banana Corporation"])->execute();
    $new_prod_tbl->insert(["name", "price", "description"])->values(["bPhone ultraplus","933.1","Even bigger bPhone from Banana Corporation"])->execute();
    $new_prod_tbl->insert(["name", "price", "description"])->values(["bPad","345.33","Completely useless gadget from Banana Corporation"])->execute();


    /*
                Then is possible to perform a selection the table in order
		to retrieve data.

                The output of the following select operation is:
		-> bPhone ultraplus
		-> bPhone
    */

    $result = $new_prod_tbl->select(["name"])->where("price > 500")->orderBy("price desc")->execute();
    foreach( $result->fetchAll() as $item ) {
        print " -> ".$item["name"].PHP_EOL;
    }

    /*
                Removing elements is straightforward, a call
		to delete followed by the deleting criteria is
		all that is needed.

                After the following instruction all that remains
		in the new_products_table is the bPad product.
    */

    $new_prod_tbl->delete()->where("price > 500")->execute();

    /*
		Is possible to drop the schema with a call to dropSchema,
		providing the name of the schema do drop.
    */

    $session->dropSchema("products");

    /*
		All the operations which I've listed here are executed
		(operation request transmitted to the server) after a call
		to 'execute', all the chaining we're doing before execute
		is not effecting the collection.

		All the changes once executes are reflected on server side,
		and is possible to query the DB's to make a verification
    */
?>
