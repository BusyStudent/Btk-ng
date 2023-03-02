#pragma once

#include <Btk/defs.hpp>
#include <memory>
#include <tuple>

BTK_NS_BEGIN

/**
 * @brief Wrapper for a cancelable task.
 * 
 * @tparam Callable 
 * @tparam Args 
 */
template <typename Callable, typename ...Args>
class CancelableWrapper : public std::tuple<Args...> {
    public:
        static constexpr bool nothrow_v = std::is_nothrow_invocable_v<Callable, Args...>;
        using Mark = std::shared_ptr<bool>;

        CancelableWrapper(const Mark &m, Callable &&cb, Args &&...args) : 
            std::tuple<Args...>(std::forward<Args>(args)...),
            callable(std::forward<Callable>(cb)),
            mark(m)
        {

        }
        CancelableWrapper(const CancelableWrapper &) = default;
        ~CancelableWrapper() = default;

        void invoke() noexcept(nothrow_v) {
            if (*mark) {
                // Still alive
                std::apply(
                    callable,
                    static_cast<std::tuple<Args...>&&>(*this)
                );
            }
            else {
                BTK_LOG("Task was canceled %s\n", BTK_FUNCTION);
            }
        }
        void operator()() noexcept(nothrow_v) {
            return invoke();
        }

        static void Call(void *self) noexcept(nothrow_v) {
            std::unique_ptr<CancelableWrapper> wp(
                static_cast<CancelableWrapper*>(self)
            );
            wp->invoke();
        }
    private:
        Callable callable;
        Mark mark;
};

template <typename Callable, typename ...Args>
class DeferWrapper : public std::tuple<Args...> {
    public:
        static constexpr bool nothrow_v = std::is_nothrow_invocable_v<Callable, Args...>;

        DeferWrapper(Callable &&cb, Args &&...args) : 
            std::tuple<Args...>(std::forward<Args>(args)...),
            callable(std::forward<Callable>(cb)) 
        {

        }
        DeferWrapper(const DeferWrapper &) = default;
        ~DeferWrapper() = default;

        void invoke() noexcept(nothrow_v) {
            std::apply(
                callable,
                static_cast<std::tuple<Args...>&&>(*this)
            );
        }

        static void Call(void *self) noexcept(nothrow_v) {
            std::unique_ptr<DeferWrapper> wp(
                static_cast<DeferWrapper*>(self)
            );
            wp->invoke();
        }
    private:
        Callable callable;
};

template <typename T>
class DeleteWrapper {
    public:
        template <typename Base = void>
        static void Call(Base *p) {
            delete static_cast<T*>(p);
        }
        template <typename Base = void>
        static void Destruct(Base *p) {
            static_cast<T*>(p)->~T();
        }
};


BTK_NS_END