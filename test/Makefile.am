SUBDIRS = . fixtures

EXTRA_DIST = run-test.sh

if WITH_CUTTER
TESTS = run-test.sh
TESTS_ENVIRONMENT = NO_MAKE=yes CUTTER="$(CUTTER)"

bin_PROGRAMS = target-exe

noinst_LTLIBRARIES = \
test-measure-time.la \
test-user-probe.la \
test-disassembler.la \
libtargets.la libtestutil.la \
user_probe.la \
libimplicitdlopener.la libimplicitopentarget.la

endif

AM_CFLAGS = -g -Wall $(GLIB_CFLAGS) $(CUTTER_CFLAGS) -I../src
AM_CXXFLAGS = $(AM_CFLAGS)
AM_LDFLAGS = -module -rpath $(libdir) -avoid-version -no-undefined
LIBS = $(CUTTER_LIBS) $(CPPCUTTER_LIBS)

NORM_LIB_LDFLAGS = -rpath $(libdir)

# Testers
test_disassembler_la_SOURCES = test-disassembler.cc
if I386
test_disassembler_la_SOURCES += test-disassembler-target-i386.S
endif
if X86_64
test_disassembler_la_SOURCES += test-disassembler-target-x86_64.S
endif

test_disassembler_la_LIBADD = ../src/libcockroach.la

# Testees
libtargets_la_SOURCES = target-func-lib.c
libtargets_la_LDFLAGS = $(NORM_LIB_LDFLAGS)

target_exe_SOURCES = target-exe.c
target_exe_LDFLAGS = -ldl
target_exe_LDADD = ./libtargets.la

libimplicitopentarget_la_SOURCES = implicitopentarget.c
libimplicitopentarget_la_LDFLAGS = $(NORM_LIB_LDFLAGS)

libimplicitdlopener_la_SOURCES = implicitdlopener.c
libimplicitdlopener_la_LIBADD = ./libimplicitopentarget.la
libimplicitdlopener_la_LDFLAGS = $(NORM_LIB_LDFLAGS)

# Testers
libtestutil_la_SOURCES = testutil.cc
libtestutil_la_LDFLAGS = $(NORM_LIB_LDFLAGS) -lboost_thread-mt

test_measure_time_la_SOURCES = test-measure-time.cc
test_measure_time_la_LIBADD = ./libtestutil.la

test_user_probe_la_SOURCES = test-user-probe.cc
test_user_probe_la_LIBADD = ./libtestutil.la

# User probe
user_probe_la_SOURCES = user-probe.cc
user_probe_la_CXXFLAGS = $(AM_CXXFLAGS)

echo-cutter:
	@echo $(CUTTER)
