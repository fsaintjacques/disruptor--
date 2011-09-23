// Copyright 2011 <François Saint-Jacques>

#include <exception>

#ifndef DISRUPTOR_EXCEPTIONS_H_  // NOLINT
#define DISRUPTOR_EXCEPTIONS_H_  // NOLINT

namespace disruptor {

class AlertException : public std::exception {
};

};  // namespace disruptor

#endif // DISRUPTOR_EXCEPTIONS_H_  NOLINT
