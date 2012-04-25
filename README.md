# php-mruby

# Experimental

# Install

````
git submodule init
git submodule update
cd mruby
make
cd lib
cp ritevm.a libritevm.a
cd ../../
phpize
./configure
make
make install
# add `extension=mruby.so` to your php.ini
````

# Usage

````
<?php

$mruby = new MRuby();
$mruby->run('puts "Hello World"');
````

# License

PHP License
