PHP_ARG_ENABLE(mruby, Whether to enable the "mruby" extension,
	[ --enable-mruby 	Enable "mruby" extension support])

if test $PHP_MRUBY != "no"; then
    MRUBY_SOURCES="
mruby/src/array.c
mruby/src/ascii.c
mruby/src/cdump.c
mruby/src/class.c
mruby/src/codegen.c
mruby/src/compar.c
mruby/src/crc.c
mruby/src/dump.c
mruby/src/encoding.c
mruby/src/enum.c
mruby/src/error.c
mruby/src/etc.c
mruby/src/gc.c
mruby/src/hash.c
mruby/src/init.c
mruby/src/init_ext.c
mruby/src/kernel.c
mruby/src/load.c
mruby/src/minimain.c
mruby/src/numeric.c
mruby/src/object.c
mruby/src/pool.c
mruby/src/print.c
mruby/src/proc.c
mruby/src/range.c
mruby/src/re.c
mruby/src/regcomp.c
mruby/src/regenc.c
mruby/src/regerror.c
mruby/src/regexec.c
mruby/src/regparse.c
mruby/src/sprintf.c
mruby/src/st.c
mruby/src/state.c
mruby/src/string.c
mruby/src/struct.c
mruby/src/symbol.c
mruby/src/transcode.c
mruby/src/unicode.c
mruby/src/us_ascii.c
mruby/src/utf_8.c
mruby/src/variable.c
mruby/src/version.c
mruby/src/vm.c
mruby/src/y.tab.c
mruby/mrblib/mrblib.c
"
    PHP_NEW_EXTENSION(mruby, php_mruby.c mruby.c $MRUBY_SOURCES, $ext_shared)
    PHP_SUBST(MRUBY_SHARED_LIBADD)

    PHP_ADD_BUILD_DIR([$ext_builddir/mruby])

    PHP_ADD_INCLUDE([$ext_srcdir/mruby/include])
    PHP_ADD_INCLUDE([$ext_srcdir/mruby/src])
    PHP_ADD_INCLUDE([$ext_builddir/mruby/include])
    PHP_ADD_INCLUDE([$ext_builddir/mruby/src])
    PHP_ADD_MAKEFILE_FRAGMENT
fi
