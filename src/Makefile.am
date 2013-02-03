AM_CXXFLAGS = -Wall -g

#
# lib
#
lib_LTLIBRARIES = libcockroach.la cockroach.la

# libcockroach.so
libcockroach_la_SOURCES = \
  utils.cc mapped_lib_manager.cc mapped_lib_info.cc \
  probe.cc side_code_area_manager.cc disassembler.cc opecode.cc \
  opecode_relocator.cc rip_relative_relocator.cc \
  cockroach-time-measure.cc time_measure_probe.cc data_on_shm.cc
libcockroach_la_LDFLAGS = -lrt -dl -lpthread

# cockroach.so
cockroach_la_SOURCES = cockroach-instance.cc cockroach.cc
cockroach_la_LDFLAGS = \
  -lcockroach \
  -ldl \
  -avoid-version \
  -module -no-undefined \
  -export-dynamic

#
# bin
#
bin_PROGRAMS = cockroach-time-measure-tool cockroach-record-data-tool

# cockroach-time-measure-tool
cockroach_time_measure_tool_SOURCES = cockroach-time-measure-tool.cc
cockroach_time_measure_tool_LDFLAGS = -lcockroach -lrt -ldl

# cockroach-record-data-tool
cockroach_record_data_tool_SOURCES = cockroach-record-data-tool.cc
cockroach_record_data_tool_LDFLAGS = -lcockroach -lrt -ldl
