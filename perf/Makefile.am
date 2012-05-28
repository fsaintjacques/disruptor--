AM_CXXFLAGS = @AM_CXXFLAGS@ -I$(top_srcdir)

check_PROGRAMS = perf-1P-1EP-UNICAST perf-1P-3EP-PIPELINE

perf_1P_1EP_UNICAST_SOURCES = one_publisher_to_one_unicast_throughput_test.cc
perf_1P_3EP_PIPELINE_SOURCES = one_publisher_to_three_pipeline_throughput_test.cc

TESTS = ${check_PROGRAMS}
