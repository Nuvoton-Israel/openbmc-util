bin_PROGRAMS = xfer_mbox_mem

xfer_mbox_mem_SOURCES = main.cpp
xfer_mbox_mem_LDADD = libupdater.la $(CODE_COVERAGE_LIBS)
xfer_mbox_mem_CXXFLAGS = \
	-I$(top_srcdir) \
	$(CODE_COVERAGE_CXXFLAGS)

noinst_LTLIBRARIES = libupdater.la
libupdater_la_LDFLAGS = -static $(CODE_COVERAGE_LIBS) $(PCILIB_LIBS)
libupdater_la_CXXFLAGS = \
	-I$(top_srcdir) \
	$(CODE_COVERAGE_CXXFLAGS) \
	$(PCILIB_CFLAGS)
libupdater_la_SOURCES = \
	updater.cpp \
	handler.cpp \
	io.cpp \
	pci.cpp \
	p2a.cpp \
	progress.cpp \
	ipmi_handler.cpp
libupdater_la_LIBADD = $(top_builddir)/libfirmware_common.la
