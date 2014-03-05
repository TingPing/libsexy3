AM_CPPFLAGS = -I$(top_srcdir) $(PACKAGE_CFLAGS)

sexyincdir = $(includedir)/libsexy3
sexy_headers = sexy.h sexy-spell-entry.h
sexyinc_HEADERS = $(sexy_headers)
noinst_HEADERS = gtkspell-iso-codes.h sexy-marshal.h

lib_LTLIBRARIES = libsexy3.la
libsexy3_la_CPPFLAGS = $(AM_CPPFLAGS)
libsexy3_la_SOURCES = gtkspell-iso-codes.c sexy-spell-entry.c sexy-marshal.c
libsexy3_la_LIBADD = $(PACKAGE_LIBS)
libsexy3_la_LDFLAGS = -version-info $(LT_CURRENT):$(LT_REVISION):$(LT_AGE) $(NO_UNDEFINED)

include $(INTROSPECTION_MAKEFILE)
if HAVE_INTROSPECTION
Sexy-3.0.gir: libsexy3.la
Sexy_3_0_gir_INCLUDES = Gtk-3.0
Sexy_3_0_gir_CFLAGS = $(AM_CPPFLAGS)
Sexy_3_0_gir_LIBS = libsexy3.la
Sexy_3_0_gir_FILES = sexy-spell-entry.c sexy-spell-entry.h
Sexy_3_0_gir_SCANNERFLAGS = --warn-all
Sexy_3_0_gir_METADATADIRS = .
INTROSPECTION_GIRS = Sexy-3.0.gir

girdir = $(datadir)/gir-1.0
gir_DATA = $(INTROSPECTION_GIRS)

typelibsdir = $(libdir)/girepository-1.0
typelibs_DATA = $(INTROSPECTION_GIRS:.gir=.typelib)
endif

include $(VAPIGEN_MAKEFILE)
if ENABLE_VAPIGEN
libsexy3.vapi: Sexy-3.0.gir libsexy3.deps
libsexy3_vapi_DEPS = gtk+-3.0
libsexy3_vapi_FILES = Sexy-3.0.gir
libsexy3_vapi_METADATADIRS = .
VAPIGEN_VAPIS = libsexy3.vapi

vapidir = $(datadir)/vala/vapi
vapi_DATA = $(VAPIGEN_VAPIS) $(VAPIGEN_VAPIS:.vapi=.deps)
endif

libsexy3.deps:
	$(AM_V_GEN) for pkg in $(libsexy3_vapi_DEPS); do \
		echo $$pkg >> $@; \
	done

sexy-marshal.h:	marshal.list
	$(AM_V_GEN) $(GLIB_GENMARSHAL) --prefix=sexy_marshal ./marshal.list --header > sexy-marshal.h

sexy-marshal.c:	marshal.list
	$(AM_V_GEN) $(GLIB_GENMARSHAL) --prefix=sexy_marshal ./marshal.list --body > sexy-marshal.c

BUILT_SOURCES = \
	sexy-marshal.h \
	sexy-marshal.c

EXTRA_DIST = \
	marshal.list \
	Sexy-3.0.metadata

CLEANFILES = \
	$(BUILT_SOURCES) \
	$(gir_DATA) \
	$(typelibs_DATA) \
	$(vapi_DATA)
