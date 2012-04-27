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
#include "mruby/khash.h"
#include "mruby/hash.h"
#include "mruby/array.h"

void php_mruby_init(TSRMLS_D);
zend_class_entry *mruby_class_entry;

/* are we need this here? */
KHASH_INIT(ht, mrb_value, mrb_value, 1, mrb_hash_ht_hash_func, mrb_hash_ht_hash_equal);

static int php_mruby_call_user_function_v(HashTable *function_table, zval **object_pp, zval *function_name, zval *retval_ptr, zend_uint param_count, ...)
{
	va_list ap;
	size_t i;
	int ret;
	zval **params;
	zval *tmp;
	TSRMLS_FETCH();

	if (param_count > 0) {
		params = emalloc(sizeof(zval**) * param_count);
		va_start(ap, param_count);
		for (i=0; i<param_count;i++) {
			params[i] = va_arg(ap, zval*);
		}
		va_end(ap);
	} else {
		params = NULL;
	}

	ret = call_user_function(function_table, object_pp, function_name, retval_ptr, param_count,params TSRMLS_CC);

	if (param_count > 0) {
		for (i=0; i<param_count;i++) {
			if (params[i] != NULL) {
				zval_ptr_dtor(&params[i]);
			}
		}
		efree(params);
	}
	return ret;
}


/** serializes array to mrb_value */
mrb_value php_mruby_serialize_array(mrb_state *mrb, zval *value TSRMLS_DC)
{
	HashTable *h;
	HashPosition pos;
	size_t n;
	zval **d;
	mrb_value tmp;
	
	char *key;
	uint key_len;
	int key_type;
	ulong key_index;
	
	h = Z_ARRVAL_P(value);
	n = zend_hash_num_elements(h);

	tmp = mrb_hash_new(mrb,n);
	
	zend_hash_internal_pointer_reset_ex(h, &pos);
	for (;; zend_hash_move_forward_ex(h, &pos)) {
		if (key_type == HASH_KEY_NON_EXISTANT) {
			break;
		}
		
		zval *t;
		key_type = zend_hash_get_current_key_ex(h, &key, &key_len, &key_index, 0, &pos);
		zend_hash_get_current_data_ex(h, (void *) &d, &pos);
		
		/* todo: also serializes other types */
		if (Z_TYPE_PP(d) == IS_STRING) {
			if (Z_STRLEN_PP(d) == 0) {
				mrb_hash_set(mrb, tmp, mrb_str_new_cstr(mrb, key), mrb_str_new_cstr(mrb, ""));
			} else {
				mrb_hash_set(mrb, tmp, mrb_str_new_cstr(mrb, key), mrb_str_new_cstr(mrb, Z_STRVAL_PP(d)));
			}
		}
	}

	return tmp;
}

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
		case MRB_TT_ARRAY: {
			struct RArray *array_ptr;
			HashTable *hash;
			char key[25];
			int i = 0;
			int length = RARRAY_LEN(argv);

			array_ptr = mrb_ary_ptr(argv);
			array_init(tmp);
			hash = Z_ARRVAL_P(tmp);
			//zend_hash_init(hash, length, NULL, ZVAL_PTR_DTOR, 0);
			
			for (i = 0; i < length; i++) {
				zval *z_val;
				php_mruby_convert_mrb_value(&z_val,mrb, array_ptr->buf[i] TSRMLS_CC);

				//sprintf(key,"%d",i);
				//zend_hash_add(hash,key, strlen(key)+1,(void **)&z_val,sizeof(z_val),NULL);
				add_next_index_zval(tmp,z_val);
			}
			
			Z_ARRVAL_P(tmp) = hash;
			break;
		}
		case MRB_TT_HASH: {
			struct kh_ht *h = RHASH_H_TBL(argv);
			khiter_t k;
			HashTable *hash;
			int length = RHASH_SIZE(argv);

			array_init(tmp);
			hash = Z_ARRVAL_P(tmp);

			for (k = kh_begin(h); k != kh_end(h); k++) {
				mrb_value hash_key,hash_value;
				zval *z_hash_key, *z_hash_val;

				if (!kh_exist(h, k)) {
					continue;
				}
				if(php_mruby_convert_mrb_value(&z_hash_key,mrb, kh_key(h,k) TSRMLS_CC) == FAILURE) {
					/* for now. */
					continue;
				}
				
				php_mruby_convert_mrb_value(&z_hash_val,mrb, kh_value(h,k) TSRMLS_CC);
				
				if (Z_TYPE_P(z_hash_key) != IS_STRING) {
					convert_to_string(z_hash_key);
				}
				
				zend_hash_add(hash,Z_STRVAL_P(z_hash_key), Z_STRLEN_P(z_hash_key)+1,(void **)&z_hash_val,sizeof(z_hash_val),NULL);
				zval_ptr_dtor(&z_hash_key);
			}
			
			Z_ARRVAL_P(tmp) = hash;
			break;
		}
		default: {
			if (strcmp("NilClass",mrb_obj_classname(mrb,argv)) == 0) {
				zval_ptr_dtor(&tmp);
				ZVAL_NULL(tmp);
			} else {
				fprintf(stderr,"php_mruby_convert_mrb_value does not support %s\n",mrb_obj_classname(mrb,argv));
				zval_ptr_dtor(&tmp);
				retval = FAILURE;
			}
			break;
		}
	}
	
	if (retval == SUCCESS) {
		*result = tmp;
	}

	return retval;
}


