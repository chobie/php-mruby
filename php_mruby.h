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
#include "mruby/src/compile.h"
#include "mruby/dump.h"

#include "ext/spl/spl_exceptions.h"
#include "zend_interfaces.h"

/* Define the entry point symbol
 * Zend will use when loading this module
 */
extern zend_module_entry mruby_module_entry;
#define phpext_mruby_ptr &mruby_module_entry;

extern zend_class_entry *mruby_class_entry;

typedef struct{
	zend_object zo;
	mrb_state *mrb;
} php_mruby_t;


#endif /* PHP_MRUBY_H */