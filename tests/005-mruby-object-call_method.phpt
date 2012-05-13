--TEST--
Check for MrubyObject call method
--SKIPIF--
<?php if (!extension_loaded("mruby")) print "skip"; ?>
--FILE--
<?php
$mrb = new MRuby();
$mrb->run(<<<EOH

class TestCase
  def sayHello
    "Hello"
  end

  def say(name)
    "Hello " + name
  end

  def calc(a, b)
    a + b
  end
end
EOH
);

$tc = $mrb->eval('TestCase.new');

echo $tc->sayHello() . PHP_EOL;
echo $tc->say("Chobie") . PHP_EOL;
echo $tc->calc(1,2) . PHP_EOL;

--EXPECT--
Hello
Hello Chobie
3