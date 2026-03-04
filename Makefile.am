bin_PROGRAMS = 	h16

#		h16-tabs h16-pdap h16-asctotty h16-ttytoasc \
#		h16-tabify h16-asr h16-tape h16-ppl h16-plt2ps \
#		h16-lib h16-pp-asr h16-disasm h16-leader
#

if ENABLE_DEPP
  bin_PROGRAMS += h16-depp-asr 
endif

h16_SOURCES = 	dum.cpp \
		asr_intf.cpp \
		emul.cpp \
		event_queue.cpp \
		instr.cpp \
		io_dispatch.cpp \
		monitor.cpp \
		proc.cpp \
		iodev.cpp \
		mfm.cpp \
		asr.cpp \
		stdtty.cpp \
		tty_file.cpp \
		rtc.cpp \
		gpl.c \
		dum.hpp \
		asr.hpp \
		event_queue.hpp \
		io_to_p_intf.hpp \
		io_to_p_intf.cpp \
		p_to_io_intf.hpp \
		p_to_io_intf.cpp \
		p_to_dmc_intf.hpp \
		iodev.hpp \
		io_dispatch.hpp \
		monitor.hpp \
		mfm.hpp \
		ptp.hpp \
		stdtty.hpp \
		plt.hpp \
		tty_file.hpp \
		asr_intf.hpp \
		instr.hpp \
		lpt.hpp \
		proc.hpp \
		ptr.hpp \
		rtc.hpp \
		lpt.cpp \
		ptp.cpp \
		ptr.cpp \
		plt.cpp \
		gpl.h

AM_CXXFLAGS = -Wall -Werror
AM_CFLAGS = -Wall -Werror
AUTOMAKE_OPTIONS = subdir-objects

if ENABLE_GUI

h16_SOURCES += 	gtk/fp.c \
		gtk/menu.c \
		gtk/fp.h \
		gtk/menu.h

h16_CPPFLAGS = $(GTK_CFLAGS) -I gtk

h16_LDADD = $(GTK_LIBS)

endif

if ENABLE_VERIF

h16_SOURCES +=  vdmc.cpp \
		vdmc.hpp \
		vsim.cpp \
		vsim.hpp

endif


if ENABLE_SPI

h16_SOURCES += 	spi.cpp \
		spi.hpp \
		spi_dev.hpp \
		fram.cpp \
		fram.hpp

endif

nodist_h16_SOURCES = version.h

BUILT_SOURCES = version.h

version.h: $(h16_SOURCES)
	@echo "#define NAME \"$(PACKAGE_NAME)\"" > $@
	@echo "#define VERSION \"$(PACKAGE_VERSION)\"" >> $@
	@echo "#define COPYRIGHT \"`grep ^Copyright.*Wise$$ $(srcdir)/configure`\"" >> $@
	@echo "#define DATE \"`date`\"" >> $@

# h16_tabs_SOURCES = utils/h16-tabs.c

# h16_pdap_SOURCES = utils/h16-pdap.cpp

# h16_ppl_SOURCES = utils/h16-ppl.c

# h16_asctotty_SOURCES = utils/h16-asctotty.cpp tty_file.cpp tty_file.hpp

# h16_ttytoasc_SOURCES = utils/h16-ttytoasc.cpp tty_file.cpp tty_file.hpp

# h16_tabify_SOURCES = utils/h16-tabify.cpp instr.cpp
# h16_tabify_CXXFLAGS = -DNO_DO_PROCS -Wall -Werror

# h16_asr_SOURCES = utils/h16-asr_app.cpp utils/serial.cpp utils/serial.hpp \
# 		utils/dummy_proc.cpp utils/dummy_proc.hpp \
# 		asr.cpp asr.hpp tty_file.cpp tty_file.hpp stdtty.cpp stdtty.hpp

# h16_pp_asr_SOURCES = utils/h16-pp-asr_app.cpp \
# 		utils/dummy_proc.cpp utils/dummy_proc.hpp \
# 		asr.cpp asr.hpp tty_file.cpp tty_file.hpp stdtty.cpp stdtty.hpp \
# 		utils/pp_channel.h utils/pp_channel.c
# h16_pp_asr_CXXFLAGS = -pthread -Wall -Werror
# h16_pp_asr_LDADD = -lpthread

# h16_depp_asr_SOURCES = utils/h16-depp-asr_app.cpp \
# 		utils/dummy_proc.cpp utils/dummy_proc.hpp \
# 		asr.cpp asr.hpp tty_file.cpp tty_file.hpp stdtty.cpp stdtty.hpp \
# 		utils/depp_channel.h utils/depp_channel.c
# h16_depp_asr_CFLAGS = -I/usr/local/include/digilent/adept -Wall -Werror
# h16_depp_asr_CXXFLAGS = -pthread -I/usr/local/include/digilent/adept -Wall -Werror
# h16_depp_asr_LDADD = -lpthread -L/usr/local/lib64/digilent/adept -ldmgr -ldepp

