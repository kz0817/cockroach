lib_LTLIBRARIES = cockroach.la
cockroach_la_SOURCES = \
  cockroach.cc utils.cc mapped_lib_manager.cc mapped_lib_info.cc \
  probe.cc side_code_area_manager.cc disassembler.cc opecode.cc \
  opecode_relocator.cc rip_relative_relocator.cc \
  cockroach-time-measure.cc time_measure_probe.cc

cockroach_la_CXXFLAGS = -Wall -g
cockroach_la_LDFLAGS = \
  -ldl -lpthread -lrt \
  -avoid-version \
  -module -no-undefined \
  -export-dynamic

bin_PROGRAMS = cockroach-time-measure-tool
cockroach_time_measure_tool_SOURCES = \
  cockroach-time-measure-tool.cc \
  cockroach-time-measure.cc

cockroach_time_measure_tool_CXXFLAGS = -Wall -g
cockroach_time_measure_tool_LDFLAGS = \
  -lrt
