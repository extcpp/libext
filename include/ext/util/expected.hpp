///
// expected - An implementation of std::expected with extensions
// Written in 2017 by Simon Brand (simonrbrand@gmail.com, @TartanLlama)
//
// Documentation available at http://tl.tartanllama.xyz/
//
// To the extent possible under law, the author(s) have dedicated all
// copyright and related and neighboring rights to this software to the
// public domain worldwide. This software is distributed without any warranty.
//
// You should have received a copy of the CC0 Public Domain Dedication
// along with this software. If not, see
// <http://creativecommons.org/publicdomain/zero/1.0/>.
// https://github.com/TartanLlama/expected/tree/1d9c5d8c0da84b8ddc54bd3d90d632eec95c1f13
///

//
// Copyright - 2019 - Jan Christoph Uhde <Jan@UhdeJC.com> -- added by check_code
//

#ifndef EXT_UTIL_EXPECTED_HEADER
#define EXT_UTIL_EXPECTED_HEADER

#include <exception>
#include <functional>
#include <type_traits>
#include <utility>
#include <variant>

#if defined(__EXCEPTIONS) || defined(_CPPUNWIND) || defined(__cpp_exceptions)
    #define EXT_EXPECTED_EXCEPTIONS_ENABLED
#endif // defined(__EXCEPTIONS) || defined(_CPPUNWIND)


namespace ext::util {

template<class T, class E>
class expected;

template<class E>
class unexpected {
    static_assert(!std::is_same_v<E, void>, "E must not be void");

    public:
    using error_type = E;

    constexpr unexpected(unexpected const&) = default;
    constexpr unexpected(unexpected &&) = default;

    template<typename... Args
            ,typename = std::enable_if_t<std::is_constructible_v<E,Args&&...>>
            >
    constexpr explicit unexpected(std::in_place_t, Args&&... args) : error(std::forward<Args>(args)...) {}

    template<typename E2
            ,typename = std::enable_if_t<std::is_constructible_v<E,E2> &&
                                         !std::is_same_v<std::remove_cv_t<std::remove_reference_t<E2>>, std::in_place_t> &&
                                         !std::is_same_v<std::remove_cv_t<std::remove_reference_t<E2>>, unexpected>
                                        >
            >
    constexpr explicit unexpected(E2&& e) : error(std::forward<E2>(e)) {}


    constexpr const E& value() const& {
        return error;
    }
    constexpr E& value() & {
        return error;
    }
    constexpr E&& value() && {
        return std::move(error);
    }
    constexpr const E&& value() const&& {
        return std::move(error);
    }

    private:
    error_type error;
};

template<class E>
constexpr bool operator==(const unexpected<E>& lhs, const unexpected<E>& rhs) {
    return lhs.value() == rhs.value();
}
template<class E>
constexpr bool operator!=(const unexpected<E>& lhs, const unexpected<E>& rhs) {
    return lhs.value() != rhs.value();
}
template<class E>
constexpr bool operator<(const unexpected<E>& lhs, const unexpected<E>& rhs) {
    return lhs.value() < rhs.value();
}
template<class E>
constexpr bool operator<=(const unexpected<E>& lhs, const unexpected<E>& rhs) {
    return lhs.value() <= rhs.value();
}
template<class E>
constexpr bool operator>(const unexpected<E>& lhs, const unexpected<E>& rhs) {
    return lhs.value() > rhs.value();
}
template<class E>
constexpr bool operator>=(const unexpected<E>& lhs, const unexpected<E>& rhs) {
    return lhs.value() >= rhs.value();
}

template<class E>
unexpected<typename std::decay<E>::type> make_unexpected(E&& e) {
    return unexpected<typename std::decay<E>::type>(std::forward<E>(e));
}

struct unexpect_t {
    unexpect_t() = default;
};
static constexpr unexpect_t unexpect{};

namespace detail {
template<typename E>
[[noreturn]] constexpr void throw_exception(E&& e) {
#ifdef EXT_EXPECTED_EXCEPTIONS_ENABLED
    throw std::forward<E>(e);
#else
    (void) e;
    #ifdef _MSC_VER
    __assume(0);
    #else
    __builtin_unreachable();
    #endif
#endif // EXT_EXPECTED_EXCEPTIONS_ENABLED
}

// Trait for checking if a type is a ext::util::expected
template<class T>
struct is_expected_impl : std::false_type {};
template<class T, class E>
struct is_expected_impl<expected<T, E>> : std::true_type {};
template<class T>
using is_expected = is_expected_impl<std::decay_t<T>>;
template<class T>
constexpr bool is_expected_v = is_expected<T>::value;


} // namespace detail

template<class E>
class bad_expected_access : public std::exception {
    public:
    explicit bad_expected_access(E e) : m_val(std::move(e)) {}

