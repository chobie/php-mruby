PHP_ARG_ENABLE(mruby, Whether to enable the "mruby" extension,
	[ --enable-mruby 	Enable "mruby" extension support])

if test $PHP_MRUBY != "no"; then

	PHP_CHECK_LIBRARY(ritevm, mrb_run, [
		AC_MSG_RESULT(found)
		PHP_ADD_LIBRARY_WITH_PATH(ritevm, mruby/lib, MRUBY_SHARED_LIBADD)
		PHP_ADD_INCLUDE(mruby/include)
	],[
		AC_MSG_RESULT([not found])
		AC_MSG_ERROR([Please install mruby first])
	],[
		-Lmruby/lib
	])
	PHP_SUBST(MRUBY_SHARED_LIBADD)

	PHP_NEW_EXTENSION(mruby,
		php_mruby.c \
		, $ext_shared)
		
fi
