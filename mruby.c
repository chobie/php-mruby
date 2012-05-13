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
#include "mruby/data.h"
#include "mruby/variable.h"

void php_mruby_init(TSRMLS_D);
zend_class_entry *mruby_class_entry;
zend_class_entry *mruby_object_class_entry;

static const struct mrb_data_type php_mruby_object_handle_data_type = {
	"PHPObjectHandle", NULL
};

static zend_object_value php_mruby_object_create(zend_object_value owner, mrb_value value TSRMLS_DC);
static zend_object_value php_mruby_get_env_backref(mrb_state *state);
static int php_mruby_determine_array_type(zval **val TSRMLS_DC);

/* are we need this here? */
KHASH_INIT(ht, mrb_value, mrb_value, 1, mrb_hash_ht_hash_func, mrb_hash_ht_hash_equal);

static zend_object_value php_mruby_get_env_backref(mrb_state *mrb)
{
	zend_object_value retval;
	retval.handle = (zend_object_handle)(uintptr_t)mrb_check_datatype(mrb, mrb_const_get(mrb, mrb_obj_value(mrb->object_class), mrb_intern(mrb, "__php__")), &php_mruby_object_handle_data_type);
	retval.handlers = zend_get_std_object_handlers();
	return retval;
}

/** serializes array to mrb_value */
mrb_value php_mruby_to_mrb_value(mrb_state *mrb, zval *value TSRMLS_DC) /* {{{ */
{
	switch (Z_TYPE_P(value)) {
		case IS_NULL:
			return mrb_nil_value();

		case IS_BOOL:
			return Z_LVAL_P(value) ? mrb_true_value(): mrb_false_value();

		case IS_LONG:
			return mrb_fixnum_value(Z_LVAL_P(value));

		case IS_DOUBLE:
			return mrb_float_value(Z_DOUBLE_P(value));

		case IS_STRING:
			return mrb_str_new(mrb, Z_STRVAL_P(value), Z_STRLEN_P(value));

		case IS_ARRAY: {
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

			if(php_mruby_determine_array_type(&value TSRMLS_CC) == PHP_MRUBY_OUTPUT_ARRAY) {
				tmp = mrb_ary_new(mrb);
				for (zend_hash_internal_pointer_reset_ex(h, &pos);
						(key_type = zend_hash_get_current_key_ex(h, &key, &key_len, &key_index, 0, &pos)) != HASH_KEY_NON_EXISTANT;
						zend_hash_move_forward_ex(h, &pos)) {
					zend_hash_get_current_data_ex(h, (void *) &d, &pos);
					
					mrb_ary_push(mrb, tmp, php_mruby_to_mrb_value(mrb, *d TSRMLS_CC));
				}
			} else {
				tmp = mrb_hash_new(mrb,n);
				for (zend_hash_internal_pointer_reset_ex(h, &pos);
						(key_type = zend_hash_get_current_key_ex(h, &key, &key_len, &key_index, 0, &pos)) != HASH_KEY_NON_EXISTANT;
						zend_hash_move_forward_ex(h, &pos)) {
					zend_hash_get_current_data_ex(h, (void *) &d, &pos);
					if (key_type == HASH_KEY_IS_STRING) {
						mrb_hash_set(mrb, tmp, mrb_str_new(mrb, key, key_len), php_mruby_to_mrb_value(mrb, *d TSRMLS_CC));
					} else {
						mrb_hash_set(mrb, tmp, mrb_fixnum_value(key_index), php_mruby_to_mrb_value(mrb, *d TSRMLS_CC));
					}
				}
			}
			return tmp;
		}

		case IS_OBJECT: {
			return mrb_str_new_cstr(mrb, "* FIXME *");
		}

		default: break;
	}
	php_error_docref(NULL TSRMLS_CC, E_ERROR, "Failed to convert a PHP value to MRuby value");
	/* never get here */
} /* }}} */

static zval *php_mruby_wrap(zend_object_value php_mrb, mrb_value value TSRMLS_DC) /* {{{ */
{
	zval *retval;
	ALLOC_INIT_ZVAL(retval);
	retval->type = IS_OBJECT;
	retval->value.obj = php_mruby_object_create(php_mrb, value TSRMLS_CC);
	return retval;
} /* }}} */

