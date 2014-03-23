
AUTOMAKE_OPTIONS = 1.6

DOC_MODULE=libsexy3
DOC_MAIN_SGML_FILE=$(DOC_MODULE)-docs.sgml
DOC_SOURCE_DIR=../../libsexy

SCANGOBJ_OPTIONS=
SCAN_OPTIONS=--rebuild-types #--rebuild-sections 
MKDB_OPTIONS=--xml-mode --output-format=xml
MKTMPL_OPTIONS=
FIXXREF_OPTIONS=\
	--extra-dir=$(datadir)/gtk-doc/html/gobject \
	--extra-dir=$(datadir)/gtk-doc/html/glib


AM_CPPFLAGS = -I$(top_srcdir) -I$(top_builddir) $(PACKAGE_CFLAGS)
GTKDOC_LIBS = $(PACKAGE_LIBS) $(top_builddir)/libsexy/libsexy3.la
HFILE_GLOB=$(top_srcdir)/libsexy/*.h
CFILE_GLOB=$(top_srcdir)/libsexy/*.c
IGNORE_HFILES=\
	gtkspell-iso-codes.h \
	sexy-marshal.h \
	sexy.h

HTML_IMAGES=
content_files=
expand_content_files=

include $(top_srcdir)/gtk-doc.make
