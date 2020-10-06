// Copyright - 2020 - Jan Christoph Uhde <Jan@UhdeJC.com>
// Please see LICENSE.md for license or visit https://github.com/extcpp/basics
#ifndef EXT_UTIL_BASIC_HEADER
#define EXT_UTIL_BASIC_HEADER
#include <ext/macros/general.hpp>
#include <ext/util/cast.hpp>

#include <algorithm>
#include <string>
#include <type_traits>

namespace ext { namespace util {

template<typename F, typename... T>
void for_each_arg(F&& function, T&&... args) {
    (function(std::forward<T>(args)), ...); // c++17 fold expression
                                            // EXT_EXPAND_SIDE_EFFECTS(function(std::forward<T>(args)));
}

template<typename... T>
void sort_all(T&&... args) {
    static auto do_it = [](auto& container) {
        std::sort(std::begin(container), std::end(container));
    };
    for_each_arg(do_it, std::forward<T>(args)...);
}

inline std::string_view filename(std::string const& pathname, bool is_linux = true, bool both = false) {
    auto sep_predicate = [&](char c) {
        bool const lin = ((both || is_linux) && c == '/');
        bool const win = ((both || !is_linux) && c == '\\');
        return lin || win;
    };

    auto const start_word_itr = std::find_if(pathname.rbegin(), pathname.rend(), sep_predicate).base();
    auto const start_pos = std::distance(pathname.begin(), start_word_itr);
    auto const len = std::distance(start_word_itr, pathname.end());

    return std::string_view(pathname.data() + start_pos, ::ext::util::to_unsigned(len));
}
}}     // namespace ext::util
#endif // EXT_UTIL_BASIC_HEADER