zval *php_mruby_convert_mrb_value(zend_object_value php_mrb, mrb_value argv TSRMLS_DC) /* {{{ */
{
	zval *retval = NULL;
	
	switch (mrb_type(argv)) {
		case MRB_TT_STRING: {
			ALLOC_INIT_ZVAL(retval);
			ZVAL_STRINGL(retval, RSTRING_PTR(argv), RSTRING_LEN(argv), 1);
			break;
		}
		case MRB_TT_FIXNUM: {
			ALLOC_INIT_ZVAL(retval);
			ZVAL_LONG(retval, (long)argv.value.i);
			break;
		}
		case MRB_TT_FLOAT: {
			ALLOC_INIT_ZVAL(retval);
			ZVAL_DOUBLE(retval, (double)argv.value.f);
			break;
		}
		case MRB_TT_ARRAY: {
			struct RArray *array_ptr;
			HashTable *hash;
			int i = 0;
			int length = RARRAY_LEN(argv);

			array_ptr = mrb_ary_ptr(argv);

			ALLOC_INIT_ZVAL(retval);
			array_init(retval);
			hash = Z_ARRVAL_P(retval);
			for (i = 0; i < length; i++) {
				add_next_index_zval(retval, php_mruby_convert_mrb_value(php_mrb, array_ptr->buf[i] TSRMLS_CC));
			}
			Z_ARRVAL_P(retval) = hash;
			break;
		}
		case MRB_TT_HASH: {
			struct kh_ht *h = RHASH_TBL(argv);
			khiter_t k;
			HashTable *hash;

			ALLOC_INIT_ZVAL(retval);
			array_init(retval);
			hash = Z_ARRVAL_P(retval);

			for (k = kh_begin(h); k != kh_end(h); k++) {
				zval *z_hash_key, *z_hash_val;

				if (!kh_exist(h, k)) {
					continue;
				}

				z_hash_key = php_mruby_convert_mrb_value(php_mrb, kh_key(h,k) TSRMLS_CC);
				z_hash_val = php_mruby_convert_mrb_value(php_mrb, kh_value(h,k) TSRMLS_CC);

				if (Z_TYPE_P(z_hash_key) != IS_STRING) {
					convert_to_string(z_hash_key);
				}
				
				zend_hash_add(hash, Z_STRVAL_P(z_hash_key), Z_STRLEN_P(z_hash_key) + 1, (void **)&z_hash_val, sizeof(z_hash_val), NULL);
				zval_ptr_dtor(&z_hash_key);
			}
			
			Z_ARRVAL_P(retval) = hash;
			break;
		}
		case MRB_TT_TRUE: {
			ALLOC_INIT_ZVAL(retval);
			ZVAL_BOOL(retval, TRUE);
			break;
		}
		case MRB_TT_FALSE: {
			ALLOC_INIT_ZVAL(retval);
			if (argv.value.p) {
				ZVAL_BOOL(retval, FALSE);
			} else {
				ZVAL_NULL(retval);
			}
			break;
		}
		default: {
			retval = php_mruby_wrap(php_mrb, argv TSRMLS_CC);
			break;
		}
	}
	return retval;
} /* }}} */

