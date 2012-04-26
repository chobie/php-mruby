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

void php_mruby_init(TSRMLS_D);
zend_class_entry *mruby_class_entry;

ZEND_BEGIN_ARG_INFO_EX(arginfo_mruby_run, 0, 0, 1)
	ZEND_ARG_INFO(0, code)
ZEND_END_ARG_INFO()

PHP_METHOD(mruby, run)
{
	mrb_state *mrb = mrb_open();
	struct mrb_parser_state *p;
	int n = -1;
	char *code;
	long code_len = 0;

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC,"s",&code, &code_len) == FAILURE) {
		return;
	}


	p = mrb_parse_string(mrb, code);
	n = mrb_generate_code(mrb, p->tree);
	mrb_run(mrb, mrb_proc_new(mrb, mrb->irep[n]), mrb_top_self(mrb));

	if (mrb->exc) {
		mrb_p(mrb, mrb_obj_value(mrb->exc));
	}

}

static zend_function_entry php_mruby_methods[] = {
	PHP_ME(mruby, run, arginfo_mruby_run, ZEND_ACC_PUBLIC)
	{NULL, NULL, NULL}
};

void php_mruby_init(TSRMLS_D)
{
	zend_class_entry ce;

	INIT_CLASS_ENTRY(ce, "MRuby", php_mruby_methods);
	mruby_class_entry = zend_register_internal_class(&ce TSRMLS_CC);
}
