# php-mruby

php-mruby is just simple wrapper of <https://github.com/mruby/mruby>.

[![Build Status](https://secure.travis-ci.org/chobie/php-mruby.png)](http://travis-ci.org/chobie/php-mruby)


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
