// Copyright 2011 <François Saint-Jacques>

#include "disruptor/interface.h"

#ifndef DISRUPTOR_TEST_LONG_EVENT_H_ // NOLINT
#define DISRUPTOR_TEST_LONG_EVENT_H_ // NOLINT


namespace disruptor {
namespace test {

class LongEvent {
 public:
    LongEvent(const int64_t& value = 0) : value_(value) {}

    int64_t value() const { return value_; }

    void set_value(const int64_t& value) { value_ = value; }

 private:
    int64_t value_;
};

class LongEventFactory : public EventFactoryInterface<LongEvent> {
 public:
    virtual LongEvent* NewInstance(const int& size) const {
        return new LongEvent[size];
    }
};

}; // namespace test
}; // namespace disruptor

#endif // DISRUPTOR_TEST_LONG_EVENT_H_ NOLINT
