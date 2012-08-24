#ifndef PHP_MRUBY_H

#define PHP_MRUBY_H

#define PHP_MRUBY_EXTNAME "mruby"
#define PHP_MRUBY_EXTVER "0.1"

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "php.h"
#include "mruby.h"
#include "mruby/proc.h"
#include "mruby/compile.h"
#include "mruby/dump.h"

#include "ext/spl/spl_exceptions.h"
#include "zend_exceptions.h"
#include "zend_interfaces.h"

/* Define the entry point symbol
 * Zend will use when loading this module
 */
extern zend_module_entry mruby_module_entry;
#define phpext_mruby_ptr &mruby_module_entry
#define PHP_MRUBY_OUTPUT_ARRAY        0

extern zend_class_entry *mruby_class_entry;
extern zend_class_entry *mruby_object_class_entry;

typedef struct {
	zend_object zo;
	mrb_state *mrb;
} php_mruby_t;

typedef struct {
	zend_object zo;
	zend_object_value owner;
	mrb_value value;
} php_mruby_object_t;

zval *php_mruby_convert_mrb_value(zend_object_value owner, mrb_value argv TSRMLS_DC);
mrb_value php_mruby_to_mrb_value(mrb_state *mrb, zval *src TSRMLS_DC);

#endif /* PHP_MRUBY_H */
