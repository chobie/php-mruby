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


# Documents

## \Mruby::__construct()

### *Description*

create a mruby instance.

### *Parameters*

### *Return Value*

*Mruby*: mruby instance

### *Example*

````php
<?php
$mrb = new Mruby();
````

## \Mruby::run(string $code)

### *Description*

run ruby code with current instance

### *Parameters*

*code*: ruby code

### *Return Value*

*void*:

### *Example*

````php
<?php
$mrb = new Mruby();
$mrb->run('puts "Hello World"');
````

# Contributors

* Moriyoshi Koizumi
* Shuhei Tanuma