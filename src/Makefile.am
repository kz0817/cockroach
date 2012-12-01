lib_LTLIBRARIES = cockroach.la
cockroach_la_SOURCES = \
  cockroach.cc utils.cc mapped_lib_manager.cc mapped_lib_info.cc \
  probe.cc side_code_area_manager.cc \
  time_measure_probe.cc

cockroach_la_CXFLAGS = -Wall -g
cockroach_la_LDFLAGS = \
  -ldl \
  -avoid-version \
  -module -no-undefined \
  -export-dynamic