static mrb_value phplib_get_global_zval(mrb_state *mrb, char *name, size_t length)
{
	TSRMLS_FETCH();
	mrb_value result;
	zval **data;
	
	if (PG(auto_globals_jit)) {
		zend_is_auto_global(name, length-1 TSRMLS_CC);
	}
	if (zend_hash_find(&EG(symbol_table),name,length,(void **) &data) == SUCCESS) {
		result = php_mruby_serialize_array(mrb, *data TSRMLS_CC);
	}

	return result;
}

static mrb_value phplib_eg_request(mrb_state *mrb, mrb_value self)
{
	return phplib_get_global_zval(mrb, "_REQUEST", sizeof("_REQUEST"));
}


static mrb_value phplib_eg_server(mrb_state *mrb, mrb_value self)
{
	return phplib_get_global_zval(mrb, "_SERVER", sizeof("_SERVER"));
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

static mrb_value phplib_call_user_func(mrb_state *mrb, mrb_value self)
{
	TSRMLS_FETCH();
	
	zval *z_func_name, *result, **params;
	mrb_value *argv, rb_result;
	int ret, i, param_count, argc = 0;
	mrb_get_args(mrb, "*", &argv, &argc);

	if (argc > 1) {
		int offset = 0;
		param_count = argc-1;
		params = emalloc(sizeof(zval**) * param_count);
		for (offset = 0, i=1; i<argc;i++,offset++) {
			zval *tmp;
			
			php_mruby_convert_mrb_value(&tmp, mrb, argv[i] TSRMLS_CC);
			params[offset] = tmp;
		}
	} else {
		params = NULL;
	}

	MAKE_STD_ZVAL(result);
	php_mruby_convert_mrb_value(&z_func_name, mrb, argv[0] TSRMLS_CC);
	ret = call_user_function(&EG(symbol_table), NULL, z_func_name, result, param_count, params TSRMLS_CC);

	if (param_count > 0) {
		for (i=0; i<param_count;i++) {
			if (params[i] != NULL) {
				zval_ptr_dtor(&params[i]);
			}
		}
		efree(params);
	}
	
	if (Z_TYPE_P(result) == IS_STRING) {
		rb_result = mrb_str_new_cstr(mrb, Z_STRVAL_P(result));
	}

	zval_ptr_dtor(&z_func_name);
	zval_ptr_dtor(&result);
	
	return rb_result;
}

static void phplib_initialize(mrb_state *mrb)
{
	struct RClass *phplib;

	phplib = mrb_define_module(mrb, "PHP");
	mrb_define_class_method(mrb,phplib,"echo",phplib_echo,ARGS_REQ(1));
	mrb_define_class_method(mrb,phplib,"call_user_func",phplib_call_user_func,ARGS_ANY());
	mrb_define_class_method(mrb,phplib,"_SERVER",phplib_eg_server,ARGS_REQ(1));
	mrb_define_class_method(mrb,phplib,"_REQUEST",phplib_eg_request,ARGS_REQ(1));
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


ZEND_BEGIN_ARG_INFO_EX(arginfo_mruby_assign, 0, 0, 1)
	ZEND_ARG_INFO(0, key)
	ZEND_ARG_INFO(0, value)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_mruby_run, 0, 0, 1)
	ZEND_ARG_INFO(0, code)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_mruby_evaluate_script, 0, 0, 1)
	ZEND_ARG_INFO(0, script)
ZEND_END_ARG_INFO()


PHP_METHOD(mruby, assign)
{
	mrb_state *mrb;
	struct mrb_parser_state *p;
	php_mruby_t *object = NULL;
	int n = -1;
	char *name;
	long name_len = 0;
	zval *var;
	mrb_value val;
	
	object = (php_mruby_t *) zend_object_store_get_object(getThis() TSRMLS_CC);
	mrb = object->mrb;

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC,"sz",&name,&name_len,&var) == FAILURE) {
		return;
	}
	
	if (Z_TYPE_P(var) == IS_STRING) {
		val = mrb_str_new_cstr(mrb, Z_STRVAL_P(var));
	}

	mrb_gv_set(mrb, mrb_intern(mrb, name), val);
}

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

	if (n >= 0) {
		mrb_run(mrb, mrb_proc_new(mrb, mrb->irep[n]), mrb_top_self(mrb));
	}

	if (mrb->exc) {
		mrb_p(mrb, mrb_obj_value(mrb->exc));
	}
}

/* currently, this method does not return correct result */
PHP_METHOD(mruby, evaluateScript)
{
	mrb_state *mrb;
	struct mrb_parser_state *p;
	php_mruby_t *object = NULL;
	zval *z_result;
	mrb_value result;
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
	if (n >= 0) {
		result = mrb_run(mrb, mrb_proc_new(mrb, mrb->irep[n]), mrb_nil_value());
	}
	
	if (mrb->exc) {
		mrb_p(mrb, mrb_obj_value(mrb->exc));
	}

	php_mruby_convert_mrb_value(&z_result, mrb, result TSRMLS_CC);
	
	RETVAL_ZVAL(z_result,0,0);
}

static zend_function_entry php_mruby_methods[] = {
	PHP_ME(mruby, assign, arginfo_mruby_assign, ZEND_ACC_PUBLIC)
	PHP_ME(mruby, run, arginfo_mruby_run, ZEND_ACC_PUBLIC)
	PHP_ME(mruby, evaluateScript, arginfo_mruby_evaluate_script, ZEND_ACC_PUBLIC)
	{NULL, NULL, NULL}
};

void php_mruby_init(TSRMLS_D)
{
	zend_class_entry ce;

	INIT_CLASS_ENTRY(ce, "MRuby", php_mruby_methods);
	mruby_class_entry = zend_register_internal_class(&ce TSRMLS_CC);
	mruby_class_entry->create_object = php_mruby_new;
}
