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

dist_bin_SCRIPTS = \
    benchmark.py

EXTRA_DIST = \
    README

libbenchmark_la_LDFLAGS = -module -avoid-version

install-exec-hook:
	$(SST_REGISTER_TOOL) SST_BENCHMARK_SOURCE benchmark=$(abs_srcdir)
	$(SST_REGISTER_TOOL) SST_BENCHMARK_TESTS  benchmark=$(abs_srcdir)

##########################################################################
##########################################################################
##########################################################################


