--TEST--
Check for Mruby::run 
--SKIPIF--
<?php if (!extension_loaded("mruby")) print "skip"; ?>
--FILE--
<?php
$mruby = new MRuby();

$code = <<<EOH
def hello_world
  puts "Hello World"
end

hello_world
EOH;

$mruby->run($code);
--EXPECT--
Hello World