    virtual const char* what() const noexcept override {
        return "Bad expected access";
    }

    const E& error() const& {
        return m_val;
    }
    E& error() & {
        return m_val;
    }
    const E&& error() const&& {
        return std::move(m_val);
    }
    E&& error() && {
        return std::move(m_val);
    }

    private:
    E m_val;
};

template<class T, class E>
class expected : public std::variant<T,unexpected<E>> {
    static_assert(!std::is_reference_v<T>, "T must not be a reference");
    static_assert(!std::is_same_v<T, std::remove_cv<std::in_place_t>>, "T must not be in_place_t");
    static_assert(!std::is_same_v<T, std::remove_cv<unexpect_t>>, "T must not be unexpect_t");
    static_assert(!std::is_same_v<T, std::remove_cv<unexpected<E>>>, "T must not be unexpected<E>");
    static_assert(!std::is_reference_v<E>, "E must not be a reference");

public:
    using value_type = T;
    using error_type = E;
    using unexpected_type = unexpected<E>;


private:
    template<class U = T, std::enable_if_t<!std::is_void_v<U>>* = nullptr>
    constexpr U& val() {
        return std::get<0>(*this);
    }
    constexpr unexpected<E>& err() {
        return std::get<1>(*this);
    }

    template<class U = T, std::enable_if_t<!std::is_void_v<U>>* = nullptr>
    constexpr const U& val() const {
        return std::get<1>(*this);
    }
    constexpr const unexpected<E>& err() const {
        return std::get<1>(*this);
    }

    public:

    template<class F>
    constexpr auto and_then(F&& f) & {
        return and_then_impl(*this, std::forward<F>(f));
    }
    template<class F>
    constexpr auto and_then(F&& f) && {
        return and_then_impl(std::move(*this), std::forward<F>(f));
    }
    template<class F>
    constexpr auto and_then(F&& f) const& {
        return and_then_impl(*this, std::forward<F>(f));
    }

    template<class F>
    constexpr auto and_then(F&& f) const&& {
        return and_then_impl(std::move(*this), std::forward<F>(f));
    }


    template<class F>
    constexpr auto map(F&& f) & {
        return expected_map_impl(*this, std::forward<F>(f));
    }
    template<class F>
    constexpr auto map(F&& f) && {
        return expected_map_impl(std::move(*this), std::forward<F>(f));
    }
    template<class F>
    constexpr auto map(F&& f) const& {
        return expected_map_impl(*this, std::forward<F>(f));
    }
    template<class F>
    constexpr auto map(F&& f) const&& {
        return expected_map_impl(std::move(*this), std::forward<F>(f));
    }

    template<class F>
    constexpr auto transform(F&& f) & {
        return expected_map_impl(*this, std::forward<F>(f));
    }
    template<class F>
    constexpr auto transform(F&& f) && {
        return expected_map_impl(std::move(*this), std::forward<F>(f));
    }
    template<class F>
    constexpr auto transform(F&& f) const& {
        return expected_map_impl(*this, std::forward<F>(f));
    }
    template<class F>
    constexpr auto transform(F&& f) const&& {
        return expected_map_impl(std::move(*this), std::forward<F>(f));
    }

    template<class F>
    constexpr auto map_error(F&& f) & {
        return map_error_impl(*this, std::forward<F>(f));
    }
    template<class F>
    constexpr auto map_error(F&& f) && {
        return map_error_impl(std::move(*this), std::forward<F>(f));
    }
    template<class F>
    constexpr auto map_error(F&& f) const& {
        return map_error_impl(*this, std::forward<F>(f));
    }
    template<class F>
    constexpr auto map_error(F&& f) const&& {
        return map_error_impl(std::move(*this), std::forward<F>(f));
    }

    template<class F>
    expected constexpr or_else(F&& f) & {
        return or_else_impl(*this, std::forward<F>(f));
    }

    template<class F>
    expected constexpr or_else(F&& f) && {
        return or_else_impl(std::move(*this), std::forward<F>(f));
    }

    template<class F>
    expected constexpr or_else(F&& f) const& {
        return or_else_impl(*this, std::forward<F>(f));
    }

