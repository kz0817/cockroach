SUBDIRS = . fixtures

EXTRA_DIST = run-test.sh

if WITH_CUTTER
TESTS = run-test.sh
TESTS_ENVIRONMENT = NO_MAKE=yes CUTTER="$(CUTTER)"

bin_PROGRAMS = helper-bin

noinst_LTLIBRARIES = \
test-measure-time.la \
test-user-probe.la \
libtargets.la libtestutil.la \
user_probe.la

endif

AM_CFLAGS = -g -Wall $(GLIB_CFLAGS) $(CUTTER_CFLAGS)
AM_CXXFLAGS = $(AM_CFLAGS)
AM_LDFLAGS = -module -rpath $(libdir) -avoid-version -no-undefined
LIBS = $(CUTTER_LIBS) $(CPPCUTTER_LIBS)

NORM_LIB_LDFLAGS = -rpath $(libdir)

# Testees
libtargets_la_SOURCES = targets.c
libtargets_la_LDFLAGS = $(NORM_LIB_LDFLAGS)

helper_bin_SOURCES = helper-bin.c
helper_bin_LDFLAGS =
helper_bin_LDADD = ./libtargets.la

# Testers
libtestutil_la_SOURCES = testutil.cc
libtestutil_la_LDFLAGS = $(NORM_LIB_LDFLAGS) -lboost_thread-mt

test_measure_time_la_SOURCES = test-measure-time.cc
test_measure_time_la_LIBADD = ./libtestutil.la

test_user_probe_la_SOURCES = test-user-probe.cc
test_user_probe_la_LIBADD = ./libtestutil.la

# User probe
user_probe_la_SOURCES = user-probe.cc
user_probe_la_CXXFLAGS = $(AM_CXXFLAGS) -I../src

echo-cutter:
	@echo $(CUTTER)
