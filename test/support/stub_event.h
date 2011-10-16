// Copyright 2011 <François Saint-Jacques>

#include <string>

#include "disruptor/interface.h"

#include "long_event.h"

#ifndef DISRUPTOR_TEST_STUB_EVENT_H_ // NOLINT
#define DISRUPTOR_TEST_STUB_EVENT_H_ // NOLINT


namespace disruptor {
namespace test {

class StubEvent : public LongEvent {
 public:
    StubEvent(const int64_t& value = 0) : LongEvent(value) {}

    std::string test_string() const { return test_string_; }

    void set_test_string(const std::string& test_string) {
        test_string_ = test_string;
    }

 private:
    std::string test_string_;
};

class StubEventFactory : public EventFactoryInterface<StubEvent> {
 public:
    virtual StubEvent* NewInstance(const int& size) const {
        return new StubEvent[size];
    }
};

class StubBatchHandler : public EventHandlerInterface<StubEvent> {
 public:
    virtual void OnEvent(StubEvent* event, const int64_t& sequence, bool end_of_batch) {
        if (event)
            event->set_value(sequence);
    };

    virtual void OnStart() {}
    virtual void OnShutdown() {}
};

class StubEventTranslator : public EventTranslatorInterface<StubEvent> {
 public:
    virtual StubEvent* TranslateTo(StubEvent* event, const int64_t& sequence) {
        event->set_value(sequence);
        return event;
    };

};

}; // namespace test
}; // namespace disruptor

#endif // DISRUPTOR_TEST_LONG_EVENT_H_ NOLINT