    template<class F>
    expected constexpr or_else(F&& f) const&& {
        return or_else_impl(std::move(*this), std::forward<F>(f));
    }
    constexpr expected() = default;
    constexpr expected(const expected& rhs) = default;
    constexpr expected(expected&& rhs) = default;
    expected& operator=(const expected& rhs) = default;
    expected& operator=(expected&& rhs) = default;

    template<class... Args, std::enable_if_t<std::is_constructible_v<T, Args&&...>>* = nullptr>
    constexpr expected(std::in_place_t, Args&&... args)
        : std::variant<T,E>(std::in_place, std::forward<Args>(args)...) {}

    template<class U,
             class... Args,
             std::enable_if_t<std::is_constructible_v<T, std::initializer_list<U>&, Args&&...>>* = nullptr>
    constexpr expected(std::in_place_t, std::initializer_list<U> il, Args&&... args)
        : std::variant<T,E>(std::in_place, il, std::forward<Args>(args)...) {}

    template<class G = E,
             std::enable_if_t<std::is_constructible_v<E, const G&>>* = nullptr,
             std::enable_if_t<!std::is_convertible_v<const G&, E>>* = nullptr>
    explicit constexpr expected(const unexpected<G>& e)
        : std::variant<T,E>(unexpect, e.value()) {}

    template<class G = E,
             std::enable_if_t<std::is_constructible_v<E, const G&>>* = nullptr,
             std::enable_if_t<std::is_convertible_v<const G&, E>>* = nullptr>
    constexpr expected(unexpected<G> const& e)
        : std::variant<T,E>(unexpect, e.value()) {}

    template<class G = E,
             std::enable_if_t<std::is_constructible_v<E, G&&>>* = nullptr,
             std::enable_if_t<!std::is_convertible_v<G&&, E>>* = nullptr>
    explicit constexpr expected(unexpected<G>&& e) noexcept(std::is_nothrow_constructible_v<E, G&&>)
        : std::variant<T,E>(unexpect, std::move(e.value())) {}

    template<class G = E,
             std::enable_if_t<std::is_constructible_v<E, G&&>>* = nullptr,
             std::enable_if_t<std::is_convertible_v<G&&, E>>* = nullptr>
    constexpr expected(unexpected<G>&& e) noexcept(std::is_nothrow_constructible_v<E, G&&>)
        : std::variant<T,E>(unexpect, std::move(e.value())) {}

    template<class... Args, std::enable_if_t<std::is_constructible_v<E, Args&&...>>* = nullptr>
    constexpr explicit expected(unexpect_t, Args&&... args)
        : std::variant<T,E>(unexpect, std::forward<Args>(args)...) {}

    template<class U,
             class... Args,
             std::enable_if_t<std::is_constructible_v<E, std::initializer_list<U>&, Args&&...>>* = nullptr>
    constexpr explicit expected(unexpect_t, std::initializer_list<U> il, Args&&... args)
        : std::variant<T,E>(unexpect, il, std::forward<Args>(args)...) {}

//    template<class U,
//             class G,
//             std::enable_if_t<!(std::is_convertible_v<U const&, T> && std::is_convertible_v<G const&, E>)>* = nullptr,
//             detail::expected_enable_from_other<T, E, U, G, const U&, const G&>* = nullptr>
//    explicit constexpr expected(const expected<U, G>& rhs) : ctor_base(detail::default_constructor_tag{}) {
//        if (rhs.has_value()) {
//            this->construct(*rhs);
//        } else {
//            this->construct_error(rhs.error());
//        }
//    }

//    template<class U,
//             class G,
//             std::enable_if_t<(std::is_convertible_v<U const&, T> && std::is_convertible_v<G const&, E>)>* = nullptr,
//             detail::expected_enable_from_other<T, E, U, G, const U&, const G&>* = nullptr>
//    constexpr expected(const expected<U, G>& rhs) : ctor_base(detail::default_constructor_tag{}) {
//        if (rhs.has_value()) {
//            this->construct(*rhs);
//        } else {
//            this->construct_error(rhs.error());
//        }
//    }

//    template<class U,
//             class G,
//             std::enable_if_t<!(std::is_convertible_v<U&&, T> && std::is_convertible_v<G&&, E>)>* = nullptr,
//             detail::expected_enable_from_other<T, E, U, G, U&&, G&&>* = nullptr>
//    explicit constexpr expected(expected<U, G>&& rhs) : ctor_base(detail::default_constructor_tag{}) {
//        if (rhs.has_value()) {
//            this->construct(std::move(*rhs));
//        } else {
//            this->construct_error(std::move(rhs.error()));
//        }
//    }

//    template<class U,
//             class G,
//             std::enable_if_t<(std::is_convertible_v<U&&, T> && std::is_convertible_v<G&&, E>)>* = nullptr,
//             detail::expected_enable_from_other<T, E, U, G, U&&, G&&>* = nullptr>
//    constexpr expected(expected<U, G>&& rhs) : ctor_base(detail::default_constructor_tag{}) {
//        if (rhs.has_value()) {
//            this->construct(std::move(*rhs));
//        } else {
//            this->construct_error(std::move(rhs.error()));
//        }
//    }