# h16_tape_SOURCES = utils/h16-tape.cpp instr.cpp
# h16_tape_CXXFLAGS = -DNO_DO_PROCS -Wall -Werror

# h16_plt2ps_SOURCES = utils/h16-plt2ps.cpp

# h16_lib_SOURCES = utils/h16-lib.cpp

# h16_disasm_SOURCES = utils/h16-disasm.cpp instr.cpp
# h16_disasm_CXXFLAGS = -DNO_DO_PROCS -Wall -Werror

# h16_leader_SOURCES = utils/h16-leader.c

EXTRA_DIST = 	data/m4h_defines.m4 \
		data/main.css \
		data/dap.css \
		scripts/h16-as.in \
		scripts/h16-as.m4 \
		scripts/h16-mac.m4 \
		scripts/h16-ld.in \
		tapes/dap-16_mod2_slst.ptp \
		tapes/mac_slst.ptp \
		tapes/ldrapm-76-palap-77.slst \
		scripts/h16-f4.in \
		scripts/h16-f4.m4 \
		tapes/fortran_slst.ptp

install-data-local: h16-as h16-f4 h16-ld
	test -z "$(DESTDIR)$(pkgdatadir)/data" || $(mkdir_p) "$(DESTDIR)$(pkgdatadir)/data"
	$(INSTALL_DATA) $(srcdir)/data/m4h_defines.m4 $(DESTDIR)$(pkgdatadir)/data
	$(INSTALL_DATA) $(srcdir)/data/main.css $(DESTDIR)$(pkgdatadir)/data
	$(INSTALL_DATA) $(srcdir)/data/dap.css $(DESTDIR)$(pkgdatadir)/data
	test -z "$(DESTDIR)$(pkgdatadir)/scripts" || $(mkdir_p) "$(DESTDIR)$(pkgdatadir)/scripts"
	$(INSTALL_PROGRAM) h16-as $(DESTDIR)$(pkgdatadir)/scripts
	$(INSTALL_PROGRAM) h16-f4 $(DESTDIR)$(pkgdatadir)/scripts
	$(INSTALL_PROGRAM) h16-ld $(DESTDIR)$(pkgdatadir)/scripts
	$(INSTALL_DATA) $(srcdir)/scripts/h16-as.m4 $(DESTDIR)$(pkgdatadir)/scripts
	$(INSTALL_DATA) $(srcdir)/scripts/h16-mac.m4 $(DESTDIR)$(pkgdatadir)/scripts
	$(INSTALL_DATA) $(srcdir)/scripts/h16-f4.m4 $(DESTDIR)$(pkgdatadir)/scripts
	test -z "$(DESTDIR)$(pkgdatadir)/tapes" || $(mkdir_p) "$(DESTDIR)$(pkgdatadir)/tapes"
	$(INSTALL_DATA) $(srcdir)/tapes/dap-16_mod2_slst.ptp $(DESTDIR)$(pkgdatadir)/tapes
	$(INSTALL_DATA) $(srcdir)/tapes/mac_slst.ptp $(DESTDIR)$(pkgdatadir)/tapes
	$(INSTALL_DATA) $(srcdir)/tapes/ldrapm-76-palap-77.slst $(DESTDIR)$(pkgdatadir)/tapes
	$(INSTALL_DATA) $(srcdir)/tapes/fortran_slst.ptp $(DESTDIR)$(pkgdatadir)/tapes

CLEANFILES = h16-as h16-ld h16-f4 version.h

do_subst = sed -e 's,[@]pkgdatadir[@],$(pkgdatadir),g'

%: $(srcdir)/scripts/%.in Makefile
	$(do_subst) < $< > $@

install-exec-local:
	rm -f $(bindir)/h16-as && \
	{ ln -s $(DESTDIR)$(pkgdatadir)/scripts/h16-as $(DESTDIR)$(bindir)/h16-as || \
	  $(INSTALL_PROGRAM) h16-as $(DESTDIR)$(bindir); }
	rm -f $(bindir)/h16-ld && \
	{ ln -s $(DESTDIR)$(pkgdatadir)/scripts/h16-ld $(DESTDIR)$(bindir)/h16-ld || \
	  $(INSTALL_PROGRAM) h16-ld $(DESTDIR)$(bindir); }
	rm -f $(bindir)/h16-f4 && \
	{ ln -s $(DESTDIR)$(pkgdatadir)/scripts/h16-f4 $(DESTDIR)$(bindir)/h16-f4 || \
	  $(INSTALL_PROGRAM) h16-f4 $(DESTDIR)$(bindir); }

uninstall-local:
	rm -rf $(DESTDIR)$(pkgdatadir)/tapes
	rm -rf $(DESTDIR)$(pkgdatadir)/data
	rm -rf $(DESTDIR)$(pkgdatadir)/scripts
	rm -f  $(DESTDIR)$(bindir)/h16-as
	rm -f  $(DESTDIR)$(bindir)/h16-ld
	rm -f  $(DESTDIR)$(bindir)/h16-f4