static mrb_value phplib_get_global_zval(mrb_state *mrb, char *name, size_t length)
{
	TSRMLS_FETCH();
	mrb_value result;
	zval **data;
	
	if (PG(auto_globals_jit)) {
		zend_is_auto_global(name, length-1 TSRMLS_CC);
	}
	if (zend_hash_find(&EG(symbol_table), name, length, (void **) &data) == SUCCESS) {
		result = php_mruby_to_mrb_value(mrb, *data TSRMLS_CC);
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
			if ((tmp = php_mruby_convert_mrb_value(php_mruby_get_env_backref(mrb), argv TSRMLS_CC))) {
				if (Z_TYPE_P(tmp) != IS_STRING) {
					convert_to_string(tmp);
				}
				
				zend_print_variable(tmp);
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
	zend_object_value php_mrb = php_mruby_get_env_backref(mrb);
	mrb_get_args(mrb, "*", &argv, &argc);

	if (argc > 1) {
		int offset = 0;
		param_count = argc-1;
		params = emalloc(sizeof(zval**) * param_count);
		for (offset = 0, i=1; i<argc;i++,offset++) {
			params[offset] = php_mruby_convert_mrb_value(php_mrb, argv[i] TSRMLS_CC);
		}
	} else {
		params = NULL;
		param_count = 0;
	}

	MAKE_STD_ZVAL(result);
	z_func_name = php_mruby_convert_mrb_value(php_mrb, argv[0] TSRMLS_CC);
	ret = call_user_function(&EG(symbol_table), NULL, z_func_name, result, param_count, params TSRMLS_CC);

	if (param_count > 0) {
		for (i=0; i<param_count;i++) {
			zval_ptr_dtor(&params[i]);
		}
		efree(params);
	}
	
	rb_result = php_mruby_to_mrb_value(mrb, result TSRMLS_CC);

	zval_ptr_dtor(&z_func_name);
	zval_ptr_dtor(&result);

	return rb_result;
}

static mrb_value phplib_var_dump(mrb_state *mrb, mrb_value self)
{
	TSRMLS_FETCH();
	
	zval *z_func_name, *result, **params;
	mrb_value *argv;
	int ret, i, argc = 0;
	mrb_get_args(mrb, "*", &argv, &argc);

	if (argc > 0) {
		zend_object_value php_mrb = php_mruby_get_env_backref(mrb);
		params = emalloc(sizeof(zval**) * argc);
		for (i=0; i<argc;i++) {
			params[i] = php_mruby_convert_mrb_value(php_mrb, argv[i] TSRMLS_CC);
		}
	} else {
		params = NULL;
	}

	MAKE_STD_ZVAL(result);
	MAKE_STD_ZVAL(z_func_name);
	ZVAL_STRING(z_func_name,"var_dump",1);
	ret = call_user_function(&EG(symbol_table), NULL, z_func_name, result, argc, params TSRMLS_CC);

	if (argc > 0) {
		for (i=0; i< argc; i++) {
			if (params[i] != NULL) {
				zval_ptr_dtor(&params[i]);
			}
		}
		efree(params);
	}
	
	zval_ptr_dtor(&z_func_name);
	zval_ptr_dtor(&result);
	
	return mrb_nil_value();
}

static void phplib_initialize(mrb_state *mrb)
{
	struct RClass *phplib;

	phplib = mrb_define_module(mrb, "PHP");
	mrb_define_class_method(mrb,phplib,"echo",phplib_echo,ARGS_REQ(1));
	mrb_define_class_method(mrb,phplib,"call_user_func",phplib_call_user_func,ARGS_ANY());
	mrb_define_class_method(mrb,phplib,"var_dump",phplib_var_dump,ARGS_ANY());
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

ZEND_BEGIN_ARG_INFO_EX(arginfo_mruby_assign, 0, 0, 1)
	ZEND_ARG_INFO(0, key)
	ZEND_ARG_INFO(0, value)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_mruby_run, 0, 0, 2)
	ZEND_ARG_INFO(0, code)
	ZEND_ARG_INFO(0, arguments)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_mruby_eval, 0, 0, 2)
	ZEND_ARG_INFO(0, script)
	ZEND_ARG_INFO(0, arguments)
ZEND_END_ARG_INFO()


PHP_METHOD(mruby, assign)
{
	mrb_state *mrb;
	php_mruby_t *object = NULL;
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
	zval *arguments = NULL;
	
	object = (php_mruby_t *) zend_object_store_get_object(getThis() TSRMLS_CC);
	mrb = object->mrb;

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC,"s|a",&code, &code_len, &arguments) == FAILURE) {
		return;
	}

	p = mrb_parse_string(mrb, code);
	n = mrb_generate_code(mrb, p->tree);
	mrb_pool_close(p->pool);

	if (arguments != NULL) {
		mrb_value ARGV;
		ARGV = php_mruby_to_mrb_value(mrb, arguments TSRMLS_CC);
		mrb_define_global_const(mrb, "ARGV", ARGV);
	}

	if (n >= 0) {
		mrb_run(mrb, mrb_proc_new(mrb, mrb->irep[n]), mrb_top_self(mrb));
	}

	if (mrb->exc) {
		mrb_p(mrb, mrb_obj_value(mrb->exc));
	}
}