    template<class U = T,
             std::enable_if_t<!std::is_convertible_v<U&&, T>>* = nullptr>
    explicit constexpr expected(U&& v) : expected(std::in_place, std::forward<U>(v)) {}

    template<class U = T,
             std::enable_if_t<std::is_convertible_v<U&&, T>>* = nullptr>
    constexpr expected(U&& v) : expected(std::in_place, std::forward<U>(v)) {}

    template<class U = T,
             class G = T,
             std::enable_if_t<std::is_nothrow_constructible_v<T, U&&>>* = nullptr,
             std::enable_if_t<!std::is_void_v<G>>* = nullptr,
             std::enable_if_t<(!std::is_same_v<expected<T, E>, std::decay_t<U>> &&
                               !std::conjunction_v<std::is_scalar<T>, std::is_same<T, std::decay_t<U>>> &&
                               std::is_constructible_v<T, U> && std::is_assignable_v<G&, U> &&
                               std::is_nothrow_move_constructible_v<E>)>* = nullptr>
    expected& operator=(U&& v) {
        if (has_value()) {
            val() = std::forward<U>(v);
        } else {
            err().~unexpected<E>();
            ::new (valptr()) T(std::forward<U>(v));
            this->m_has_val = true;
        }

        return *this;
    }

    template<class U = T,
             class G = T,
             std::enable_if_t<!std::is_nothrow_constructible_v<T, U&&>>* = nullptr,
             std::enable_if_t<!std::is_void_v<U>>* = nullptr,
             std::enable_if_t<(!std::is_same_v<expected<T, E>, std::decay_t<U>> &&
                               !std::conjunction_v<std::is_scalar<T>, std::is_same<T, std::decay_t<U>>> &&
                               std::is_constructible_v<T, U> && std::is_assignable_v<G&, U> &&
                               std::is_nothrow_move_constructible_v<E>)>* = nullptr>
    expected& operator=(U&& v) {
        if (has_value()) {
            val() = std::forward<U>(v);
        } else {
            auto tmp = std::move(err());
            err().~unexpected<E>();

#ifdef EXT_EXPECTED_EXCEPTIONS_ENABLED
            try {
                ::new (valptr()) T(std::forward<U>(v));
                this->m_has_val = true;
            } catch (...) {
                err() = std::move(tmp);
                throw;
            }
#else
            ::new (valptr()) T(std::forward<U>(v));
            this->m_has_val = true;
#endif // EXT_EXPECTED_EXCEPTIONS_ENABLED
        }

        return *this;
    }

    template<class G = E,
             std::enable_if_t<std::is_nothrow_copy_constructible_v<G> && std::is_assignable_v<G&, G>>* = nullptr>
    expected& operator=(const unexpected<G>& rhs) {
        if (!has_value()) {
            err() = rhs;
        } else {
            this->destroy_val();
            ::new (errptr()) unexpected<E>(rhs);
            this->m_has_val = false;
        }

        return *this;
    }

    template<class G = E,
             std::enable_if_t<std::is_nothrow_move_constructible_v<G> && std::is_move_assignable_v<G>>* = nullptr>
    expected& operator=(unexpected<G>&& rhs) noexcept {
        if (!has_value()) {
            err() = std::move(rhs);
        } else {
            this->destroy_val();
            ::new (errptr()) unexpected<E>(std::move(rhs));
            this->m_has_val = false;
        }

        return *this;
    }

    template<class... Args, std::enable_if_t<std::is_nothrow_constructible_v<T, Args&&...>>* = nullptr>
    void emplace(Args&&... args) {
        if (has_value()) {
            val() = T(std::forward<Args>(args)...);
        } else {
            err().~unexpected<E>();
            ::new (valptr()) T(std::forward<Args>(args)...);
            this->m_has_val = true;
        }
    }

