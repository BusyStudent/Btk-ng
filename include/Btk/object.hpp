#pragma once

#include <Btk/signal/trackable.hpp>
#include <Btk/defs.hpp>
#include <type_traits>

#define BTK_EXPOSE_SIGNAL(name) \
    auto &signal##name() { \
        return name;\
    }

BTK_NS_BEGIN

class ObjectImpl;
class TimerEvent;

// Event filter, return true to discard event
using EventFilter = bool (*)(Object *, Event &, void *);

/**
 * @brief All objects has virtual destructor and slots should inherit from Object
 * 
 */
class BTKAPI Object : public Any, public Trackable {
    public:
        Object() = default;
        ~Object();

        // Handle Event
        virtual bool handle(Event &);

        // Get / Set value
        void set_userdata(const char_t *key, Any *value);
        Any     *userdata(const char_t *key) const;

        // Context
        context_t ui_context() const;

        // Timer management
        timerid_t add_timer(timertype_t type, uint32_t interval);
        timerid_t add_timer(uint32_t interval);
        bool      del_timer(timerid_t timerid);
        // Deferred call
        void      defer_delete();
        // Event filter
        void      add_event_filter(EventFilter filter, pointer_t data);
        void      del_event_filter(EventFilter filter, pointer_t data);
        bool      run_event_filter(Event &);

        // Timer Event
        virtual bool timer_event(TimerEvent &) { return false; }
    private:
        ObjectImpl *implment() const;

        mutable ObjectImpl *priv = nullptr;
};

class BTKAPI Timer : public Object {
    public:
        Timer();
        Timer(const Timer &) = delete;
        ~Timer();

        void set_interval(uint32_t interval);
        void set_repeat(bool repeat);
        void start();
        void stop();

        bool timer_event(TimerEvent &) override;

        BTK_EXPOSE_SIGNAL(_timeout);
    private:
        Signal<void()> _timeout;
        timertype_t    _type     = TimerType::Precise;
        timerid_t      _id       = 0;
        uint32_t       _interval = 0;
        bool           _repeat   = false;
};

// Inline functions
inline timerid_t Object::add_timer(uint32_t interval) {
    return add_timer(TimerType::Precise, interval);
}

BTK_NS_END