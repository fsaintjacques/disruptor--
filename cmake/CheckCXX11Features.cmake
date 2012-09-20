# Checks for C++11 features
#
# Tests whether the compiler supports C++11 features.
#
# The list of tested features is
#  AUTO               - auto keyword
#  RANGE_BASED_FOR    - range-based for loops
#  NULLPTR            - nullptr
#  LAMBDA             - lambdas
#  STATIC_ASSERT      - static_assert()
#  RVALUE_REFERENCES  - rvalue references
#  DECLTYPE           - decltype keyword
#  CSTDINT_H          - cstdint header
#  LONG_LONG          - long long signed & unsigned types
#  VARIADIC_TEMPLATES - variadic templates
#  CONSTEXPR          - constexpr keyword
#  SIZEOF_MEMBER      - sizeof() non-static members
#  FUNC               - __func__ preprocessor constant
#
# To make a feature mandatory, use the list variable CXX11_REQUIRED_FEATURES.
# For example:
#   set ( CXX11_REQUIRED_FEATURES RANGE_BASED_FOR CONSTEXPR )
#   include ( CheckCXX11Features.cmake )
# will raise an error if the tests for these features fail.
#
# For each feature, defines a corresponding var, according to whether 
# the compiler implements it or not:
#
#  CXX11_HAVE_<feature>
# or 
#  CXX11_HAVE_NOT_<feature>
#
# Cached vars:
#
#  CXX11_FEATURE_LIST   - a list containing the status of all features
#
# Utility vars:
#
#  CXX11_REQUIRED_FLAGS - a string containing the compiler options necessary
#                         use the c++11 features (eg -std=c++0x vs -std=c++11)
#  CXX11_FEATURE_FLAGS  - a string containing the defines of all features, 
#                         for use in compiler command lines
#  CXX11_FEATURE_CONFIG - a string containing the defines of all features, 
#                         for use in configuration headers
# 
# The prefix CXX11_HAVE can be changed: prior to including this module, 
# set the variable CXX11_FEATURE_PREFIX to the desired value. Eg.,
#   set(CXX11_FEATURE_PREFIX "CXX11_HAS")
#   include(CheckCXX11Features)
# will make the result variables hold the prefix CXX11_HAS_ instead
# of CXX_HAVE_.
#
# Adapted by Joao Paulo Magalhaes from the original script 
# by Rolf Eike Beer and Andreas Weis
#
#------------------------------------------------------------------------------

cmake_minimum_required(VERSION 2.8.3)

include(CheckCXXSourceCompiles)
include(CheckCXXSourceRuns)

