#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ([2.71])
AC_INIT([h16],[1.9.0],[apw@adrianwise.co.uk])

AM_INIT_AUTOMAKE
AC_PROG_MAKE_SET

AC_COPYRIGHT([Copyright (c) 1997-1999, 2004-2014, 2016-2020, 2024 Adrian Wise])
AC_CONFIG_SRCDIR([proc.cc])
AC_CONFIG_HEADERS([config.h])

# Checks for programs.
AC_PROG_CC
AC_PROG_CXX
AX_CXX_COMPILE_STDCXX_11(,[mandatory])

AC_ARG_ENABLE([gui],
              AS_HELP_STRING([--disable-gui],[Compile a text-only version with no GUI]),
              [], [enable_gui=yes])
AM_CONDITIONAL([ENABLE_GUI], [test x$enable_gui = xyes])
if test x$enable_gui = xyes; then
  AC_DEFINE_UNQUOTED([ENABLE_GUI], [1], [Define to 1 to compile with the GUI])
  PKG_CHECK_MODULES(GTK, gtk+-3.0)
fi

AC_ARG_ENABLE([verif],
              AS_HELP_STRING([--enable-verif],[Compile with support for verification devices]),
              [], [enable_verif=no])
AM_CONDITIONAL([ENABLE_VERIF], [test x$enable_verif = xyes])
if test x$enable_verif = xyes; then
  AC_DEFINE_UNQUOTED([ENABLE_VERIF], [1], [Define to 1 to compile with verification devices])
fi

AC_ARG_ENABLE([spi],
              AS_HELP_STRING([--enable-spi],[Compile with support for experimental SPI devices]),
              [], [enable_spi=no])
AM_CONDITIONAL([ENABLE_SPI], [test x$enable_spi = xyes])
if test x$enable_spi = xyes; then
  AC_DEFINE_UNQUOTED([ENABLE_SPI], [1], [Define to 1 to compile with SPI])
fi

# Checks for libraries.
AC_CHECK_LIB([termcap], [tgoto])
AC_CHECK_LIB([history], [add_history])
AC_CHECK_LIB([readline], [readline])
AC_CHECK_LIB([depp], [DeppEnable])

AM_CONDITIONAL([ENABLE_DEPP], [test x$ac_cv_lib_depp_DeppEnable = xyes])

# Checks for header files.
AC_CHECK_HEADERS([stdlib.h string.h unistd.h errno.h readline/readline.h readline/history.h])

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