/* currently, this method does not return correct result */
PHP_METHOD(mruby, eval)
{
	mrb_state *mrb;
	struct mrb_parser_state *p;
	php_mruby_t *object = NULL;
	mrb_value result;
	int n = -1;
	char *code;
	long code_len = 0;
	zval *arguments = NULL;
	
	object = (php_mruby_t *) zend_object_store_get_object(getThis() TSRMLS_CC);
	mrb = object->mrb;

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC,"s|a",&code, &code_len, &arguments) == FAILURE) {
		return;
	}

	p = mrb_parse_string(mrb, code);
	n = mrb_generate_code(mrb, p->tree);
	mrb_pool_close(p->pool);
	
	if (arguments != NULL) {
		mrb_value ARGV;
		ARGV = php_mruby_to_mrb_value(mrb, arguments TSRMLS_CC);
		mrb_define_global_const(mrb, "ARGV", ARGV);
	}

	if (n >= 0) {
		result = mrb_run(mrb, mrb_proc_new(mrb, mrb->irep[n]), mrb_nil_value());
	}
	
	if (mrb->exc) {
		mrb_p(mrb, mrb_obj_value(mrb->exc));
		RETURN_NULL();
	}

	{
		zval *tmp = php_mruby_convert_mrb_value(Z_OBJVAL_P(getThis()), result TSRMLS_CC);
		RETVAL_ZVAL(tmp,0,1);
	}
}

static zend_object_value php_mruby_new(zend_class_entry *ce TSRMLS_DC)
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
	{
		struct RClass *wrapper_class = mrb_define_class(obj->mrb, "PHPObjectHandle", obj->mrb->object_class);
		mrb_define_global_const(obj->mrb, "__php__", mrb_obj_value(mrb_data_object_alloc(obj->mrb, wrapper_class, (void *)(uintptr_t)retval.handle, &php_mruby_object_handle_data_type)));
	}
	return retval;
}

static zend_function_entry php_mruby_methods[] = {
	PHP_ME(mruby, assign, arginfo_mruby_assign, ZEND_ACC_PUBLIC)
	PHP_ME(mruby, run, arginfo_mruby_run, ZEND_ACC_PUBLIC)
	PHP_ME(mruby, eval, arginfo_mruby_eval, ZEND_ACC_PUBLIC)
	{NULL, NULL, NULL}
};

static php_mruby_mruby_class_init(TSRMLS_D)
{
	zend_class_entry ce;
	INIT_CLASS_ENTRY(ce, "MRuby", php_mruby_methods);
	mruby_class_entry = zend_register_internal_class(&ce TSRMLS_CC);
	mruby_class_entry->create_object = php_mruby_new;
}

static zval *php_mruby_object_property_read(zval *zv, zval *member, int type TSRMLS_DC) /* {{{ */
{
	zval *retval;
	php_mruby_object_t *object = (php_mruby_object_t *)zend_object_store_get_object(zv TSRMLS_CC);
	mrb_state *mrb = ((php_mruby_t *)zend_object_store_get_object_by_handle(object->owner.handle TSRMLS_CC))->mrb;

	if (UNEXPECTED(Z_TYPE_P(member) != IS_STRING)) {
		zval *tmp;
		ALLOC_ZVAL(tmp);
		*tmp = *member;
		INIT_PZVAL(tmp);
		zval_copy_ctor(tmp);
		convert_to_string(tmp);
		member = tmp;
	} else {
		Z_ADDREF_P(member);
	}

	retval = php_mruby_convert_mrb_value(object->owner, mrb_funcall(mrb, object->value, Z_STRVAL_P(member), 0) TSRMLS_CC);
	Z_DELREF_P(retval); /* this is ok. retval gets addref'ed after the function call */

	zval_ptr_dtor(&member);
	return retval;
} /* }}} */

static void php_mruby_object_property_write(zval *zv, zval *member, zval *value_zv TSRMLS_DC) /* {{{ */
{
	zval member_with_equal;
	php_mruby_object_t *object = (php_mruby_object_t *)zend_object_store_get_object(zv TSRMLS_CC);
	mrb_state *mrb = ((php_mruby_t *)zend_object_store_get_object_by_handle(object->owner.handle TSRMLS_CC))->mrb;

	member_with_equal = *member;
	zval_copy_ctor(&member_with_equal);
	if (Z_TYPE(member_with_equal) != IS_STRING) {
		convert_to_string(&member_with_equal);
	}

	{
		int len = Z_STRLEN(member_with_equal);	
		char *tmp = Z_STRVAL(member_with_equal);
		tmp = erealloc(tmp, len + 2);
		tmp[len] = '=';
		tmp[len + 1] = '\0';
		ZVAL_STRINGL(&member_with_equal, tmp, len + 1, 0);
	}

	mrb_funcall(mrb, object->value, Z_STRVAL(member_with_equal), 1, php_mruby_to_mrb_value(mrb, value_zv TSRMLS_CC));
	zval_dtor(&member_with_equal);
}
/* }}} */

