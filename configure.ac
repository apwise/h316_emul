#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ(2.59)
dnl AC_INIT(h16, 1.4.1, adrian@adrianwise.co.uk)
AC_INIT(h16, 1.4.x, adrian@adrianwise.co.uk)

AM_INIT_AUTOMAKE
AC_PROG_MAKE_SET

AC_COPYRIGHT([Copyright (c) 1997, 1998, 1999, 2004, 2005, 2006 Adrian Wise])
AC_CONFIG_SRCDIR([proc.cc])
AC_CONFIG_HEADER([config.h])

# Checks for programs.
AC_PROG_CC
AC_PROG_CXX


AC_ARG_ENABLE([gui],
              AS_HELP_STRING([--disable-gui],[Compile a text-only version with no GUI]),
	      [], [enable_gui=yes])
AM_CONDITIONAL([ENABLE_GUI], [test x$enable_gui = xyes])
if test x$enable_gui = xyes; then
  AC_DEFINE_UNQUOTED([ENABLE_GUI], [1], [Define to 1 to compile with the GUI])
  AM_PATH_GLIB([1.2.10])
  AM_PATH_GTK([1.2.10])
fi

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

AC_CONFIG_FILES([Makefile])

AC_CONFIG_SUBDIRS([utils])

AC_OUTPUT
