#pragma once

#include <Btk/defs.hpp>
#include <memory>
#include <tuple>

BTK_NS_BEGIN

template<typename Callable, typename ...Args>
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

        auto invoke() noexcept(nothrow_v) -> std::invoke_result_t<Callable, Args...> {
            return std::apply(
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