AM_CXXFLAGS = @AM_CXXFLAGS@ -I$(top_srcdir) -D_GLIBCXX_USE_SCHED_YIELD -D_GLIBCXX_USE_NANOSLEEP

check_PROGRAMS = perf-1P-1EP
perf_1P_1EP_SOURCES = one_publisher_to_one_unicast_throughput_test.cc

TESTS = ${check_PROGRAMS}
