lib_LTLIBRARIES = cockroach.la
cockroach_la_SOURCES = \
  cockroach.cc utils.cc mapped_lib_manager.cc mapped_lib_info.cc \
  probe.cc side_code_area_manager.cc disassembler.cc opecode.cc \
  opecode_relocator.cc rip_relative_relocator.cc \
  cockroach-time-measure.cc time_measure_probe.cc data_on_shm.cc

cockroach_la_CXXFLAGS = -Wall -g
cockroach_la_LDFLAGS = \
  -ldl -lpthread -lrt \
  -avoid-version \
  -module -no-undefined \
  -export-dynamic

bin_PROGRAMS = cockroach-time-measure-tool cockroach-record-data-tool
cockroach_time_measure_tool_SOURCES = \
  cockroach-time-measure-tool.cc \
  cockroach-time-measure.cc
cockroach_time_measure_tool_CXXFLAGS = -Wall -g
cockroach_time_measure_tool_LDFLAGS = -lrt

cockroach_record_data_tool_SOURCES = \
  cockroach-record-data-tool.cc data_on_shm.cc utils.cc
cockroach_record_data_tool_CXXFLAGS = -Wall -g
cockroach_record_data_tool_LDFLAGS = -lrt -ldl
