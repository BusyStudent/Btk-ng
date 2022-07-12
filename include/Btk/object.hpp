#pragma once

#include <Btk/signal/trackable.hpp>
#include <Btk/defs.hpp>
#include <type_traits>

BTK_NS_BEGIN

class ObjectImpl;
class TimerEvent;

template <typename T>
class ValueHolder : public Any {
    public:
        ValueHolder(T value) : value(value) {}
        T value;
};

class BTKAPI Object : public Any, public Trackable {
    public:
        Object() = default;
        ~Object();

        // Handle Event
        virtual bool handle(Event &);

        // Get / Set value
        void set_userdata(const char_t *key, Any *value);
        Any     *userdata(const char_t *key) const;

        template <typename T>
        void set_userdata(const char_t *key, T && value) {
            if constexpr(std::is_pointer_v<T>) {
                set_userdata(key, new ValueHolder<void*>(value));
            }
            else {
                set_userdata(key, new ValueHolder<T>(value));
            }
        }

        template <typename T>
        T  *userdata(const char_t *key) const {
            Any *value = userdata(key);
            if (value) {
                return dynamic_cast<ValueHolder<T>*>(value)->value;
            }
            return nullptr;
        }
        // Context
        context_t ui_context() const;

        // Timer management
        timerid_t add_timer(uint32_t interval);
        bool      del_timer(timerid_t timerid);
        // Deferred call
        void      defer_delete();

        // Timer Event
        virtual bool timer_event(TimerEvent &) { return false; }
    private:
        ObjectImpl *implment() const;

        mutable ObjectImpl *priv = nullptr;
};

class Timer : public Object {
    
};

BTK_NS_END