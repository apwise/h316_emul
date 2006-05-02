#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ(2.59)
dnl AC_INIT(h16, 1.4.1, adrian@adrianwise.co.uk)
AC_INIT(h16_utils, 1.4.x, adrian@adrianwise.co.uk)

AM_INIT_AUTOMAKE
AC_PROG_MAKE_SET

AC_COPYRIGHT([Copyright (c) 2004, 2006 Adrian Wise])
AC_CONFIG_SRCDIR([h16-tabs.c])
AC_CONFIG_HEADER([config.h])

# Checks for programs.
AC_PROG_CC
AC_PROG_CXX

# Checks for header files.
AC_HEADER_STDC
AC_CHECK_HEADERS([stdlib.h string.h unistd.h])

# Checks for typedefs, structures, and compiler characteristics.
AC_HEADER_STDBOOL
AC_C_CONST
AC_C_INLINE
AC_TYPE_SIZE_T

# Checks for library functions.
AC_FUNC_MALLOC
AC_CHECK_FUNCS([floor strdup strtol])

AC_CONFIG_FILES([Makefile])

AC_OUTPUT