macro(cxx11_check_feature 
  FEATURE_NAME 
  FEATURE_NUMBER 
  FEATURE_SUFFIX
  CODE # must run
  CODE_FAIL_RUN # must fail
  CODE_FAIL_COMPILE # must fail compilation
)
#  if (DEFINED ${RESULT_VAR})
#    return ()
#  endif ()
  
  if (${FEATURE_NUMBER})
    set(_LOG_NAME "${FEATURE_NAME} (N${FEATURE_NUMBER})")
  else ()
    set(_LOG_NAME "${FEATURE_NAME}")
  endif ()
  message(STATUS "Checking C++11 support for ${_LOG_NAME}")

  # check if given codes are empty
  if("${CODE_FAIL}" STREQUAL "")
    set(_HAVE_CODE_FAIL FALSE)
  else()
    set(_HAVE_CODE_FAIL TRUE)
  endif()
  if("${CODE_FAIL_COMPILE}" STREQUAL "")
    set(_HAVE_CODE_COMPILE_FAIL FALSE)
  else()
    set(_HAVE_CODE_COMPILE_FAIL TRUE)
  endif()
  
  set ( RESULT_VAR "${CXX11_FEATURE_PREFIX}_${FEATURE_SUFFIX}" )

  if (CROSS_COMPILING)
    check_cxx_source_compiles("${CODE}" ${RESULT_VAR})
    if (${RESULT_VAR} AND _HAVE_CODE_FAIL)
      check_cxx_source_compiles("${CODE_FAIL_RUN}" ${RESULT_VAR}_FAIL_RUN)
      if (${RESULT_VAR}_FAIL_RUN)
        set(${RESULT_VAR} FALSE)
      endif ()
    endif ()
  else (CROSS_COMPILING)
    check_cxx_source_runs("${CODE}" ${RESULT_VAR})
    if (${RESULT_VAR} AND _HAVE_CODE_FAIL)
      check_cxx_source_runs("${CODE_FAIL_RUN}" ${RESULT_VAR}_FAIL_RUN)
      if (${RESULT_VAR}_FAIL_RUN)
        set(${RESULT_VAR} FALSE)
      endif ()
    endif ()
  endif (CROSS_COMPILING)
  
  if (${RESULT_VAR} AND _HAVE_CODE_COMPILE_FAIL)
    check_cxx_source_compiles("${CODE_FAIL_COMPILE}" ${RESULT_VAR}_FAIL_COMPILE)
    if (${RESULT_VAR}_FAIL_COMPILE)
      set(${RESULT_VAR} FALSE)
    endif ()
  endif ()

  if (${RESULT_VAR})
    message(STATUS "Compiler support for C++11: ${_LOG_NAME} -- works")
    list(APPEND CXX11_FEATURE_LIST "${CXX11_FEATURE_PREFIX}_${FEATURE_SUFFIX}")
  else ()
    message(STATUS "Compiler support for C++11: ${_LOG_NAME} -- not supported")
    list(APPEND CXX11_FEATURE_LIST "${CXX11_FEATURE_PREFIX}_NOT_${FEATURE_SUFFIX}")

    if (CXX11_REQUIRED_FEATURES)
      list (FIND CXX11_REQUIRED_FEATURES "${FEATURE_SUFFIX}" pos)
      if (NOT ( pos EQUAL -1 ))
        message ( SEND_ERROR "${FEATURE_SUFFIX}: this feature is required. Choose a different compiler or use CMAKE_CXX_FLAGS to specify the necessary options to enable this feature." )
      endif ()
    endif ()
  endif (${RESULT_VAR})
  set(${RESULT_VAR} ${${RESULT_VAR}} CACHE STRING "Compiler support for C++11: ${_LOG_NAME}")
  
  message(STATUS "")

endmacro()

#---------------------------------------------------------------------

message(STATUS "")
message(STATUS "Checking C++11 features")

# determine the compiler being used & find the -std flag which it will require
if (CMAKE_COMPILER_IS_GNUCXX)

  # determine the version of the compiler being used
  execute_process(COMMAND ${CMAKE_CXX_COMPILER} -dumpversion OUTPUT_VARIABLE CXX_VERSION)
  message(STATUS "Compiler: ${CMAKE_CXX_COMPILER} ${CXX_VERSION}")
  
  if (CXX_VERSION VERSION_GREATER 4.7 OR CXX_VERSION VERSION_EQUAL 4.7)
    set(MODE_FLAG "-std=c++11")
  elseif (CXX_VERSION VERSION_GREATER 4.3 OR CXX_VERSION VERSION_EQUAL 4.3)
    set(MODE_FLAG "-std=c++0x")
  else ()
    message(FATAL_ERROR "C++11 needed. Therefore a gcc compiler with a version higher than 4.3 is needed.")   
  endif()

else(CMAKE_COMPILER_IS_GNUCXX)

  if ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang")
    # using Clang

    # TODO WHICH ONE IS IT?
    # [jppm@machine] $ clang++ --version
    # Ubuntu clang version 3.0-6ubuntu3 (tags/RELEASE_30/final) (based on LLVM 3.0)
    # Target: x86_64-pc-linux-gnu
    # Thread model: posix
    # [jppm@machine] $ clang++ -dumpversion
    # 4.2.1

    # determine the version of the compiler being used
    execute_process(COMMAND ${CMAKE_CXX_COMPILER} -dumpversion OUTPUT_VARIABLE CXX_VERSION)
    message(STATUS "Compiler: ${CMAKE_CXX_COMPILER} ${CXX_VERSION}")

    if (CXX_VERSION VERSION_GREATER 4.2 OR CXX_VERSION VERSION_EQUAL 4.2)
      set(MODE_FLAG "-std=c++0x")
    elseif ()
      set(MODE_FLAG "-std=c++0x")
    endif()

  elseif ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Intel")
    # determine the version of the compiler being used
    execute_process(COMMAND ${CMAKE_CXX_COMPILER} -dumpversion OUTPUT_VARIABLE CXX_VERSION)
    message(STATUS "Compiler: ${CMAKE_CXX_COMPILER} ${CXX_VERSION}")

    set(MODE_FLAG "-std=c++0x")
  elseif ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "MSVC")
    # using Visual Studio C++
    set(MODE_FLAG "") # none needed
  elseif ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU")
    # using GCC
    message ( FATAL_ERROR "should not get here" )
  endif ()