static zval *php_mruby_object_dimension_read(zval *zv, zval *offset, int type TSRMLS_DC) /* {{{ */
{
	zval offset_with_brackets;
	zval *retval;
	php_mruby_object_t *object = (php_mruby_object_t *)zend_object_store_get_object(zv TSRMLS_CC);
	mrb_state *mrb = ((php_mruby_t *)zend_object_store_get_object_by_handle(object->owner.handle TSRMLS_CC))->mrb;

	offset_with_brackets = *offset;
	zval_copy_ctor(&offset_with_brackets);
	if (Z_TYPE(offset_with_brackets) != IS_STRING) {
		convert_to_string(&offset_with_brackets);
	}

	{
		int len = Z_STRLEN(offset_with_brackets);	
		char *tmp = Z_STRVAL(offset_with_brackets);
		tmp = erealloc(tmp, len + 3);
		tmp[len] = '[';
		tmp[len + 1] = ']';
		tmp[len + 2] = '\0';
		ZVAL_STRINGL(&offset_with_brackets, tmp, len + 2, 0);
	}

	retval = php_mruby_convert_mrb_value(object->owner, mrb_funcall(mrb, object->value, Z_STRVAL(offset_with_brackets), 0) TSRMLS_CC);
	Z_DELREF_P(retval); /* this is ok. retval gets addref'ed after the function call */

	zval_dtor(&offset_with_brackets);
	return retval;
} /* }}} */

static void php_mruby_object_dimension_write(zval *zv, zval *offset, zval *value_zv TSRMLS_DC) /* {{{ */
{
	zval offset_with_brackets_and_equal;
	php_mruby_object_t *object = (php_mruby_object_t *)zend_object_store_get_object(zv TSRMLS_CC);
	mrb_state *mrb = ((php_mruby_t *)zend_object_store_get_object_by_handle(object->owner.handle TSRMLS_CC))->mrb;

	offset_with_brackets_and_equal = *offset;
	zval_copy_ctor(&offset_with_brackets_and_equal);
	if (Z_TYPE(offset_with_brackets_and_equal) != IS_STRING) {
		convert_to_string(&offset_with_brackets_and_equal);
	}

	{
		int len = Z_STRLEN(offset_with_brackets_and_equal);	
		char *tmp = Z_STRVAL(offset_with_brackets_and_equal);
		tmp = erealloc(tmp, len + 4);
		tmp[len] = '[';
		tmp[len + 1] = ']';
		tmp[len + 2] = '=';
		tmp[len + 3] = '\0';
		ZVAL_STRINGL(&offset_with_brackets_and_equal, tmp, len + 3, 0);
	}

	mrb_funcall(mrb, object->value, Z_STRVAL(offset_with_brackets_and_equal), 1, php_mruby_to_mrb_value(mrb, value_zv TSRMLS_CC));
	zval_dtor(&offset_with_brackets_and_equal);
} /* }}} */

static int php_mruby_object_property_exists(zval *zv, zval *member, int check_empty TSRMLS_DC) /* {{{ */
{
	php_mruby_object_t *object = (php_mruby_object_t *)zend_object_store_get_object(zv TSRMLS_CC);
	mrb_state *mrb = ((php_mruby_t *)zend_object_store_get_object_by_handle(object->owner.handle TSRMLS_CC))->mrb;
	int retval;

	if (UNEXPECTED(Z_TYPE_P(member) != IS_STRING)) {
		zval *tmp;
		ALLOC_ZVAL(tmp);
		*tmp = *member;
		INIT_PZVAL(tmp);
		zval_copy_ctor(tmp);
		convert_to_string(tmp);
		member = tmp;
	} else {
		Z_ADDREF_P(member);
	}

	retval = mrb_obj_respond_to(mrb_obj_class(mrb, object->value), mrb_intern(mrb, Z_STRVAL_P(member)));
	zval_ptr_dtor(&member);
	return retval;
} /* }}} */

