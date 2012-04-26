$(srcdir)/mruby/src/y.tab.c: mruby/src/parse.y
	$(BISON) -o $@ $<

$(srcdir)/mruby/tools/mrbc/mrbc:
	$(MAKE) -C $(srcdir)/mruby/tools/mrbc

$(srcdir)/mruby/mrblib/mrblib.c: $(srcdir)/mruby/tools/mrbc/mrbc
	$(MAKE) -C $(srcdir)/mruby/mrblib mrblib.c
