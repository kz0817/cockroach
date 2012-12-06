#SUBDIRS = fixtures

EXTRA_DIST = run-test.sh

if WITH_CUTTER
TESTS = run-test.sh
TESTS_ENVIRONMENT = NO_MAKE=yes CUTTER="$(CUTTER)"

bin_PROGRAMS = helper-bin

noinst_LTLIBRARIES = \
test-measure-time.la \
libtargets.la

endif

AM_CFLAGS = -g -Wall $(GLIB_CFLAGS) $(CUTTER_CFLAGS)
AM_CXXFLAGS = $(AM_CFLAGS)
AM_LDFLAGS = -module -rpath $(libdir) -avoid-version -no-undefined

libtargets_la_SOURCES = targets.c
libtargets_la_LDFLAGS = -rpath $(libdir)

helper_bin_SOURCES = helper-bin.c
helper_bin_LDADD = ./libtargets.la

test_measure_time_la_SOURCES = test-measure-time.cc

echo-cutter:
	@echo $(CUTTER)
