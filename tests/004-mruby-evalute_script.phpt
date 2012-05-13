--TEST--
Check for Mruby::evaluateScript
--SKIPIF--
<?php if (!extension_loaded("mruby")) print "skip"; ?>
--FILE--
<?php
$mruby = new MRuby();

/* string */
echo $mruby->eval('"I" + " am" + " you" + " are" + " me.\n"');

/* integer */
echo $mruby->eval('1 + 2');
echo PHP_EOL;

/* float */
echo $mruby->eval('3.14159265');
echo PHP_EOL;

/* symbol (for now, it'll return as a string)*/
echo $mruby->eval(':name');
echo PHP_EOL;

--EXPECT--
I am you are me.
3
3.14159265
name