
AC_PREREQ(2.60)
AC_INIT(libsexy3, 1.0.0)
AC_COPYRIGHT([Copyright 2005-2006 Christian Hammond])
AC_CONFIG_HEADERS([config.h])
AC_CONFIG_MACRO_DIR(m4)
AM_INIT_AUTOMAKE([1.12 tar-pax dist-xz no-dist-gzip subdir-objects no-define foreign -Wall])
AM_SILENT_RULES([yes])

AX_IS_RELEASE([git-directory])
AX_CHECK_ENABLE_DEBUG([yes], [G_ENABLE_DEBUG], [G_DISABLE_ASSERT G_DISABLE_CHECKS G_DISABLE_CAST_CHECKS])

dnl #************************* platform ******************************
AC_CANONICAL_HOST

platform_win32=no
case $host_os in
	*mingw*|*cygwin*|*msys*)
		platform_win32=yes;;
	*);;
esac

if test "$platform_win32" = yes; then
	NO_UNDEFINED="-no-undefined"
	AC_SUBST(NO_UNDEFINED)
fi

dnl #************************ versioning *****************************
LIBSEXY_MAJOR_VERSION=1
LIBSEXY_MINOR_VERSION=0
LIBSEXY_MICRO_VERSION=0
LIBSEXY_VERSION=$LIBSEXY_MAJOR_VERSION.$LIBSEXY_MINOR_VERSION.$LIBSEXY_MICRO_VERSION

AC_DEFINE_UNQUOTED(LIBSEXY_MAJOR_VERSION, $LIBSEXY_MAJOR_VERSION,
	[libsexy major version.])
AC_DEFINE_UNQUOTED(LIBSEXY_MINOR_VERSION, $LIBSEXY_MINOR_VERSION,
	[libsexy minor version.])
AC_DEFINE_UNQUOTED(LIBSEXY_MICRO_VERSION, $LIBSEXY_MICRO_VERSION,
	[libsexy micro version.])
AC_DEFINE_UNQUOTED(LIBSEXY_VERSION, "$LIBSEXY_VERSION",
	[libsexy version.])

LT_CURRENT=1
LT_REVISION=0
LT_AGE=0
LT_RELEASE=$LIBSEXY_VERSION
AC_SUBST(LT_CURRENT)
AC_SUBST(LT_REVISION)
AC_SUBST(LT_AGE)
AC_SUBST(LT_RELEASE)

VERSION=$LIBSEXY_VERSION
PACKAGE=libsexy
GETTEXT_PACKAGE=$(PACKAGE)
AC_SUBST(GETTEXT_PACKAGE)
AC_DEFINE_UNQUOTED(GETTEXT_PACKAGE,"$GETTEXT_PACKAGE",[Gettext package name])

dnl #*************************** programs/libs *******************************
AC_PROG_CC
AC_PROG_CC_C99
AM_PROG_AR
AC_PROG_INSTALL
AC_SYS_LARGEFILE

LT_PREREQ([2.2.0])
LT_INIT

IT_PROG_INTLTOOL([0.40.0])
AM_GLIB_GNU_GETTEXT

GTK_DOC_CHECK([1.4], [--flavour no-tmpl])
GOBJECT_INTROSPECTION_CHECK([1.30.0])
VAPIGEN_CHECK([0.18])
AC_PATH_PROG(GLIB_GENMARSHAL, glib-genmarshal, no)

PKG_PROG_PKG_CONFIG([0.28])
PKG_CHECK_MODULES(PACKAGE, [glib-2.0 >= 2.44 gtk+-3.0 enchant])

AC_DEFINE([GLIB_VERSION_MIN_REQUIRED], [GLIB_VERSION_2_44], [Dont warn using older APIs])
AC_DEFINE([GLIB_VERSION_MAX_ALLOWED], [GLIB_VERSION_2_44], [Prevents using newer APIs])

dnl #***************************** iso-codes **********************************
AC_MSG_CHECKING([for iso-codes package])
PKG_CHECK_EXISTS(iso-codes, [
	AC_MSG_RESULT([yes])

	AC_MSG_CHECKING([whether iso-codes has iso-639 and iso-3166 domains])
	AS_IF([`$PKG_CONFIG --variable=domains iso-codes | grep -q 639` && \
	       `$PKG_CONFIG --variable=domains iso-codes | grep -q 3166`], [
		AC_DEFINE_UNQUOTED([ISO_CODES_PREFIX],["`$PKG_CONFIG --variable=prefix iso-codes`"],[ISO codes prefix])
		AC_DEFINE_UNQUOTED([ISO_CODES_LOCALEDIR],["$ISO_CODES_PREFIX/share/locale"],[ISO codes locale dir])
		AC_DEFINE([HAVE_ISO_CODES],[1],[iso-codes available])
		AC_MSG_RESULT([yes])
	],[
		AC_MSG_RESULT([no])
		AC_MSG_ERROR([iso-638 or iso-3166 not found])
	])
],[
	AC_MSG_RESULT([no])
	AC_MSG_ERROR([iso-codes not found])
])

AX_APPEND_COMPILE_FLAGS([ \
	-std=gnu99 \
	-funsigned-char \
	-Wall \
	-Wextra \
	-Winline \
	-Wno-padded \
	-Wno-unused-parameter \
	-Wstrict-prototypes \
	-Wmissing-prototypes \
	-Werror=implicit-function-declaration \
	-Werror=pointer-arith \
	-Werror=init-self \
	-Werror=format-security \
	-Werror=format=2 \
	-Werror=missing-include-dirs \
	-Werror=date-time \
])


AC_CONFIG_FILES([
	Makefile
	libsexy/Makefile
	data/libsexy3.pc
	data/Makefile
	docs/Makefile
	docs/reference/Makefile
	po/Makefile.in
])

AC_OUTPUT

dnl #********************************* Summary *********************************
echo
echo "Configuration summary for $PACKAGE v$VERSION:"
echo "  - Host:                  ${host}"
echo "  - Prefix:                ${prefix}"
echo "  - Debug:                 $enable_debug"
echo "  - GObject introspection: $enable_introspection"
echo "  - Vala bindings:         $enable_vala"
echo "  - Documentation:         $enable_gtk_doc"
echo
