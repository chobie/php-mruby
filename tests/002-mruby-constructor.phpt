--TEST--
Check for Mruby::__construct 
--SKIPIF--
<?php if (!extension_loaded("mruby")) print "skip"; ?>
--FILE--
<?php
$mruby = new MRuby();
/* don't leak */
$mruby = new MRuby();
if ($mruby instanceof MRuby) {
  echo "OK";
} else {
  echo "FAIL";
}
--EXPECT--
OK
