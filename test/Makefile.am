AM_CXXFLAGS = @AM_CXXFLAGS@ -I$(top_srcdir) -D_GLIBCXX_USE_SCHED_YIELD -D_GLIBCXX_USE_NANOSLEEP

AM_LDFLAGS = -lboost_unit_test_framework \
			 @AM_LDFLAGS@

check_PROGRAMS = sequencer_test ring_buffer_test

sequencer_test_SOURCES = sequencer_test.cc
ring_buffer_test_SOURCES = ring_buffer_test.cc

TESTS = ${check_PROGRAMS}
