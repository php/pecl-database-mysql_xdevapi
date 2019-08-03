--TEST--
orabug #30093227: mysql_xdevapi\CollectionFind::fields() path fails when ends in number
--SKIPIF--
--FILE--
<?php
/*
	bug reported by user
	MYSQL JSON - how do I access nested JSON object that's indexed by numerical key?
	https://bugs.php.net/bug.php?id=78331
	https://stackoverflow.com/questions/57169316/mysql-json-how-do-i-access-nested-json-object-thats-indexed-by-numerical-key
*/
require(__DIR__."/connect.inc");

function fetch_field($field) {
	global $coll;
	$expr = mysql_xdevapi\Expression($field);
	$result = $coll->find('$.pk=25')->fields($expr)->execute();
	echo "----- ", $field, " -----\n";
	var_dump($result->fetchAll());
}

$doc = [
    'pk' => 25,
    'tree' => [
        100 => [
            'qwe' => [
                1
            ],
            'asd' => [
                'a'
            ]
        ],
        '300' => [
            'qwerty' => [
                2
            ],
            'asdf' => [
                'b'
            ]
        ],
        200 => 'a',
        '400.5' => 'x',
        'abc' => 999,
        '-13' => [
            'qq' => [
                13
            ]
		],
        -1313 => [
            'qqryq' => [
                -1212
            ]
		],
        'abc-0.7' => 'xyz',
        '_def0-9' => -6.5,
        '13.14.15' => 16.172,
        '_def0;9' => 'weird id',
        'jkl,11' => 'yet another weird id',
    ]
];

$session = create_test_db();
$schema = $session->getSchema($db);
$coll = $schema->getCollection($test_collection_name);
$coll->add($doc)->execute();

fetch_field('$.tree."100".qwe');
fetch_field('$.tree."100"');
fetch_field('$.tree."200"');
fetch_field('$.tree."300"');
fetch_field('$.tree."400.5"');
fetch_field('$.tree."abc"');
fetch_field('$.tree."-13"');
fetch_field('$.tree."-1313"');
fetch_field('$.tree."abc-0.7"');
fetch_field('$.tree."_def0-9"');
fetch_field('$.tree."13.14.15"');
fetch_field('$.tree."_def0;9"');
fetch_field('$.tree."jkl,11"');

verify_expectations();
print "done!\n";
?>
--CLEAN--
<?php
require(__DIR__."/connect.inc");
clean_test_db();
?>
--EXPECTF--
----- $.tree."100".qwe -----
array(1) {
  [0]=>
  array(1) {
    ["qwe"]=>
    array(1) {
      [0]=>
      int(1)
    }
  }
}
----- $.tree."100" -----
array(1) {
  [0]=>
  array(1) {
    [100]=>
    array(2) {
      ["asd"]=>
      array(1) {
        [0]=>
        string(1) "a"
      }
      ["qwe"]=>
      array(1) {
        [0]=>
        int(1)
      }
    }
  }
}
----- $.tree."200" -----
array(1) {
  [0]=>
  array(1) {
    [200]=>
    string(1) "a"
  }
}
----- $.tree."300" -----
array(1) {
  [0]=>
  array(1) {
    [300]=>
    array(2) {
      ["asdf"]=>
      array(1) {
        [0]=>
        string(1) "b"
      }
      ["qwerty"]=>
      array(1) {
        [0]=>
        int(2)
      }
    }
  }
}
----- $.tree."400.5" -----
array(1) {
  [0]=>
  array(1) {
    ["400.5"]=>
    string(1) "x"
  }
}
----- $.tree."abc" -----
array(1) {
  [0]=>
  array(1) {
    ["abc"]=>
    int(999)
  }
}
----- $.tree."-13" -----
array(1) {
  [0]=>
  array(1) {
    [-13]=>
    array(1) {
      ["qq"]=>
      array(1) {
        [0]=>
        int(13)
      }
    }
  }
}
----- $.tree."-1313" -----
array(1) {
  [0]=>
  array(1) {
    [-1313]=>
    array(1) {
      ["qqryq"]=>
      array(1) {
        [0]=>
        int(-1212)
      }
    }
  }
}
----- $.tree."abc-0.7" -----
array(1) {
  [0]=>
  array(1) {
    ["abc-0.7"]=>
    string(3) "xyz"
  }
}
----- $.tree."_def0-9" -----
array(1) {
  [0]=>
  array(1) {
    ["_def0-9"]=>
    float(-6.5)
  }
}
----- $.tree."13.14.15" -----
array(1) {
  [0]=>
  array(1) {
    ["13.14.15"]=>
    float(16.172)
  }
}
----- $.tree."_def0;9" -----
array(1) {
  [0]=>
  array(1) {
    ["_def0;9"]=>
    string(8) "weird id"
  }
}
----- $.tree."jkl,11" -----
array(1) {
  [0]=>
  array(1) {
    ["jkl,11"]=>
    string(20) "yet another weird id"
  }
}
done!%A
