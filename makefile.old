#######################################################
# $Id: makefile,v 1.4 2004/04/21 21:20:27 adrian Exp $
#
# Created 15 January 1998 Adrian Wise
#
# Copyright (c) 1998 Adrian WIse
#
# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 2 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software
# Foundation, Inc., 59 Temple Place, Suite 330, Boston,
# MA  02111-1307 USA
#
# $Log: makefile,v $
# Revision 1.4  2004/04/21 21:20:27  adrian
# Batch operation and line-printer
#
# Revision 1.3  2001/06/09 22:12:02  adrian
# Various bug fixes
#
# Revision 1.2  1999/02/25 06:54:55  adrian
# Make executable even if this is a sub-directory
#
# Revision 1.1  1999/02/20 00:06:35  adrian
# Initial revision
#
#
#######################################################


#
# location of the top of the project tree
#
topdir := ../

#
# List the subdirectories which must be visited
#
ifneq ($(strip $(NO_GTK)),)
subdirs :=
project_cxxflags := -DNO_GTK=1
else
#
# Use gtk-config to get appropriate cflags and libs options
#
GTK_LIBS = $(shell gtk-config --libs)

#
# This picks up the first -L/... path from the gtk libs
# option (which is assumed to be where the gtk libraries
# are installed) and adds the Gtk--/include directory to
# form an appropriate -I/... flag for gtk-- code
#
lib_dir_flags = $(filter -L/% , $(GTK_LIBS))
gnu_lib_dir_flag = $(patsubst -L%, %, $(word 1, $(lib_dir_flags)))

#
# Build the appropriate macros for the built-in compilation
# rules.
# Note that since the preprocessor needs to see th -I/...
# flags CPPFLAGS is used here in preference to CXXFLAGS.
#
#LOADLIBES = $(GTK_LIBS) -lgtkmm
LOADLIBES = $(GTK_LIBS) -lreadline -lhistory -ltermcap
subdirs := gtk
endif

#
# List the directories containing library code
#
libdirs := ../asrlib

#
# list the "C" and "C++" source files in this directory
# which are part of the overall project
#
csources := gpl.c
cxxsources := instr.cc proc.cc emul.cc iodev.cc ptr.cc asr_intf.cc\
	event.cc tree.cc monitor.cc ptp.cc lpt.cc

#
# Stuff for testing this portion of the project
#
testprog := h16
testcsources :=
testcxxsources :=
libraries := 

#
# Set 'PLUS_TEST' because the "testcode" in this directory
# isn't really testcode - it's an executable in its own right.
# This causes the executable to be rebuilt.
#
PLUS_TEST := 1

#
# The first thing make all (or just make) should do is
# delete the dat-stamp file which causes it to rebuilt
#
all: remove_date

include $(topdir)makebody

no_gtk:
	@make NO_GTK=1 SPECIAL_TARGET=1

#
# date file to be rebuilt each time we build...
#
$(objdir)/monitor.o $(objdir)/monitor.d:	obj/date.h
$(objdir)/gpl.o $(objdir)/gpl.d:	obj/date.h

obj/date.h: $(objdir)/.exists
	@echo "#define DATE \"`date`\"" > $@

remove_date: FORCE
	@rm -f obj/date.h


