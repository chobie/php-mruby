--TEST--
Check for PHP::call_user_func
--SKIPIF--
<?php if (!extension_loaded("mruby")) print "skip"; ?>
--FILE--
<?php
$mruby = new MRuby();

function hello()
{
    echo "Hello World" . PHP_EOL;
}

function say_hello_to($name)
{
    echo "Hello " . $name . PHP_EOL;
}

function say($name,$message)
{
    echo "Hi " . $name . ", " . $message . PHP_EOL;
}


function dump_array($array)
{
    var_dump($array);
}


$code = <<<EOH
require 'php'

PHP::call_user_func "hello"
PHP::call_user_func "say_hello_to", "Joe"
PHP::call_user_func "say", "Joe", "how do you do?"
PHP::call_user_func "dump_array", [1,2,3,4]
EOH;

$mruby->run($code);
--EXPECT--
Hello World
Hello Joe
Hi Joe, how do you do?
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