    template<class... Args, std::enable_if_t<!std::is_nothrow_constructible_v<T, Args&&...>>* = nullptr>
    void emplace(Args&&... args) {
        if (has_value()) {
            val() = T(std::forward<Args>(args)...);
        } else {
            auto tmp = std::move(err());
            err().~unexpected<E>();

#ifdef EXT_EXPECTED_EXCEPTIONS_ENABLED
            try {
                ::new (valptr()) T(std::forward<Args>(args)...);
                this->m_has_val = true;
            } catch (...) {
                err() = std::move(tmp);
                throw;
            }
#else
            ::new (valptr()) T(std::forward<Args>(args)...);
            this->m_has_val = true;
#endif // EXT_EXPECTED_EXCEPTIONS_ENABLED
        }
    }

    template<class U,
             class... Args,
             std::enable_if_t<std::is_nothrow_constructible_v<T, std::initializer_list<U>&, Args&&...>>* = nullptr>
    void emplace(std::initializer_list<U> il, Args&&... args) {
        if (has_value()) {
            T t(il, std::forward<Args>(args)...);
            val() = std::move(t);
        } else {
            err().~unexpected<E>();
            ::new (valptr()) T(il, std::forward<Args>(args)...);
            this->m_has_val = true;
        }
    }

    template<class U,
             class... Args,
             std::enable_if_t<!std::is_nothrow_constructible_v<T, std::initializer_list<U>&, Args&&...>>* = nullptr>
    void emplace(std::initializer_list<U> il, Args&&... args) {
        if (has_value()) {
            T t(il, std::forward<Args>(args)...);
            val() = std::move(t);
        } else {
            auto tmp = std::move(err());
            err().~unexpected<E>();

#ifdef EXT_EXPECTED_EXCEPTIONS_ENABLED
            try {
                ::new (valptr()) T(il, std::forward<Args>(args)...);
                this->m_has_val = true;
            } catch (...) {
                err() = std::move(tmp);
                throw;
            }
#else
            ::new (valptr()) T(il, std::forward<Args>(args)...);
            this->m_has_val = true;
#endif // EXT_EXPECTED_EXCEPTIONS_ENABLED
        }
    }

    private:
    using t_is_void = std::true_type;
    using t_is_not_void = std::false_type;
    using t_is_nothrow_move_constructible = std::true_type;
    using move_constructing_t_can_throw = std::false_type;
    using e_is_nothrow_move_constructible = std::true_type;
    using move_constructing_e_can_throw = std::false_type;

    void swap_where_both_have_value(expected& /*rhs*/, t_is_void) noexcept {
        // swapping void is a no-op
    }

    void swap_where_both_have_value(expected& rhs, t_is_not_void) {
        using std::swap;
        swap(val(), rhs.val());
    }

    void swap_where_only_one_has_value(expected& rhs, t_is_void) noexcept(std::is_nothrow_move_constructible_v<E>) {
        ::new (errptr()) unexpected_type(std::move(rhs.err()));
        rhs.err().~unexpected_type();
        std::swap(this->m_has_val, rhs.m_has_val);
    }

    void swap_where_only_one_has_value(expected& rhs, t_is_not_void) {
        swap_where_only_one_has_value_and_t_is_not_void(rhs,
                                                        typename std::is_nothrow_move_constructible<T>::type{},
                                                        typename std::is_nothrow_move_constructible<E>::type{});
    }

    void swap_where_only_one_has_value_and_t_is_not_void(expected& rhs,
                                                         t_is_nothrow_move_constructible,
                                                         e_is_nothrow_move_constructible) noexcept {
        auto temp = std::move(val());
        val().~T();
        ::new (errptr()) unexpected_type(std::move(rhs.err()));
        rhs.err().~unexpected_type();
        ::new (rhs.valptr()) T(std::move(temp));
        std::swap(this->m_has_val, rhs.m_has_val);
    }

    void swap_where_only_one_has_value_and_t_is_not_void(expected& rhs,
                                                         t_is_nothrow_move_constructible,
                                                         move_constructing_e_can_throw) {
        auto temp(std::move(val()));
        val().~T();
#ifdef EXT_EXPECTED_EXCEPTIONS_ENABLED
        try {
            ::new (errptr()) unexpected_type(std::move(rhs.err()));
            rhs.err().~unexpected_type();
            ::new (rhs.valptr()) T(std::move(temp));
            std::swap(this->m_has_val, rhs.m_has_val);
        } catch (...) {
            ::new (valptr()) T(std::move(temp));
            throw;
        }
#else
        ::new (errptr()) unexpected_type(std::move(rhs.err()));
        rhs.err().~unexpected_type();
        ::new (rhs.valptr()) T(std::move(temp));
        std::swap(this->m_has_val, rhs.m_has_val);
#endif // EXT_EXPECTED_EXCEPTIONS_ENABLED
    }

