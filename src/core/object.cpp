#include "build.hpp"

#include <Btk/detail/platform.hpp>
#include <Btk/context.hpp>
#include <Btk/object.hpp>
#include <Btk/event.hpp>
#include <vector>
#include <set>

BTK_NS_BEGIN

struct UserData {
    UserData *next = nullptr;
    Any      *value = nullptr;
    u8string  key;
};
struct EventFilterNode {
    EventFilter filter;
    void       *userdata;
    EventFilterNode *next;
};
//< Object implementation
struct ObjectImpl {
    EventFilterNode *filters = nullptr;
    UserData *chain = nullptr;
    UIContext *ctxt = nullptr;

    std::set<timerid_t> timers;
    // Mark for Auto cancel call
    std::shared_ptr<bool> mark {std::make_shared<bool>(true)};
    
    ~ObjectImpl() {
        // Remove all userdata
        UserData *cur = chain;
        while (cur) {
            UserData *next = cur->next;
            delete cur->value;
            delete cur;
            cur = next;
        }
        // Remove all filters
        while (filters) {
            auto next = filters->next;
            delete filters;
            filters = next;
        }
    }
    void set_userdata(const char_t *key, Any *value) {
        UserData *cur = chain;
        while (cur) {
            if (cur->key == key) {
                if (value == nullptr) {
                    // Remove it
                    if (cur->next) {
                        cur->next->next = cur->next;
                    }
                    else {
                        chain = cur->next;
                    }
                    delete cur;
                }
                else{
                    // Just update
                    cur->value = value;
                }
                return;
            }
            cur = cur->next;
        }
        // Insert new
        UserData *new_data = new UserData;
        new_data->key = key;
        new_data->value = value;
        new_data->next = chain;
        chain = new_data;
    }
    Any *userdata(const char_t *key) const {
        UserData *cur = chain;
        while (cur) {
            if (cur->key == key) {
                return cur->value;
            }
            cur = cur->next;
        }
        return nullptr;
    }
};

Object::~Object() {
    // Delete timer if needed
    if (priv) {
        // Cancel mark
        *(priv->mark) = false;

        for (auto timerid : priv->timers) {
            priv->ctxt->dispatcher()->timer_del(this, timerid);
        }
    }

    delete priv;
}
void Object::set_userdata(const char_t *key, Any *value) {
    implment()->set_userdata(key, value);
}
Any *Object::userdata(const char_t *key) const {
    return implment()->userdata(key);
}
bool Object::handle(Event &event) {
    if (run_event_filter(event)) {
        return true;
    }
    if (event.type() == Event::Timer) {
        return timer_event(event.as<TimerEvent>());
    }
    return false;
}

ObjectImpl *Object::implment() const {
    if (priv == nullptr) {
        priv = new ObjectImpl;
        priv->ctxt = GetUIContext();
        BTK_ASSERT_MSG(priv->ctxt,"UIContext couldnot be nullptr");
    }
    return priv;
}
UIContext  *Object::ui_context() const {
    return implment()->ctxt;
}
bool        Object::ui_thread() const {
    return ui_context()->ui_thread_id() == std::this_thread::get_id();
}
std::shared_ptr<bool> Object::mark() const {
    return implment()->mark;
}

// Timer
timerid_t  Object::add_timer(timertype_t t,uint32_t ms) {
    auto timerid = implment()->ctxt->dispatcher()->timer_add(this, t, ms);
    if (timerid != 0) {
        implment()->timers.insert(timerid);
    }
    return timerid;
}
bool       Object::del_timer(timerid_t timerid) {
    if (implment()->timers.erase(timerid) > 0) {
        return implment()->ctxt->dispatcher()->timer_del(this, timerid);
    }
    return false;
}

// Deferred call
void       Object::defer_delete() {
    defer_call([this]() {
        delete this;
    });
}
void       Object::defer_call(DeferRoutinue rt, pointer_t user) {
    auto ctxt = ui_context();
    CallEvent event;
    event.set_func(rt);
    event.set_user(user);
    ctxt->dispatcher()->send(event);
}

// Event Filter
void  Object::add_event_filter(EventFilter filter, pointer_t user) {
    EventFilterNode *node = new EventFilterNode;
    node->filter = filter;
    node->userdata = user;
    node->next = implment()->filters;
    implment()->filters = node;
}
void  Object::del_event_filter(EventFilter filter, pointer_t user) {
    EventFilterNode *node = implment()->filters;
    EventFilterNode *prev = nullptr;
    while (node) {
        if (node->filter == filter && node->userdata == user) {
            if (prev == nullptr) {
                implment()->filters = node->next;
            }
            else {
                prev->next = node->next;
            }
            delete node;
            return;
        }
        prev = node;
        node = node->next;
    }
}
bool  Object::run_event_filter(Event &event) {
    if (priv == nullptr) {
        return false;
    }
    // Run it
    auto node = priv->filters;
    while (node) {
        if (node->filter(this, event, node->userdata)) {
            // Discard event
            return true;
        }
        node = node->next;
    }
    return false;
}


// Timer
Timer::Timer() {}
Timer::~Timer() {
    if (_id != 0) {
        del_timer(_id);
    }
}

void Timer::stop() {
    if (_id != 0) {
        del_timer(_id);
        _id = 0;
    }
}
void Timer::start() {
    if (_id != 0) {
        del_timer(_id);
        _id = 0;
    }
    if (_interval > 0) {
        _id = add_timer(_type, _interval);
    }
}
void Timer::set_interval(uint32_t ms) {
    _interval = ms;
    if (_id == 0) {
        return;
    }
    // Update timer
    del_timer(_id);
    _id = 0;
    if (_interval > 0) {
        _id = add_timer(_type, _interval);
    }
}
void Timer::set_type(timertype_t t) {
    _type = t;
    if (_id == 0) {
        return;
    }
    // Update timer
    del_timer(_id);
    _id = 0;
    if (_interval > 0) {
        _id = add_timer(_type, _interval);
    }
}
void Timer::set_repeat(bool repeat) {
    _repeat = repeat;
}
bool Timer::timer_event(TimerEvent &event) {
    if (event.timerid() != _id) {
        return false;
    }
    _timeout.emit();
    if (!_repeat) {
        stop();
    }
    return true;
}

// Task Manager
TaskManager::TaskManager(EventDispatcher *d) : dispatcher(d) {
    if (!dispatcher) {
        dispatcher = GetUIDisplatcher();
    }
}
TaskManager::~TaskManager() {
    cancel_task();
}
void TaskManager::send(void (*fn)(void *), void *obj) {
    CallEvent event;
    event.set_func(fn);
    event.set_user(obj);

    dispatcher->send(event);
}
void TaskManager::cancel_task() {
    *mark = false;
    mark = std::make_shared<bool>(true);
}

BTK_NS_END