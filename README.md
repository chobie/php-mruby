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
cd ../
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

## \Mruby::assign(string $key, string $value)

### *Description*

assign global variable to mruby.

### *Parameters*

*key*: variable name. it need '$' prefix.
*value*: value. currently, only support string.

### *Return Value*

*void*:

### *Example*

````php
<?php
$mrb = new Mruby();
$mrb->assign('$myname','chobie');
$mrb->run('puts $myname');
````


## \Mruby::run(string $code[, array $arguments])

### *Description*

run ruby code with current instance

### *Parameters*

*code*: ruby code
*arguments*: arguments. it can use with ARGV constant.

### *Return Value*

*void*:

### *Example*

````php
<?php
$mrb = new Mruby();
$mrb->run('puts "Hello World"');
````


## \Mruby::eval(string $source[, array $arguments])

### *Description*

evaluate ruby source.

### *Parameters*

*source*: ruby source
*arguments*: arguments. it can use with ARGV constant.

### *Return Value*

*mixed*: converted mruby value.

### *Example*

````php
<?php
$mrb = new Mruby();
foreach($mrb->eval("ARGV.map{|v| v+1}",[1,2,3]) as $k => $v) {
  echo $v . PHP_EOL // should be displayed 2,3,4.
}
````

### PHP mruby modules (experimental)

now, you can import PHP module in your mruby instance!

````
require 'php'

PHP::echo string
  echo string with php

    PHP::echo "Hello World"

PHP::var_dump args...
  the var_dump

    PHP::var_dump 1, 2, 3, "Hello", [4,5,6]


PHP::call_user_func func_name args
  call php function.

    PHP::echo PHP::call_user_func "base64_encode", "Hello world"

PHP::_REQUEST
  returns converted $_REQUEST hash
    
    PHP::_REQUEST.each {| k,v | PHP::echo k + " " + v }

PHP::_SERVER
  returns converted $_SERVER hash
    
    PHP::_SERVER.each {| k,v | PHP::echo k + " " + v }
````

# Contributors

* Moriyoshi Koizumi
* Shuhei Tanuma