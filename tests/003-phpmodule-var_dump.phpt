--TEST--
Check for PHP::var_dump
--SKIPIF--
<?php if (!extension_loaded("mruby")) print "skip"; ?>
--FILE--
<?php
$mruby = new MRuby();

$code = <<<EOH
require 'php'

PHP::var_dump "Hello World"
PHP::var_dump 1234
PHP::var_dump 1.2
PHP::var_dump [1,2,3,4]
hash = {"a" => "b","c" => "d"}
PHP::var_dump hash
EOH;

$mruby->run($code);
--EXPECT--
string(11) "Hello World"
int(1234)
float(1.2)
array(4) {
  [0]=>
  int(1)
  [1]=>
  int(2)
  [2]=>
  int(3)
  [3]=>
  int(4)
}
array(2) {
  ["c"]=>
  string(1) "d"
  ["a"]=>
  string(1) "b"
}