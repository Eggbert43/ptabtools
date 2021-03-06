# Process this file with autoconf to produce a configure script.

AC_PREREQ(2.50)
AC_INIT(ptabtools, 0.5.0, [jelmer@samba.org])
AC_CONFIG_SRCDIR([ptb.c])
AC_CONFIG_HEADER([config.h])

# Checks for programs.
AC_PROG_CC
AC_PROG_INSTALL

AC_MINGW32
AC_EXEEXT
AC_OBJEXT

TARGET_BINS=""
TARGET_LIBS=""
POPT_LIBS=""
AC_SUBST(POPT_LIBS)

# Checks for libraries.
AC_CHECK_LIB([popt], [poptGetArg], [ 
	  POPT_LIBS="-lpopt"
	  TARGET_BINS="$TARGET_BINS ptbdict$EXEEXT ptb2ly$EXEEXT ptb2ascii$EXEEXT ptbinfo$EXEEXT gp2ly$EXEEXT ptb2abc$EXEEXT" 
	  ] , AC_MSG_WARN([Popt is required for command-line utilities]))
PKG_CHECK_MODULES(LIBXML, libxml-2.0, [
if test $ac_cv_lib_popt_poptGetArg = yes; then  
	TARGET_BINS="$TARGET_BINS ptb2xml$EXEEXT" 
fi
] , AC_MSG_WARN([libxml not found: not building ptb2xml]))
PKG_CHECK_MODULES(LIBXSLT, libxslt, AC_DEFINE(HAVE_XSLT, 1, [whether libxslt is available]) , AC_MSG_WARN([libxslt not found: ptb2xml will not support musicxml]))

PKG_CHECK_MODULES(CHECK, check, [], [ echo -n "" ])

if test "$MINGW32" = "yes"; then 
	TARGET_BINS="$TARGET_BINS ptb.dll" 
else
	TARGET_LIBS="$TARGET_LIBS libptb.so.$PACKAGE_VERSION libptb.a"
fi

AC_SUBST(DLLFLAGS)
AC_SUBST(TARGET_BINS)
AC_SUBST(TARGET_LIBS)

# Checks for header files.
AC_HEADER_STDC
AC_HEADER_TIME
AC_CHECK_HEADERS([stdlib.h string.h unistd.h popt.h sys/time.h ctype.h])

# Checks for typedefs, structures, and compiler characteristics.
AC_C_CONST
AC_TYPE_SIZE_T

AC_SUBST(SHFLAGS)
case $host in 
	*darwin*) SHFLAGS="-dynamiclib" ;;
	*) SHFLAGS="-shared -Wl,-soname,libptb.so.0" ;;
esac

AC_CONFIG_FILES([Makefile.settings ptabtools.spec ptabtools.pc])
AC_OUTPUT
