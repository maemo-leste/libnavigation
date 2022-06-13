MAINTAINERCLEANFILES 				= Makefile.in \
						  version.xml \
						  tmpl/*.sgml \
						  gtk-doc.make

DOC_MODULE					= libnavigation

DOC_MAIN_SGML_FILE				= $(DOC_MODULE)-docs.sgml


SCANOBJ_OPTIONS					=--type-init-func="gtk_type_init(0)"

DOC_SOURCE_DIR					= $(top_srcdir)/navigation
DOC_BUILD_DIR 					= $(top_builddir)/libnavigation


HFILE_GLOB					= $(top_srcdir)/navigation/*.h

CFILE_GLOB					= $(top_srcdir)/navigation/*.c

IGNORE_HFILES 					= navigation-provider-glue.h navigation-provider-client-glue.h

AM_CPPFLAGS 					= $(NAVIGATION_CFLAGS) -I$(top_srcdir)/navigation

MKDB_OPTIONS					=--sgml-mode --output-format=xml

GTKDOC_CFLAGS 					= $(NAVIGATION_CFLAGS)

GTKDOC_LIBS 					= $(NAVIGATION_LIBS) $(top_builddir)/navigation/libnavigation.la

version.xml: ../configure.ac
	@echo $(VERSION) > version.xml

html: html-build.stamp

FIXXREF_OPTIONS					=

include $(top_srcdir)/doc/gtk-doc.make

