#pragma once

#include <Btk/signal/trackable.hpp>
#include <Btk/detail/wrapper.hpp>
#include <Btk/defs.hpp>
#include <type_traits>

#define BTK_EXPOSE_SIGNAL(name) \
    auto &signal##name() { \
        return name;\
    }

BTK_NS_BEGIN

class ObjectImpl;
class TimerEvent;

// Maybe we use FilterResult to replace EventFilter return value (bool)
class FilterResult {
    public:
        static constexpr bool Discard = true;
        static constexpr bool Keep    = false;
    private:
        FilterResult() = default;
};

// Event filter, return true to discard event
using EventFilter   = bool (*)(Object *, Event &, void *);
using DeferRoutinue = void (*)(void *);

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
        bool      ui_thread()  const;

        // Timer management
        timerid_t add_timer(timertype_t type, uint32_t interval);
        timerid_t add_timer(uint32_t interval);
        bool      del_timer(timerid_t timerid);
        // Deferred call
        template <typename Callable, typename ...Args>
        void      defer_call(Callable    &&cb, Args ...args);
        void      defer_call(DeferRoutinue rt, pointer_t data);
        void      defer_delete();
        // Event filter
        void      add_event_filter(EventFilter filter, pointer_t data);
        void      del_event_filter(EventFilter filter, pointer_t data);
        bool      run_event_filter(Event &);

        // Timer Event
        virtual bool timer_event(TimerEvent &) { return false; }

        // Signals
        Signal<void()> &signal_destoryed();
    private:
        ObjectImpl *implment() const;
        std::shared_ptr<bool> mark() const;

        mutable ObjectImpl *priv = nullptr;
};

/**
 * @brief Timer
 * 
 */
class BTKAPI Timer : public Object {
    public:
        Timer();
        Timer(const Timer &) = delete;
        ~Timer();

        /**
         * @brief Set the interval object
         * 
         * @param interval The interval of timer
         */
        void set_interval(uint32_t interval);
        /**
         * @brief Set the type object
         * 
         * @param type 
         */
        void set_type(timertype_t type);
        /**
         * @brief Set the repeat object
         * 
         * @param repeat true if you want the timer emit the signal until you stop it
         */
        void set_repeat(bool repeat);
        /**
         * @brief Start the timer
         * 
         */
        void start();
        /**
         * @brief Stop the timer
         * 
         */
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

/**
 * @brief Manager for manage call in event queue (It can cancel it)
 * 
 */
class BTKAPI TaskManager {
    public:
        /**
         * @brief Construct a new Defer Manager object
         * 
         * @param dispatcher The dispatcher pointer (nullptr on UIContext's dispatcher)
         */
        TaskManager(EventDispatcher *dispatcher = nullptr);
        TaskManager(const TaskManager &) = delete;
        ~TaskManager();

        /**
         * @brief Cancel all task in queue
         * 
         */
        void cancel_task();

        /**
         * @brief Add a task in the queue, waiting for dispatch
         * 
         * @tparam Callable 
         * @tparam Args 
         * @param callable 
         * @param args 
         */
        template <typename Callable, typename ...Args>
        void add_task(Callable &&callable, Args ...args);

        /**
         * @brief Add a task to call the object's method, 
         * 
         * @tparam IClass 
         * @tparam RetT 
         * @tparam CArgs 
         * @tparam Class 
         * @tparam Args 
         * @tparam std::enable_if_t<
         * std::is_base_of_v<Object, IClass>
         * > 
         * @param method 
         * @param obj 
         * @param args 
         */
        template <
            typename IClass, typename RetT, typename ...CArgs, 
            typename Class, typename ...Args,
            typename _Cond = std::enable_if_t<
                std::is_base_of_v<Object, IClass>
            >
        >
        void add_task(RetT (IClass::*method)(CArgs...), Class *obj, Args...args);

        /**
         * @brief Add a task to emit this signal
         * 
         * @tparam Ret 
         * @tparam Args 
         * @param signal 
         * @param args 
         */
        template <typename Ret, typename ...Args>
        void emit_signal(Signal<Ret (Args...)> &signal, Args ...args);

    private:
        void send(void (*fn)(void *), void *);

        // When mark on false means canceled
        std::shared_ptr<bool> mark { std::make_shared<bool>(true) };
        EventDispatcher *dispatcher;
};

// Inline functions
inline timerid_t Object::add_timer(uint32_t interval) {
    return add_timer(TimerType::Precise, interval);
}

template <typename Callable, typename ...Args>
inline void      Object::defer_call(Callable &&callable, Args ...args) {
    using Wrapper = CancelableWrapper<Callable, Args...>;

    Wrapper *wp = new Wrapper(
        mark(), //< Get Cancel mark
        std::forward<Callable>(callable),
        std::forward<Args>(args)...
    );

    DeferRoutinue rt = Wrapper::Call;
    pointer_t     pr = wp;

    defer_call(rt, pr);
}

template <typename Callable, typename ...Args>
inline void      TaskManager::add_task(Callable &&callable, Args ...args) {
    using Wrapper = CancelableWrapper<Callable, Args...>;

    Wrapper *wp = new Wrapper(
        mark,
        std::forward<Callable>(callable),
        std::forward<Args>(args)...
    );

    DeferRoutinue rt = Wrapper::Call;
    pointer_t     pr = wp;

    send(rt, pr);
}

template <
    typename IClass, typename RetT, typename ...CArgs, 
    typename Class, typename ...Args,
    typename _Cond
>
inline void      TaskManager::add_task(RetT (IClass::*method)(CArgs...), Class *obj, Args...args) {
    // using Callable = RetT (IClass::*)(CArgs...);
    // using Wrapper = CancelableWrapper<Callable, Class*, Args...>;

    // Make a wrapper and send it to object's defer_call
    obj->defer_call([=, m = mark]() {
        if (*m) {
            std::invoke(method, obj, std::forward<Args>(args)...);
        }
        else {
            BTK_LOG("Task was canceled %s\n", BTK_FUNCTION);
        }
    });
}

template <typename Ret, typename ...Args>
inline void      TaskManager::emit_signal(Signal<Ret (Args...)> &signal, Args ...args) {
    if (signal.empty()) {
        return;
    }
    add_task(&Signal<Ret (Args...)>::emit, &signal, std::forward<Args>(args)...);
}

BTK_NS_END