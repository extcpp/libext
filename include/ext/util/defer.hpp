// Copyright - 2021 - Jan Christoph Uhde <Jan@UhdeJC.com>
// Please see LICENSE.md for license or visit https://github.com/extcpp/basics
#ifndef EXT_UTIL_DEFER_HEADER
#define EXT_UTIL_DEFER_HEADER

#include <utility>
#include <type_traits>

namespace ext::util {

template<typename Functor>
struct defer {
    static_assert(std::is_nothrow_destructible_v<Functor>);
#ifdef EXT_DEFER_ALLOW_NON_NOTHROW_INVOCABLE
    constexpr static bool is_nothrow_invocable = std::is_nothrow_invocable_v<Functor>;
#else
    static_assert(std::is_nothrow_invocable_v<Functor>);
#endif //EXT_DEFER_ALLOW_NON_NOTHROW_INVOCABLE

    defer() = delete;
    defer(defer const& other) = delete;
    defer& operator=(defer const& other) = delete;

    defer(Functor && c) noexcept(std::is_nothrow_move_constructible_v<Functor>)
        : functor(std::move(c)), active(true) { };
    defer(Functor const& c) noexcept(std::is_nothrow_copy_constructible_v<Functor>)
        : functor(c), active(true) { };
    defer(defer&& other) noexcept(std::is_nothrow_move_constructible_v<Functor>)
        : functor(std::move(other.functor)), active(other.active) {
        other.active = false;
    }

#ifndef EXT_DEFER_ENABLE_MOVE_ASSIGN
    defer& operator=(defer&& other) = delete;
#else
    defer& operator=(defer&& other) noexcept(std::is_nothrow_move_assignable_v<Functor>) {
        // ensure that this is the intended sematic
        execute(); // execute own action before it is lost
        if (other.active) {
            functor = std::move(other.functor);
            active = other.active;
            other.active = false;
        }
        return *this;
    }
#endif //EXT_DEFER_ENABLE_MOVE_ASSIGN

    ~defer() {
        execute();
    }

    void cancel() noexcept { active = false; }
    void execute() noexcept {
        if (active) {
#ifdef EXT_DEFER_ALLOW_NON_NOTHROW_INVOCABLE
            invoke();
#else
            functor(); // invoke functor directly as it is known to be safe
#endif //EXT_DEFER_ALLOW_NON_NOTHROW_INVOCABLE
            active = false;
        }
    }

private:

    Functor functor;
    bool active;

#ifdef EXT_DEFER_ALLOW_NON_NOTHROW_INVOCABLE
    [[deprecated]] void you_are_using_a_functor_in_defer_that_may_throw() noexcept {
        try {
            functor();
        } catch (...) { }
    }

    void invoke() noexcept {
        if constexpr (is_nothrow_invocable) {
            functor();
        } else {
            you_are_using_a_functor_in_defer_that_may_throw();
        }
    }
#endif //EXT_DEFER_ALLOW_NON_NOTHROW_INVOCABLE
};

} // namespace ext::util
#endif
