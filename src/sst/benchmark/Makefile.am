# -*- Makefile -*-
#
#

AM_CPPFLAGS = \
	$(MPI_CPPFLAGS) \
	-I $(abs_top_srcdir)/src/

compdir = $(pkglibdir)
comp_LTLIBRARIES = libbenchmark.la
libbenchmark_la_SOURCES = \
	Worker.h \
	Worker.cc

EXTRA_DIST = \
    README \
    benchmark.py

libbenchmark_la_LDFLAGS = -module -avoid-version

install-exec-hook:
	$(SST_REGISTER_TOOL) SST_BENCHMARK_SOURCE benchmark=$(abs_srcdir)
	$(SST_REGISTER_TOOL) SST_BENCHMARK_TESTS  benchmark=$(abs_srcdir)

##########################################################################
##########################################################################
##########################################################################