static int php_mruby_object_compare(zval *lhs_zv, zval *rhs_zv TSRMLS_DC) /* {{{ */
{
	php_mruby_object_t *lhs = (php_mruby_object_t *)zend_object_store_get_object(lhs_zv TSRMLS_CC);
	php_mruby_object_t *rhs = (php_mruby_object_t *)zend_object_store_get_object(rhs_zv TSRMLS_CC);

	if (rhs == NULL) {
		zend_throw_exception(zend_exception_get_default(TSRMLS_C), "RHS operand is not a MRuby object wrapper", 0 TSRMLS_CC);
		return 0;
	}

	if (lhs->owner.handle != rhs->owner.handle) {
		zend_throw_exception(zend_exception_get_default(TSRMLS_C), "Cannot compare objects of the different owners", 0 TSRMLS_CC);
		return 0;
	}

	{
		mrb_value tmp = mrb_funcall(((php_mruby_t *)zend_object_store_get_object_by_handle(lhs->owner.handle TSRMLS_CC))->mrb, lhs->value, "<=>", 1, rhs->value);
		if (mrb_type(tmp) != MRB_TT_FIXNUM) {
			zend_throw_exception(zend_exception_get_default(TSRMLS_C), "MRuby object responded with a non-Fixnum value, which is invalid", 0 TSRMLS_CC);
			return 0;
		}
		return tmp.value.i;
	}

} /* }}} */


static int php_mruby_object_call_method(const char *method, INTERNAL_FUNCTION_PARAMETERS) /* {{{ */
{
	zval *retval;
	php_mruby_object_t *object = (php_mruby_object_t *)zend_object_store_get_object(getThis() TSRMLS_CC);
	mrb_state *mrb = ((php_mruby_t *)zend_object_store_get_object_by_handle(object->owner.handle TSRMLS_CC))->mrb;
	zval ***args;
	mrb_value argv[16];
	int argc = 0;
	int i;

	if (ZEND_NUM_ARGS() > 0) {
		if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "+", &args, &argc) == FAILURE) {
			return;
		}

		for (i = 0; i < argc; i++) {
			argv[i] = php_mruby_to_mrb_value(mrb, *args[i] TSRMLS_CC);
		}
		
		retval = php_mruby_convert_mrb_value(object->owner, mrb_funcall_argv(mrb, object->value, method, argc, argv) TSRMLS_CC);
		efree(args);
	} else {
		retval = php_mruby_convert_mrb_value(object->owner, mrb_funcall(mrb, object->value, method, 0) TSRMLS_CC);
	}

	RETVAL_ZVAL(retval,0,1);
	return 0; /* FIXME: is this okay? */
} /* }}} */

static union _zend_function* php_mruby_object_get_method(zval **object_ptr, char *method, int method_len, const struct _zend_literal *key TSRMLS_DC) /* {{{ */
{
	union _zend_function *zf = NULL;
	zf = ecalloc(1,sizeof(union _zend_function));
	
	zf->type = ZEND_OVERLOADED_FUNCTION;
	zf->common.function_name = method;
	zf->common.scope = object_ptr;
	zf->common.fn_flags = ZEND_ACC_CALL_VIA_HANDLER;

	return zf;
} /* }}} */

static int php_mruby_object_cast(zval *zv, zval *result, int type TSRMLS_DC) /* {{{ */
{
	php_mruby_object_t *object = (php_mruby_object_t *)zend_object_store_get_object(zv TSRMLS_CC);
	mrb_state *mrb = ((php_mruby_t *)zend_object_store_get_object_by_handle(object->owner.handle TSRMLS_CC))->mrb;

	switch (type) {
		case IS_STRING:
			{
				zval *tmp = php_mruby_convert_mrb_value(object->owner, mrb_funcall(mrb, object->value, "to_s", 0) TSRMLS_CC);
				*result = *tmp;
				if (Z_TYPE_P(result) != IS_STRING) {
					convert_to_string(result);
				}
				efree(tmp);
			}
			return SUCCESS;
		default:
			INIT_PZVAL(result);
			Z_TYPE_P(result) = IS_NULL;
			break;
	}
	return FAILURE;
}
/* }}} */