endif(CMAKE_COMPILER_IS_GNUCXX)

message(STATUS "C++11 flag: ${MODE_FLAG}")
set(CHECK_CXX11_OLD_REQUIRED_FLAGS ${CMAKE_REQUIRED_FLAGS})
set(CMAKE_REQUIRED_FLAGS "${MODE_FLAG}")


if (NOT CXX11_FEATURE_PREFIX)
  set(CXX11_FEATURE_PREFIX "CXX11_HAVE")
endif ()

#---------------------------------------------------------------------
cxx11_check_feature("auto"               2546 AUTO
# CODE
"
int main()
{
  auto i = 5;
  auto f = 3.14159f;
  auto d = 3.14159;
  bool ret = ((sizeof(f) < sizeof(d)) && (sizeof(i) == sizeof(int)));
  return ret ? 0 : 1;
}
"
# CODE_FAIL_RUN
""
# CODE_FAIL_COMPILE
""
)
#---------------------------------------------------------------------
cxx11_check_feature("range_based_for"    2930 RANGE_BASED_FOR
# CODE
"
int main()
{
  int array[5] = { 1, 2, 3, 4, 5 };
  for (int& x : array) x *= 2;
}
"
# CODE_FAIL_RUN
""
# CODE_FAIL_COMPILE
""
)
#---------------------------------------------------------------------
cxx11_check_feature("initializer_list"  2672 INITIALIZER_LISTS
# CODE
"
#include <vector>

template< typename Type >
Type
sum ( std::initializer_list< Type > vals )
{
  Type tmp{};
  for ( typename std::initializer_list< Type >::const_iterator it = vals.begin(); 
        it != vals.end(); ++it ) 
    tmp += *it;
  return tmp;
}

int main()
{
  // {} can now be used to initialize structures
  int values[] {1,2,3};
  std::vector< int > v {1,2,3,5,8,13,21};

  int i;    // undefined value
  int j{};  // j is initialized with 0
  int *p;   // p has undefined value
  int *q{}; // p is initialized with nullptr

  return sum( {1,-1,2,-2} );
}
"
# CODE_FAIL_RUN
""
# CODE_FAIL_COMPILE
""
)
#---------------------------------------------------------------------
cxx11_check_feature("nullptr"            2431 NULLPTR
# CODE
"
int main()
{
  int* test = nullptr;
  return test ? 1 : 0;
}
"
# CODE_FAIL_RUN
""
# CODE_FAIL_COMPILE
"
int main()
{
  int i = nullptr;
  return 1;
}
"
)
#---------------------------------------------------------------------
cxx11_check_feature("lambda"             2927 LAMBDA
# CODE
"
int main()
{
  int ret = 0;
  return ([&ret]() -> int { return ret; })();
}
"
# CODE_FAIL_RUN
""
# CODE_FAIL_COMPILE
""
)
#---------------------------------------------------------------------
cxx11_check_feature("static_assert"      1720 STATIC_ASSERT
# CODE
"
int main()
{
  static_assert(0 < 1, \"your ordering of integers is screwed\");
  return 0;
}
"
# CODE_FAIL_RUN
""
# CODE_FAIL_COMPILE
"
int main()
{
  static_assert(1 < 0, \"this should fail\");
  return 0;
}
"
)
#---------------------------------------------------------------------
cxx11_check_feature("rvalue_references"  2118 RVALUE_REFERENCES
# CODE
"
int foo(int& lvalue) { return 123; }
int foo(int&& rvalue) { return 321; }
int main()
{
  int i = 42;
  return ((foo(i) == 123) && (foo(42) == 321)) ? 0 : 1;
}
"
# CODE_FAIL_RUN
""
# CODE_FAIL_COMPILE
""
)
#---------------------------------------------------------------------
cxx11_check_feature("decltype"           2343 DECLTYPE
# CODE
"
bool check_size(int i) { return sizeof(int) == sizeof(decltype(i)); }
int main()
{
  bool ret = check_size(42);
  return ret ? 0 : 1;
}

"
# CODE_FAIL_RUN
""
# CODE_FAIL_COMPILE
""
)
#---------------------------------------------------------------------
cxx11_check_feature("cstdint"            "" CSTDINT_H
# CODE
"
#include <cstdint>
int main()
{
  bool test;
  test = (sizeof(int8_t) == 1) 
         &&
         (sizeof(int16_t) == 2) 
         &&
         (sizeof(int32_t) == 4) 
         &&
         (sizeof(int64_t) == 8);
  return test ? 0 : 1;
}
"
# CODE_FAIL_RUN
""
# CODE_FAIL_COMPILE
""
)
#---------------------------------------------------------------------
cxx11_check_feature("long_long"          1811 LONG_LONG
# CODE
"
int main(void)
{
  long long l;
  unsigned long long ul;
  return ((sizeof(l) >= 8) && (sizeof(ul) >= 8)) ? 0 : 1;
}
"
# CODE_FAIL_RUN
""
# CODE_FAIL_COMPILE
""
)
#---------------------------------------------------------------------
cxx11_check_feature("variadic_templates" 2555 VARIADIC_TEMPLATES
# CODE
"
int Accumulate() { return 0; }

template<typename T, typename... Ts>
int Accumulate(T v, Ts... vs)
{
  return v + Accumulate(vs...);
}

template<int... Is>
int CountElements() { return sizeof...(Is); }

int main()
{
  int acc = Accumulate(1, 2, 3, 4, -5);
  int count = CountElements<1,2,3,4,5>();
  return ((acc == 5) && (count == 5)) ? 0 : 1;
}
"
# CODE_FAIL_RUN
""
# CODE_FAIL_COMPILE
""
)
#---------------------------------------------------------------------
cxx11_check_feature("constexpr"          2235 CONSTEXPR
# CODE
"
constexpr int square(int x) { return x*x; }
constexpr int the_answer() { return 42; }
int main()
{
  int test_arr[square(3)];
  bool ret = (
    (square(the_answer()) == 1764) 
    && 
    (sizeof(test_arr)/sizeof(test_arr[0]) == 9)
  );
  return ret ? 0 : 1;
}
"
# CODE_FAIL_RUN
""
# CODE_FAIL_COMPILE
""
)
#---------------------------------------------------------------------
cxx11_check_feature("sizeof_member"      2253 SIZEOF_MEMBER
# CODE
"
struct foo {
  char bar;
  int baz;
};

int main(void)
{
  bool ret = (
               (sizeof(foo::bar) == 1) 
               &&
               (sizeof(foo::baz) >= sizeof(foo::bar)) 
               &&
               (sizeof(foo) >= sizeof(foo::bar)+sizeof(foo::baz))
             );
  return ret ? 0 : 1;
}
"
# CODE_FAIL_RUN
""
# CODE_FAIL_COMPILE
""
)
#---------------------------------------------------------------------
cxx11_check_feature("__func__"           2340 FUNC
# CODE
"
#include <cstring>
int main()
{
  if (!__func__) { return 1; }
  if(std::strlen(__func__) <= 0) { return 1; }
  return 0;
}
"
# CODE_FAIL_RUN
""
# CODE_FAIL_COMPILE
""
)
#---------------------------------------------------------------------

set(CXX11_REQUIRED_FLAGS "${CMAKE_REQUIRED_FLAGS}" )
set(CXX11_FEATURE_FLAGS "")
set(CXX11_FEATURE_CONFIG "")
foreach(flag ${CXX11_FEATURE_LIST})
  set(CXX11_FEATURE_FLAGS "${CXX11_FEATURE_FLAGS} -D${flag}")
  set(CXX11_FEATURE_CONFIG "${CXX11_FEATURE_CONFIG}
#define ${flag}")
endforeach()

set(CMAKE_REQUIRED_FLAGS ${CHECK_CXX11_OLD_REQUIRED_FLAGS})
unset(CHECK_CXX11_OLD_REQUIRED_FLAGS)

set(CXX11_FEATURE_LIST ${CXX11_FEATURE_LIST} CACHE STRING "C++11 feature support list")
MARK_AS_ADVANCED(FORCE CXX11_FEATURE_LIST)


