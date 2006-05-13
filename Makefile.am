bin_PROGRAMS = h16 h16-tabs h16-pdap

h16_SOURCES = 	asr_intf.cc \
		emul.cc \
		event.cc \
		instr.cc \
		iodev.cc \
		lpt.cc \
		monitor.cc \
		proc.cc \
		ptp.cc \
		ptr.cc \
		tree.cc \
		asr.cc \
		stdtty.cc \
		gpl.c \
		asr.hh \
		event.hh \
		iodev.hh \
		monitor.hh \
		ptp.hh \
		stdtty.hh \
		asr_intf.hh \
		instr.hh \
		lpt.hh \
		proc.hh \
		ptr.hh \
		tree.hh \
		gpl.h

if ENABLE_GUI

h16_SOURCES += 	gtk/fp.c \
		gtk/menu.c \
		gtk/fp.h \
		gtk/menu.h

h16_CPPFLAGS = $(GTK_CFLAGS) -I gtk

h16_LDADD = $(GTK_LIBS)

endif

nodist_h16_SOURCES = version.h

BUILT_SOURCES = version.h

version.h: $(h16_SOURCES)
	@echo "#define NAME \"$(PACKAGE_NAME)\"" > $@
	@echo "#define VERSION \"$(PACKAGE_VERSION)\"" >> $@
	@echo "#define COPYRIGHT \"`grep ^Copyright.*Wise$$ configure`\"" >> $@
	@echo "#define DATE \"`date`\"" >> $@

h16_tabs_SOURCES = utils/h16-tabs.c

h16_pdap_SOURCES = utils/h16-pdap.cc

EXTRA_DIST = 	data/m4h_defines.m4 \
		data/main.css \
		data/dap.css \
		scripts/h16-script.in \
		tapes/dap-16_mod2_slst.ptp

install-data-local:
	test -z "$(DESTDIR)$(pkgdatadir)/data" || $(mkdir_p) "$(DESTDIR)$(pkgdatadir)/data"
	$(INSTALL_DATA) $(srcdir)/data/m4h_defines.m4 $(DESTDIR)$(pkgdatadir)/data
	$(INSTALL_DATA) $(srcdir)/data/main.css $(DESTDIR)$(pkgdatadir)/data
	$(INSTALL_DATA) $(srcdir)/data/dap.css $(DESTDIR)$(pkgdatadir)/data

CLEANFILES = h16-script

do_subst = sed -e 's,[@]pkgdatadir[@],$(pkgdatadir),g'

h16-script: $(srcdir)/scripts/h16-script.in Makefile
	$(do_subst) < $< > $@

install-exec-local: h16-script
	test -z "$(DESTDIR)$(pkgdatadir)/scripts" || $(mkdir_p) "$(DESTDIR)$(pkgdatadir)/scripts"
	$(INSTALL_PROGRAM) h16-script $(DESTDIR)$(pkgdatadir)/scripts
	test -z "$(DESTDIR)$(pkgdatadir)/tapes" || $(mkdir_p) "$(DESTDIR)$(pkgdatadir)/tapes"
	$(INSTALL_DATA) dap-16_mod2_slst.ptp $(DESTDIR)$(pkgdatadir)/tapes
