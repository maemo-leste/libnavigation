lib_LTLIBRARIES = libnavigation.la
libnavigation_la_CFLAGS = -I$(top_srcdir) $(NAVIGATION_CFLAGS) \
		-DLIBDIR='"$(libdir)"' \
		-DISO_CODES_PREFIX='"$(ISO_CODES_PREFIX)"'
libnavigation_la_LDFLAGS = -Wl,--as-needed $(NAVIGATION_LIBS) \
		-Wl,--no-undefined
libnavigation_la_SOURCES = navigation-provider.c

libnavigation_includedir = $(includedir)/@PACKAGE_NAME@
libnavigation_include_HEADERS = navigation-provider-glue.h \
		navigation-provider-enums.h \
		navigation-provider.h

BUILT_SOURCES = navigation-provider-glue.h navigation-provider-client-glue.h

navigation-provider-glue.h: navigation-provider.xml
	$(DBUS_BINDING_TOOL) --prefix=navigation \
		--mode=glib-server $< > xgen-$(@F) \
	&& ( cmp -s xgen-$(@F) $@ || cp xgen-$(@F) $@ ) \
	&& rm -f xgen-$(@F)

navigation-provider-client-glue.h: navigation-provider.xml
	$(DBUS_BINDING_TOOL) --prefix=navigation \
		--mode=glib-client $< > xgen-$(@F) \
	&& ( cmp -s xgen-$(@F) $@ || cp xgen-$(@F) $@ ) \
	&& rm -f xgen-$(@F)

xmldir = $(docdir)
xml_DATA = navigation-provider.xml

schemasdir = $(GCONF_SCHEMA_FILE_DIR)
schemas_DATA = libnavigation.schemas

CLEANFILES = $(BUILT_SOURCES)

MAINTAINERCLEANFILES = Makefile.in
