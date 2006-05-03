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

dist_pkgdata_DATA = utils/pkgdata/m4h_defines.m4
