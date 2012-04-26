--TEST--
Check for mruby presence
--SKIPIF--
<?php if (!extension_loaded("mruby")) print "skip"; ?>
--FILE--
<?php
echo "mruby extension is available";
--EXPECT--
mruby extension is available
