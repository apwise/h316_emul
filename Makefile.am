bin_PROGRAMS = h16

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
		gtk/fp.c \
		gtk/menu.c \
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
		gpl.h \
		version.h \
		gtk/fp.h \
		gtk/menu.h

nodist_h16_SOURCES = date.h

BUILT_SOURCES = date.h

h16_CPPFLAGS = $(GTK_CFLAGS) -I gtk

h16_LDADD = $(GTK_LIBS)

date.h: $(h16_SOURCES)
	@echo "#define DATE \"`date`\"" > $@

