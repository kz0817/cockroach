lib_LTLIBRARIES = cockroach.la
cockroach_la_SOURCES = \
  cockroach.cc utils.cc mapped_lib_manager.cc mapped_lib_info.cc probe_info.cc

cockroach_la_CXFLAGS = -Wall -g
cockroach_la_LDFLAGS = \
  -ldl \
  -avoid-version \
  -module -no-undefined \
  -export-dynamic
