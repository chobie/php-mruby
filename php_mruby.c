/*
   +----------------------------------------------------------------------+
   | PHP Version 5                                                        |
   +----------------------------------------------------------------------+
   | Copyright (c) 1997-2011 The PHP Group                                |
   +----------------------------------------------------------------------+
   | This source file is subject to version 3.01 of the PHP license,      |
   | that is bundled with this package in the file LICENSE, and is        |
   | available through the world-wide-web at the following url:           |
   | http://www.php.net/license/3_01.txt                                  |
   | If you did not receive a copy of the PHP license and are unable to   |
   | obtain it through the world-wide-web, please send a note to          |
   | license@php.net so we can mail you a copy immediately.               |
   +----------------------------------------------------------------------+
   | Authors: Shuhei Tanuma <chobieee@gmail.com>                          |
   +----------------------------------------------------------------------+
 */


#include "php_mruby.h"

extern void php_mruby_init(TSRMLS_D);
extern zend_class_entry *mruby_class_entry;

void php_mruby_init(TSRMLS_D);


PHP_MINIT_FUNCTION(mruby) {
	php_mruby_init(TSRMLS_C);

	return SUCCESS;
}


PHP_MINFO_FUNCTION(mruby)
{
	php_printf("PHP Mruby Extension\n");
}

zend_module_entry mruby_module_entry = {
#if ZEND_MODULE_API_NO >= 20010901
	STANDARD_MODULE_HEADER,
#endif
	"mruby",
	NULL,					/* Functions */
	PHP_MINIT(mruby),	/* MINIT */
	NULL,					/* MSHUTDOWN */
	NULL,					/* RINIT */
	NULL,					/* RSHUTDOWN */
	PHP_MINFO(mruby),	/* MINFO */
#if ZEND_MODULE_API_NO >= 20010901
	PHP_MRUBY_EXTVER,
#endif
	STANDARD_MODULE_PROPERTIES
};


#ifdef COMPILE_DL_MRUBY
ZEND_GET_MODULE(mruby)
#endif
