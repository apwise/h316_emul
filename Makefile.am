
dnl SUBDIRS = gtk

bin_PROGRAMS = h16

h16_SOURCES = 	asr_intf.cc \
		emul.cc \
		event.cc \
		gpl.c \
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
		gtk/fp.c \
		gtk/menu.c

h16_CPPFLAGS = $(GTK_CFLAGS) -I gtk

h16_LDADD = $(GTK_LIBS)

dnl h16_LDADD = gtk/libgtkfp.la
