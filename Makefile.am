bin_PROGRAMS = h16 \
		h16-tabs h16-pdap h16-asctotty h16-ttytoasc \
		h16-tabify h16-asr h16-tape h16-ppl h16-plt2ps \
		h16-lib h16-pp-asr

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
		plt.cc \
		gpl.c \
		asr.hh \
		event.hh \
		iodev.hh \
		monitor.hh \
		ptp.hh \
		stdtty.hh \
		plt.hh \
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

h16_ppl_SOURCES = utils/h16-ppl.c

h16_asctotty_SOURCES = utils/h16-asctotty.c

h16_ttytoasc_SOURCES = utils/h16-ttytoasc.c

h16_tabify_SOURCES = utils/h16-tabify.cc instr.cc
h16_tabify_CXXFLAGS = -DNO_DO_PROCS

h16_asr_SOURCES = utils/h16-asr_app.cc utils/serial.cc utils/serial.hh \
		utils/dummy_proc.cc utils/dummy_proc.hh \
		asr.cc asr.hh stdtty.cc stdtty.hh

h16_pp_asr_SOURCES = utils/h16-pp-asr_app.cc \
		utils/dummy_proc.cc utils/dummy_proc.hh \
		asr.cc asr.hh stdtty.cc stdtty.hh \
		utils/pp_channel.h utils/pp_channel.c
h16_pp_asr_CXXFLAGS = -pthread
h16_pp_asr_LDADD = -lpthread

h16_tape_SOURCES = utils/h16-tape.cc instr.cc
h16_tape_CXXFLAGS = -DNO_DO_PROCS

h16_plt2ps_SOURCES = utils/h16-plt2ps.cc

h16_lib_SOURCES = utils/h16-lib.cc

EXTRA_DIST = 	data/m4h_defines.m4 \
		data/main.css \
		data/dap.css \
		scripts/h16-as.in \
		scripts/h16-as.m4 \
		scripts/h16-mac.m4 \
		tapes/dap-16_mod2_slst.ptp \
		tapes/mac_slst.ptp \
		scripts/h16-f4.in \
		scripts/h16-f4.m4 \
		tapes/fortran_slst.ptp

install-data-local: h16-as h16-f4
	test -z "$(DESTDIR)$(pkgdatadir)/data" || $(mkdir_p) "$(DESTDIR)$(pkgdatadir)/data"
	$(INSTALL_DATA) $(srcdir)/data/m4h_defines.m4 $(DESTDIR)$(pkgdatadir)/data
	$(INSTALL_DATA) $(srcdir)/data/main.css $(DESTDIR)$(pkgdatadir)/data
	$(INSTALL_DATA) $(srcdir)/data/dap.css $(DESTDIR)$(pkgdatadir)/data
	test -z "$(DESTDIR)$(pkgdatadir)/scripts" || $(mkdir_p) "$(DESTDIR)$(pkgdatadir)/scripts"
	$(INSTALL_PROGRAM) h16-as $(DESTDIR)$(pkgdatadir)/scripts
	$(INSTALL_PROGRAM) h16-f4 $(DESTDIR)$(pkgdatadir)/scripts
	$(INSTALL_DATA) $(srcdir)/scripts/h16-as.m4 $(DESTDIR)$(pkgdatadir)/scripts
	$(INSTALL_DATA) $(srcdir)/scripts/h16-mac.m4 $(DESTDIR)$(pkgdatadir)/scripts
	$(INSTALL_DATA) $(srcdir)/scripts/h16-f4.m4 $(DESTDIR)$(pkgdatadir)/scripts
	test -z "$(DESTDIR)$(pkgdatadir)/tapes" || $(mkdir_p) "$(DESTDIR)$(pkgdatadir)/tapes"
	$(INSTALL_DATA) $(srcdir)/tapes/dap-16_mod2_slst.ptp $(DESTDIR)$(pkgdatadir)/tapes
	$(INSTALL_DATA) $(srcdir)/tapes/mac_slst.ptp $(DESTDIR)$(pkgdatadir)/tapes
	$(INSTALL_DATA) $(srcdir)/tapes/fortran_slst.ptp $(DESTDIR)$(pkgdatadir)/tapes

CLEANFILES = h16-as h16-f4 version.h

do_subst = sed -e 's,[@]pkgdatadir[@],$(pkgdatadir),g'

h16-as: $(srcdir)/scripts/h16-as.in Makefile
	$(do_subst) < $< > $@

h16-f4: $(srcdir)/scripts/h16-f4.in Makefile
	$(do_subst) < $< > $@

install-exec-local:
	rm -f $(bindir)/h16-as && \
	{ ln -s $(DESTDIR)$(pkgdatadir)/scripts/h16-as $(DESTDIR)$(bindir)/h16-as || \
	  $(INSTALL_PROGRAM) h16-as $(DESTDIR)$(bindir); }
	rm -f $(bindir)/h16-f4 && \
	{ ln -s $(DESTDIR)$(pkgdatadir)/scripts/h16-f4 $(DESTDIR)$(bindir)/h16-f4 || \
	  $(INSTALL_PROGRAM) h16-f4 $(DESTDIR)$(bindir); }

uninstall-local:
	rm -rf $(DESTDIR)$(pkgdatadir)/tapes
	rm -rf $(DESTDIR)$(pkgdatadir)/data
	rm -rf $(DESTDIR)$(pkgdatadir)/scripts
	rm -f  $(DESTDIR)$(bindir)/h16-as
	rm -f  $(DESTDIR)$(bindir)/h16-f4
