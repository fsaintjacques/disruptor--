// Copyright 2011 <François Saint-Jacques>

#include <exception>

#include "disruptor/interface.h"

#ifndef DISRUPTOR_EXCEPTION_HANDLER_H_  // NOLINT
#define DISRUPTOR_EXCEPTION_HANDLER_H_  // NOLINT

namespace disruptor {

template<typename T>
class IgnoreExceptionHandler: public ExceptionHandlerInterface<T> {
 public:
    virtual void Handle(const std::exception& exception,
                         const int64_t& sequence,
                         T* event) {
        // do nothing with the exception.
        ;
    }
};

template<typename T>
class FatalExceptionHandler: public ExceptionHandlerInterface<T> {
 public:
    virtual void Handle(const std::exception& exception,
                         const int64_t& sequence,
                         T* event) {
        // rethrow the exception
        throw exception;
    }
};

};  // namespace disruptor

#endif // DISRUPTOR_EXCEPTION_HANDLER_H_  NOLINT
