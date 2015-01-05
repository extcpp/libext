#pragma once

#include <functional>
#include <utility>
#include <type_traits>
#include <map>
#include <tuple>

namespace obi { namespace util {

//requires c++14
auto add_function_cache = [](auto fun) {
    using fun_type = decltype(fun);
    return ([=](auto... run_args){
        using fun_return_type = std::result_of_t<fun_type(decltype(run_args)...)>;
        static std::map<
            std::tuple<decltype(run_args)...>,
            fun_return_type
        > result_cache;

        std::tuple<decltype(run_args)...> tuple(run_args...);
        if (result_cache.find(tuple) == result_cache.end()) {
            fun_return_type rv = fun(run_args...);
            result_cache[tuple] = fun(run_args...);
            return rv;
        }
        else {
            return result_cache[tuple];
        }
        return fun(run_args...);
    });
};

template <typename R, class... Args> auto
add_function_cache_old(std::function<R(Args...)> fun)
-> std::function<R(Args...)>
{
    std::map<std::tuple<Args...>, R> cache;
    return [=](Args... args) mutable  {
        std::tuple<Args...> t(args...);
        if (cache.find(t) == cache.end()) {
            R rv = fun(args...);
            cache[t] = rv;
            return rv;
        }
        else {
            return cache[t];
        }
    };
};

}}  // namespace obi::util