    void swap_where_only_one_has_value_and_t_is_not_void(expected& rhs,
                                                         move_constructing_t_can_throw,
                                                         t_is_nothrow_move_constructible) {
        auto temp = std::move(rhs.err());
        rhs.err().~unexpected_type();
#ifdef EXT_EXPECTED_EXCEPTIONS_ENABLED
        try {
            ::new (rhs.valptr()) T(val());
            val().~T();
            ::new (errptr()) unexpected_type(std::move(temp));
            std::swap(this->m_has_val, rhs.m_has_val);
        } catch (...) {
            rhs.err() = std::move(temp);
            throw;
        }
#else
        ::new (rhs.valptr()) T(val());
        val().~T();
        ::new (errptr()) unexpected_type(std::move(temp));
        std::swap(this->m_has_val, rhs.m_has_val);
#endif // EXT_EXPECTED_EXCEPTIONS_ENABLED
    }

    public:
    void swap(expected& rhs) noexcept{
        using std::swap;
        swap(rhs, *swap);
    }

    constexpr const T* operator->() const {
        return valptr();
    }
    constexpr T* operator->() {
        return valptr();
    }

    template<class U = T, std::enable_if_t<!std::is_void_v<U>>* = nullptr>
    constexpr const U& operator*() const& {
        return val();
    }
    template<class U = T, std::enable_if_t<!std::is_void_v<U>>* = nullptr>
    constexpr U& operator*() & {
        return val();
    }
    template<class U = T, std::enable_if_t<!std::is_void_v<U>>* = nullptr>
    constexpr const U&& operator*() const&& {
        return std::move(val());
    }
    template<class U = T, std::enable_if_t<!std::is_void_v<U>>* = nullptr>
    constexpr U&& operator*() && {
        return std::move(val());
    }

    constexpr bool has_value() const noexcept {
        return this->m_has_val;
    }
    constexpr explicit operator bool() const noexcept {
        return this->m_has_val;
    }

    template<class U = T, std::enable_if_t<!std::is_void_v<U>>* = nullptr>
    constexpr const U& value() const& {
        if (!has_value())
            detail::throw_exception(bad_expected_access<E>(err().value()));
        return val();
    }
    template<class U = T, std::enable_if_t<!std::is_void_v<U>>* = nullptr>
    constexpr U& value() & {
        if (!has_value())
            detail::throw_exception(bad_expected_access<E>(err().value()));
        return val();
    }
    template<class U = T, std::enable_if_t<!std::is_void_v<U>>* = nullptr>
    constexpr const U&& value() const&& {
        if (!has_value())
            detail::throw_exception(bad_expected_access<E>(std::move(err()).value()));
        return std::move(val());
    }
    template<class U = T, std::enable_if_t<!std::is_void_v<U>>* = nullptr>
    constexpr U&& value() && {
        if (!has_value())
            detail::throw_exception(bad_expected_access<E>(std::move(err()).value()));
        return std::move(val());
    }

    constexpr const E& error() const& {
        return err().value();
    }
    constexpr E& error() & {
        return err().value();
    }
    constexpr const E&& error() const&& {
        return std::move(err().value());
    }
    constexpr E&& error() && {
        return std::move(err().value());
    }