static zend_object_handlers php_mruby_object_handlers = { /* {{{ */
	ZEND_OBJECTS_STORE_HANDLERS,
	php_mruby_object_property_read,
	php_mruby_object_property_write,
	php_mruby_object_dimension_read,
	php_mruby_object_dimension_write,
	NULL, /* get_addr - impossible */
	NULL, /* get */
	NULL, /* set */
	php_mruby_object_property_exists,
	NULL, /* unset_property */
	NULL, /* has_dimension */
	NULL, /* unset_dimension */
	NULL, /* get_properties */
	php_mruby_object_get_method, /* get_method */
	php_mruby_object_call_method, /* call_method */
	NULL, /* get_constructor */
	NULL, /* get_class */
	NULL, /* get_class_name */
	php_mruby_object_compare, /* compare_objects */
	php_mruby_object_cast, /* cast_object */
	NULL, /* count_elements */
	NULL  /* debug_info */
}; /* }}} */

static zend_function_entry php_mruby_object_methods[] = {
	{NULL, NULL, NULL}
};

static zend_object_value php_mruby_object_new(zend_class_entry *ce TSRMLS_DC) /* {{{ */
{
	zend_object *intern;
	zend_object_value retval = zend_objects_new(&intern, ce TSRMLS_CC);
	php_error_docref(NULL TSRMLS_CC, E_ERROR, "%s cannot be instantiated directly", ce->name);
	return retval;
} /* }}} */

static php_mruby_object_free_storage(void *_obj TSRMLS_DC)
{
	php_mruby_object_t *obj = _obj;
	zend_object_std_dtor(&obj->zo TSRMLS_CC);
	zend_objects_store_del_ref_by_handle_ex(obj->owner.handle, obj->owner.handlers TSRMLS_CC);
	efree(obj);
}

static zend_object_value php_mruby_object_create(zend_object_value owner, mrb_value value TSRMLS_DC) /* {{{ */
{
	zend_object_value retval;
	php_mruby_object_t *obj = ecalloc(1, sizeof(*obj));

	zend_object_std_init(&obj->zo, mruby_object_class_entry TSRMLS_CC);
	obj->owner = owner;
	obj->value = value;
	zend_objects_store_add_ref_by_handle(owner.handle TSRMLS_CC);

	retval.handle = zend_objects_store_put(obj, 
		(zend_objects_store_dtor_t)zend_objects_destroy_object,
		(zend_objects_free_object_storage_t)php_mruby_object_free_storage,
		NULL TSRMLS_CC);
	retval.handlers = &php_mruby_object_handlers;

	return retval;
} /* }}} */

static php_mruby_mruby_object_class_init(TSRMLS_D)
{
	zend_class_entry ce;
	INIT_CLASS_ENTRY(ce, "MRubyObject", php_mruby_object_methods);
	php_mruby_object_handlers.get_class_entry = std_object_handlers.get_class_entry;
	php_mruby_object_handlers.get_class_name = std_object_handlers.get_class_name;
	mruby_object_class_entry = zend_register_internal_class(&ce TSRMLS_CC);
	mruby_object_class_entry->create_object = php_mruby_object_new;
}

void php_mruby_init(TSRMLS_D)
{
	php_mruby_mruby_class_init(TSRMLS_C);
	php_mruby_mruby_object_class_init(TSRMLS_C);
}

/* this came from PHP's json.c. 0 means array, 1 means hash */
static int php_mruby_determine_array_type(zval **val TSRMLS_DC) /* {{{ */
{
	int i;
	HashTable *myht = HASH_OF(*val);

	i = myht ? zend_hash_num_elements(myht) : 0;
	if (i > 0) {
		char *key;
		ulong index, idx;
		uint key_len;
		HashPosition pos;

		zend_hash_internal_pointer_reset_ex(myht, &pos);
		idx = 0;
		for (;; zend_hash_move_forward_ex(myht, &pos)) {
			i = zend_hash_get_current_key_ex(myht, &key, &key_len, &index, 0, &pos);
			if (i == HASH_KEY_NON_EXISTANT)
				break;

			if (i == HASH_KEY_IS_STRING) {
				return 1;
			} else {
				if (index != idx) {
					return 1;
				}
			}
			idx++;
		}
	}

	return PHP_MRUBY_OUTPUT_ARRAY;
}

/*
 * vim: noet
 */
