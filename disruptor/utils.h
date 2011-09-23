// Copyright 2011 <François Saint-Jacques>

#ifndef DISRUPTOR_UTILS_H_ // NOLINT
#define DISRUPTOR_UTILS_H_ // NOLINT

// From Google C++ Standard
// A macro to disallow the copy constructor and operator= functions
// This should be used in the private: declarations for a class
#define DISALLOW_COPY_AND_ASSIGN(TypeName) \
  TypeName(const TypeName&);               \
  void operator=(const TypeName&)

#endif // DISRUPTOR_UTILS_H_ NOLINT
