#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ(2.59)
AC_INIT(h16, 1.4.1, adrian@adrianwise.co.uk)

AM_INIT_AUTOMAKE
AC_PROG_MAKE_SET

AC_COPYRIGHT([Copyright (c) 1997, 1998, 1999, 2004, 2005 Adrian Wise])
AC_CONFIG_SRCDIR([proc.cc])
AC_CONFIG_HEADER([config.h])

# Checks for programs.
AC_PROG_CC
AC_PROG_CXX
# AC_PROG_LIBTOOL

AM_PATH_GTK([1.2.10])

# Checks for libraries.
AC_CHECK_LIB([termcap], [tgoto])
AC_CHECK_LIB([history], [add_history])
AC_CHECK_LIB([readline], [readline])

# Checks for header files.
AC_HEADER_STDC
AC_CHECK_HEADERS([stdlib.h string.h unistd.h readline/readline.h readline/history.h])

# Checks for typedefs, structures, and compiler characteristics.
AC_HEADER_STDBOOL
AC_C_CONST
AC_C_INLINE
AC_TYPE_SIZE_T

# Checks for library functions.
AC_FUNC_MALLOC
AC_CHECK_FUNCS([floor strdup strtol])

dnl AC_CONFIG_FILES([Makefile
dnl		gtk/Makefile])
AC_CONFIG_FILES([Makefile])

AC_OUTPUT
