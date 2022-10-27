// Experimental Animation 

#pragma once

#include <Btk/object.hpp>
#include <Btk/event.hpp>
#include <Btk/defs.hpp>
#include <Btk/rect.hpp>

#include <functional>
#include <algorithm>

BTK_NS_BEGIN

class AbstractAnimation : public Object {
    public:
        virtual void resume() = 0;
        virtual void pause()  = 0;
        virtual void start()  = 0;
        virtual void stop()   = 0;
};

class AnimationGroup    : public AbstractAnimation {
    private:
        std::vector<AbstractAnimation*> anims;
};

template <typename T>
class LerpAnimation     : public AbstractAnimation {
    public:
        template <typename ...Args>
        LerpAnimation(Args &&... args) {
            bind(std::forward<Args>(args)...);
        }
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
            sort_array();
            if (!values.empty()) {
                if (values.front().step == 0.0f) {
                    values.front().v = start;
                    return;
                }
                values.insert(values.begin(), Value{0.0f, start});
            }
            else {
                values.push_back({0.0f, start});
            }
        }
        void set_end_value(const T& end) {
            sort_array();
            if (!values.empty()) {
                if (values.back().step == 1.0f) {
                    values.back().v = end;
                    return;
                }
            }
            values.push_back({1.0f, end});
        }
        void add_key_value(float step, const T &value) {
            step = clamp(step, 0.0f, 1.0f);
            values.push_back(Value{step, value});
            values_changed = true;
        }

        void start() {
            sort_array();
            if (_state == Playing) {
                stop();
            }

            ts_start = GetTicks();
            ts_end = ts_start + _duration;
            ts_cur = ts_start;

            _timerid = add_timer(PerFrame);
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
        void resume() {
            sort_array();
            if (_state == Paused) {
                // Calc how many we should play
                auto peri = ts_cur - ts_end;

                ts_cur = GetTicks();
                ts_start = ts_cur - (_duration - peri);
                ts_end   = ts_cur + peri;

                _timerid = add_timer(PerFrame);
                _state   = Playing;
            }
        }

        bool timer_event(TimerEvent &event) override {
            if (event.timerid() == _timerid) {
                ts_cur = event.timestamp();
                if (ts_cur >= ts_end) {
                    stop();
                    return true;
                }
                auto rgn = float(ts_cur - ts_start) / float(_duration);
                // T v = lerp(_start_value, _end_value, rgn);
                auto [beg, end] = match_pair(rgn);
                auto perc = (rgn - beg.step) / (end.step - beg.step);


                BTK_ASSERT(!std::isnan(perc));
                BTK_ASSERT(!std::isinf(perc));

                T v = lerp(beg.v, end.v, perc);

                printf("%f / %f = %f\n", rgn - beg.step, end.step - beg.step, perc);

                run(v);
            }
            return true;
        }

        template <typename Callable>
        void bind(Callable &&cb) {
            run = cb;
        }
        // Member function
        template <typename Class, typename Ret, class Prov>
        void bind(Ret (Class::*fn)(const T &value), Prov *self) {
            run = [=](const T &v) {
                (self->*fn)(v);
            };
        }
        template <typename Class, typename Ret, class Prov>
        void bind(Ret (Class::*fn)(T value), Prov *self) {
            run = [=](const T &v) -> void {
                (self->*fn)(v);
            };
        }
        // Member variable 
        template <typename Class, typename Var, class Prov>
        void bind(Var (Class::*var), Prov *self) {
            run = [=](const T &value) {
                (self->*var) = value;
            };
        }
        // Variable
        template <typename Var>
        void bind(Var *var) {
            run = [=](const T &value) {
                *var = value;
            };
        }

        BTK_EXPOSE_SIGNAL(_finished);
    private:
        static constexpr uint32_t PerFrame = 1000 / 120;

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

        timestamp_t ts_start;
        timestamp_t ts_end;
        timestamp_t ts_cur;
        uint32_t _duration = 0;
        State    _state = Stopped;
        timerid_t _timerid = 0;
        bool     values_changed = false;

        Signal<void()> _finished;

        // Private method
        auto match_pair(float perc) const -> std::pair<Value, Value>;
        auto sort_array()                 -> void;
};

template <typename T>
inline auto LerpAnimation<T>::match_pair(float perc) const -> std::pair<Value, Value> {
    std::pair<Value, Value> result;
    for (auto iter = values.begin(); iter != values.end(); iter++) {
        if (iter->step <= perc && iter + 1 != values.end() && (iter + 1)->step >= perc) {
            result.first = *iter;

            ++iter;
            BTK_ASSERT(iter != values.end());
            result.second = *iter;
            break;
        }
    }
    return result;
}
template <typename T>
inline auto LerpAnimation<T>::sort_array() -> void {
    if (values_changed) {
        // std::sort(values.begin(), values.end());
        std::qsort(
            values.data(), 
            values.size(), 
            sizeof(Value), 
            [](const void *a, const void *b) -> int {
                return static_cast<const Value *>(a)->step > static_cast<const Value *>(b)->step;
            }
        );
        values_changed = false;
    }
}


BTK_NS_END