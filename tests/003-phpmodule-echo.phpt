--TEST--
Check for PHP::echo
--SKIPIF--
<?php if (!extension_loaded("mruby")) print "skip"; ?>
--FILE--
<?php
$mruby = new MRuby();

$code = <<<EOH
require 'php'

PHP::echo "Hello World"
PHP::echo "\n"
PHP::echo 1234
PHP::echo "\n"
PHP::echo 1.2
PHP::echo "\n"
EOH;

$mruby->run($code);
--EXPECT--
Hello World
1234
1.2
