// Experimental Animation 

#pragma once

#include <Btk/object.hpp>
#include <Btk/event.hpp>
#include <Btk/defs.hpp>
#include <Btk/rect.hpp>

#include <functional>

BTK_NS_BEGIN

template <typename T>
class LerpAnimation : public Object {
    public:
        LerpAnimation() {}
        ~LerpAnimation() {
            if (_timerid != 0) {
                del_timer(_timerid);
            }
        }

        void set_duration(uint32_t dur) {
            _duration = dur;
        }
        void set_start_value(const T& start) {
            _start_value = start;
        }
        void set_end_value(const T& end) {
            _end_value = end;
        }
        void add_key_value(float step, const T &value) {
            values.push_back(Value{step, value});
        }

        void start() {
            if (_state == Paused) {
                // TODO
                auto peri = ts_cur - ts_end;
            }
            if (_state == Playing) {
                stop();
            }

            ts_start = GetTicks();
            ts_end = ts_start + _duration;
            ts_cur = ts_start;

            _timerid = add_timer(10);
            _state = Playing;
        }
        void pause() {
            del_timer(_timerid);
            _timerid = 0;
            _state = Paused;
        }
        void stop() {
            del_timer(_timerid);
            _timerid = 0;
            _state = Stopped;
        }

        bool timer_event(TimerEvent &event) override {
            if (event.timerid() == _timerid) {
                ts_cur = event.timestamp();
                if (ts_cur >= ts_end) {
                    stop();
                    return true;
                }
                auto rgn = float(ts_cur - ts_start) / float(_duration);
                T v = lerp(_start_value, _end_value, rgn);
                run(v);
            }
            return true;
        }

        template <typename Callable>
        void bind(Callable &&cb) {
            run = cb;
        }
    private:
        class Value {
            public:
                float step;
                T v;
        };
        enum State {
            Playing,
            Paused,
            Stopped
        };
        
        std::function<void(const T &value)> run;
        std::vector<Value> values;

        T _start_value;
        T _end_value;

        timestamp_t ts_start;
        timestamp_t ts_end;
        timestamp_t ts_cur;
        uint32_t _duration = 0;
        State    _state = Stopped;
        timerid_t _timerid = 0;
};

BTK_NS_END