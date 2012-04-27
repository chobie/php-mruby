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
#include "mruby/string.h"

void php_mruby_init(TSRMLS_D);
zend_class_entry *mruby_class_entry;

int php_mruby_convert_mrb_value(zval **result, mrb_state *mrb, mrb_value argv TSRMLS_DC)
{
	int retval = SUCCESS;
	zval *tmp;
	MAKE_STD_ZVAL(tmp);
	
	switch (mrb_type(argv)) {
		case MRB_TT_STRING: {
			struct RString *str;
			str = mrb_str_ptr(argv);
			ZVAL_STRING(tmp, str->buf,1);
			break;
		}
		case MRB_TT_FIXNUM: {
			ZVAL_LONG(tmp, (long)argv.value.i);
			break;
		}
		case MRB_TT_FLOAT: {
			ZVAL_DOUBLE(tmp, (double)argv.value.f);
			break;
		}
		default: {
			fprintf(stderr,"php_mruby_convert_mrb_value does not support %s\n",mrb_obj_classname(mrb,argv));
			zval_ptr_dtor(&tmp);
			retval = FAILURE;
			break;
		}
	}
	
	if (retval == SUCCESS) {
		*result = tmp;
	}

	return retval;
}

static mrb_value phplib_echo(mrb_state *mrb, mrb_value self)
{
	TSRMLS_FETCH();
	
	mrb_value argv;
	mrb_get_args(mrb, "o", &argv);

	/* should we don't convert mrb_value to zval? */
	switch (mrb_type(argv)) {
		case MRB_TT_FLOAT:
		case MRB_TT_FIXNUM:
		case MRB_TT_STRING:
		{
			zval *tmp;
			if (php_mruby_convert_mrb_value(&tmp, mrb, argv TSRMLS_CC) == SUCCESS) {
				if (Z_TYPE_P(tmp) != IS_STRING) {
					convert_to_string(tmp);
				}
				
				php_printf("%s",Z_STRVAL_P(tmp));
				zval_ptr_dtor(&tmp);
			}
			break;
		}
		default: {
			fprintf(stderr,"PHP::echo does not support %s\n",mrb_obj_classname(mrb,argv));
			break;
		}
	}

	return argv;
}

static void phplib_initialize(mrb_state *mrb)
{
	struct RClass *phplib;

	phplib = mrb_define_module(mrb, "PHP");
	mrb_define_class_method(mrb,phplib,"echo",phplib_echo,ARGS_REQ(1));
}



static void php_mruby_free_storage(php_mruby_t *obj TSRMLS_DC)
{
	zend_object_std_dtor(&obj->zo TSRMLS_CC);
	if (obj->mrb != NULL) {
		mrb_close(obj->mrb);
		obj->mrb = NULL;
	}
	efree(obj);
}

zend_object_value php_mruby_new(zend_class_entry *ce TSRMLS_DC)
{
	zend_object_value retval;
	php_mruby_t *obj;
	zval *tmp;

	obj = ecalloc(1, sizeof(*obj));
	zend_object_std_init( &obj->zo, ce TSRMLS_CC );
#if ZEND_MODULE_API_NO >= 20100525
	object_properties_init(&(obj->zo), ce); 
#else
	zend_hash_copy(obj->zo.properties, &ce->default_properties, (copy_ctor_func_t) zval_add_ref, (void *) &tmp, sizeof(zval *));
#endif

	obj->mrb = mrb_open();
	phplib_initialize(obj->mrb);

	retval.handle = zend_objects_store_put(obj, 
		(zend_objects_store_dtor_t)zend_objects_destroy_object,
		(zend_objects_free_object_storage_t)php_mruby_free_storage,
		NULL TSRMLS_CC);
	retval.handlers = zend_get_std_object_handlers();
	return retval;
}


ZEND_BEGIN_ARG_INFO_EX(arginfo_mruby_run, 0, 0, 1)
	ZEND_ARG_INFO(0, code)
ZEND_END_ARG_INFO()


PHP_METHOD(mruby, run)
{
	mrb_state *mrb;
	struct mrb_parser_state *p;
	php_mruby_t *object = NULL;
	int n = -1;
	char *code;
	long code_len = 0;
	
	object = (php_mruby_t *) zend_object_store_get_object(getThis() TSRMLS_CC);
	mrb = object->mrb;

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
	mruby_class_entry->create_object = php_mruby_new;
}
