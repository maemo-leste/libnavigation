AC_INIT([navigation], [0.106])
AC_CANONICAL_SYSTEM

AM_CONFIG_HEADER(config.h)

AM_MAINTAINER_MODE

AM_INIT_AUTOMAKE

AC_PROG_CC
AC_PROG_INSTALL
AM_PROG_LIBTOOL

AC_HEADER_STDC

AC_PATH_X
AC_PATH_XTRA
AC_SUBST(X_CFLAGS)

PKG_PROG_PKG_CONFIG

PKG_CHECK_MODULES(NAVIGATION, [dbus-glib-1 gtk+-2.0 gdk-pixbuf-2.0 gmodule-2.0 gconf-2.0 iso-codes libxml-2.0])

#+++++++++++++++
# Misc programs 
#+++++++++++++++

AC_PATH_PROG(DBUS_BINDING_TOOL, dbus-binding-tool)

#+++++++++++++++++++
# Directories setup
#+++++++++++++++++++

ISO_CODES_PREFIX=`pkg-config iso-codes --variable=prefix`
AC_SUBST(ISO_CODES_PREFIX)

#+++++++++++++++++++
# Misc stuff
#+++++++++++++++++++

AC_ARG_ENABLE(cast-checks,  [  --disable-cast-checks   compile with GLIB cast checks disabled],[cchecks=${enableval}],cchecks=yes)
if test "x$cchecks" = "xno"; then
    CFLAGS="$CFLAGS -DG_DISABLE_CAST_CHECKS"
fi

AC_ARG_ENABLE(asserts,      [  --disable-asserts       compile with GLIB assertions disabled],[asserts=${enableval}],asserts=yes)
if test "x$asserts" = "xno"; then
    CFLAGS="$CFLAGS -DG_DISABLE_ASSERTS"
fi

AC_ARG_ENABLE(checks,       [  --disable-checks        compile with GLIB checks disabled],[checks=${enableval}],checks=yes)
if test "x$checks" = "xno"; then
    CFLAGS="$CFLAGS -DG_DISABLE_CHECKS"
fi

AC_DEFINE_UNQUOTED([G_LOG_DOMAIN], "lib$PACKAGE_NAME", [Default logging facility])

AC_OUTPUT([
	Makefile
	navigation/Makefile
	navigation.pc
])

