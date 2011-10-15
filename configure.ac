AC_INIT([disruptor], [0.1], [fsaintjacques@gmail.com])
AC_CONFIG_SRCDIR([disruptor/claim_strategy.h])

# Checks for programs.
AC_PROG_CXX
AC_PROG_CC

# automake
AM_INIT_AUTOMAKE([1.10 -Wall no-define foreign])

# Checks for header files.
AC_CHECK_HEADERS([sys/time.h])

# Checks for typedefs, structures, and compiler characteristics.
AC_HEADER_STDBOOL
AC_C_CONST
AC_TYPE_INT64_T
AC_HEADER_TIME
AC_CXX_COMPILE_STDCXX_0X

# Checks for libraries.
AC_CHECK_LIB(stdc++, [main])
AC_CHECK_LIB(rt, [main])

# Checks for library functions.
AC_CHECK_FUNCS([gettimeofday])

AM_CXXFLAGS="$AM_CXXFLAGS -std=c++0x"
AM_LDFLAGS="$AM_LDFLAGS -pthread"
AC_SUBST([AM_CXXFLAGS])
AC_SUBST([AM_LDFLAGS])

AC_CONFIG_FILES([Makefile])
AC_CONFIG_FILES([perf/Makefile])
AC_CONFIG_FILES([test/Makefile])
AC_OUTPUT