    template<class U>
    constexpr T value_or(U&& v) const& {
        static_assert(std::is_copy_constructible_v<T> && std::is_convertible_v<U&&, T>,
                      "T must be copy-constructible and convertible to from U&&");
        return bool(*this) ? **this : static_cast<T>(std::forward<U>(v));
    }
    template<class U>
    constexpr T value_or(U&& v) && {
        static_assert(std::is_move_constructible_v<T> && std::is_convertible_v<U&&, T>,
                      "T must be move-constructible and convertible to from U&&");
        return bool(*this) ? std::move(**this) : static_cast<T>(std::forward<U>(v));
    }
};

namespace detail {
template<class Exp>
using val_t = typename std::decay_t<Exp>::value_type;
template<class Exp>
using err_t = typename std::decay_t<Exp>::error_type;
template<class Exp, class Ret>
using ret_t = expected<Ret, err_t<Exp>>;

template<class Exp,
         class F,
         std::enable_if_t<!std::is_void_v<val_t<Exp>>>* = nullptr,
         class Ret = decltype(std::invoke(std::declval<F>(), *std::declval<Exp>()))>
constexpr auto and_then_impl(Exp&& exp, F&& f) {
    static_assert(detail::is_expected_v<Ret>, "F must return an expected");

    return exp.has_value() ? std::invoke(std::forward<F>(f), *std::forward<Exp>(exp))
                           : Ret(unexpect, std::forward<Exp>(exp).error());
}

template<class Exp,
         class F,
         std::enable_if_t<std::is_void_v<val_t<Exp>>>* = nullptr,
         class Ret = decltype(std::invoke(std::declval<F>()))>
constexpr auto and_then_impl(Exp&& exp, F&& f) {
    static_assert(detail::is_expected_v<Ret>, "F must return an expected");

    return exp.has_value() ? std::invoke(std::forward<F>(f)) : Ret(unexpect, std::forward<Exp>(exp).error());
}

template<class Exp,
         class F,
         std::enable_if_t<!std::is_void_v<val_t<Exp>>>* = nullptr,
         class Ret = decltype(std::invoke(std::declval<F>(), *std::declval<Exp>())),
         std::enable_if_t<!std::is_void_v<Ret>>* = nullptr>
constexpr auto expected_map_impl(Exp&& exp, F&& f) {
    using result = ret_t<Exp, std::decay_t<Ret>>;
    return exp.has_value() ? result(std::invoke(std::forward<F>(f), *std::forward<Exp>(exp)))
                           : result(unexpect, std::forward<Exp>(exp).error());
}

template<class Exp,
         class F,
         std::enable_if_t<!std::is_void_v<val_t<Exp>>>* = nullptr,
         class Ret = decltype(std::invoke(std::declval<F>(), *std::declval<Exp>())),
         std::enable_if_t<std::is_void_v<Ret>>* = nullptr>
auto expected_map_impl(Exp&& exp, F&& f) {
    using result = expected<void, err_t<Exp>>;
    if (exp.has_value()) {
        std::invoke(std::forward<F>(f), *std::forward<Exp>(exp));
        return result();
    }

    return result(unexpect, std::forward<Exp>(exp).error());
}

template<class Exp,
         class F,
         std::enable_if_t<std::is_void_v<val_t<Exp>>>* = nullptr,
         class Ret = decltype(std::invoke(std::declval<F>())),
         std::enable_if_t<!std::is_void_v<Ret>>* = nullptr>
constexpr auto expected_map_impl(Exp&& exp, F&& f) {
    using result = ret_t<Exp, std::decay_t<Ret>>;
    return exp.has_value() ? result(std::invoke(std::forward<F>(f))) : result(unexpect, std::forward<Exp>(exp).error());
}

template<class Exp,
         class F,
         std::enable_if_t<std::is_void_v<val_t<Exp>>>* = nullptr,
         class Ret = decltype(std::invoke(std::declval<F>())),
         std::enable_if_t<std::is_void_v<Ret>>* = nullptr>
auto expected_map_impl(Exp&& exp, F&& f) {
    using result = expected<void, err_t<Exp>>;
    if (exp.has_value()) {
        std::invoke(std::forward<F>(f));
        return result();
    }

    return result(unexpect, std::forward<Exp>(exp).error());
}

template<class Exp,
         class F,
         std::enable_if_t<!std::is_void_v<val_t<Exp>>>* = nullptr,
         class Ret = decltype(std::invoke(std::declval<F>(), std::declval<Exp>().error())),
         std::enable_if_t<!std::is_void_v<Ret>>* = nullptr>
constexpr auto map_error_impl(Exp&& exp, F&& f) {
    using result = expected<val_t<Exp>, std::decay_t<Ret>>;
    return exp.has_value() ? result(*std::forward<Exp>(exp))
                           : result(unexpect, std::invoke(std::forward<F>(f), std::forward<Exp>(exp).error()));
}
template<class Exp,
         class F,
         std::enable_if_t<!std::is_void_v<val_t<Exp>>>* = nullptr,
         class Ret = decltype(std::invoke(std::declval<F>(), std::declval<Exp>().error())),
         std::enable_if_t<std::is_void_v<Ret>>* = nullptr>
auto map_error_impl(Exp&& exp, F&& f) {
    using result = expected<val_t<Exp>, std::monostate>;
    if (exp.has_value()) {
        return result(*std::forward<Exp>(exp));
    }

    std::invoke(std::forward<F>(f), std::forward<Exp>(exp).error());
    return result(unexpect, std::monostate{});
}
template<class Exp,
         class F,
         std::enable_if_t<std::is_void_v<val_t<Exp>>>* = nullptr,
         class Ret = decltype(std::invoke(std::declval<F>(), std::declval<Exp>().error())),
         std::enable_if_t<!std::is_void_v<Ret>>* = nullptr>
constexpr auto map_error_impl(Exp&& exp, F&& f) {
    using result = expected<val_t<Exp>, std::decay_t<Ret>>;
    return exp.has_value() ? result()
                           : result(unexpect, std::invoke(std::forward<F>(f), std::forward<Exp>(exp).error()));
}
template<class Exp,
         class F,
         std::enable_if_t<std::is_void_v<val_t<Exp>>>* = nullptr,
         class Ret = decltype(std::invoke(std::declval<F>(), std::declval<Exp>().error())),
         std::enable_if_t<std::is_void_v<Ret>>* = nullptr>
auto map_error_impl(Exp&& exp, F&& f) {
    using result = expected<val_t<Exp>, std::monostate>;
    if (exp.has_value()) {
        return result();
    }

    std::invoke(std::forward<F>(f), std::forward<Exp>(exp).error());
    return result(unexpect, std::monostate{});
}

template<class Exp,
         class F,
         class Ret = decltype(std::invoke(std::declval<F>(), std::declval<Exp>().error())),
         std::enable_if_t<!std::is_void_v<Ret>>* = nullptr>
auto or_else_impl(Exp&& exp, F&& f) -> Ret {
    static_assert(detail::is_expected_v<Ret>, "F must return an expected");
    return exp.has_value() ? std::forward<Exp>(exp) : std::invoke(std::forward<F>(f), std::forward<Exp>(exp).error());
}

template<class Exp,
         class F,
         class Ret = decltype(std::invoke(std::declval<F>(), std::declval<Exp>().error())),
         std::enable_if_t<std::is_void_v<Ret>>* = nullptr>
std::decay_t<Exp> or_else_impl(Exp&& exp, F&& f) {
    return exp.has_value() ? std::forward<Exp>(exp)
                           : (std::invoke(std::forward<F>(f), std::forward<Exp>(exp).error()), std::forward<Exp>(exp));
}
} // namespace detail

template<class T, class E, class U, class F>
constexpr bool operator==(const expected<T, E>& lhs, const expected<U, F>& rhs) {
    return (lhs.has_value() != rhs.has_value()) ? false
                                                : (!lhs.has_value() ? lhs.error() == rhs.error() : *lhs == *rhs);
}
template<class T, class E, class U, class F>
constexpr bool operator!=(const expected<T, E>& lhs, const expected<U, F>& rhs) {
    return (lhs.has_value() != rhs.has_value()) ? true : (!lhs.has_value() ? lhs.error() != rhs.error() : *lhs != *rhs);
}

template<class T, class E, class U>
constexpr bool operator==(const expected<T, E>& x, const U& v) {
    return x.has_value() ? *x == v : false;
}
template<class T, class E, class U>
constexpr bool operator==(const U& v, const expected<T, E>& x) {
    return x.has_value() ? *x == v : false;
}
template<class T, class E, class U>
constexpr bool operator!=(const expected<T, E>& x, const U& v) {
    return x.has_value() ? *x != v : true;
}
template<class T, class E, class U>
constexpr bool operator!=(const U& v, const expected<T, E>& x) {
    return x.has_value() ? *x != v : true;
}

template<class T, class E>
constexpr bool operator==(const expected<T, E>& x, const unexpected<E>& e) {
    return x.has_value() ? false : x.error() == e.value();
}
template<class T, class E>
constexpr bool operator==(const unexpected<E>& e, const expected<T, E>& x) {
    return x.has_value() ? false : x.error() == e.value();
}
template<class T, class E>
constexpr bool operator!=(const expected<T, E>& x, const unexpected<E>& e) {
    return x.has_value() ? true : x.error() != e.value();
}
template<class T, class E>
constexpr bool operator!=(const unexpected<E>& e, const expected<T, E>& x) {
    return x.has_value() ? true : x.error() != e.value();
}

template<class T,
         class E,
         std::enable_if_t<(std::is_void_v<T> || std::is_move_constructible_v<T>) &&detail::is_swappable<T>::value &&
                          std::is_move_constructible_v<E> && detail::is_swappable<E>::value>* = nullptr>
void swap(expected<T, E>& lhs, expected<T, E>& rhs) noexcept(noexcept(lhs.swap(rhs))) {
    lhs.swap(rhs);
}
} // namespace ext::util
#endif // EXT_UTIL_EXPECTED_HEADER
