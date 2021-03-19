// Copyright - 2021 - Jan Christoph Uhde <Jan@UhdeJC.com>
// Please see LICENSE.md for license or visit https://github.com/extcpp/basics

#include <gtest/gtest.h>
#include <ext/util/defer.hpp>
#include <type_traits>

using namespace ext::util;
using namespace std::literals;

static int x = 0;
void free_function() noexcept { x++; }

TEST(util_defer, special) {
    defer lambda([&]() noexcept {  });

    static_assert(!std::is_copy_constructible_v<decltype(lambda)>);
    static_assert(!std::is_copy_assignable_v<decltype(lambda)>);
    static_assert(!std::is_move_assignable_v<decltype(lambda)>);

    static_assert(!std::is_copy_constructible_v<decltype(free_function)>);
    static_assert(!std::is_copy_assignable_v<decltype(free_function)>);
    static_assert(!std::is_move_assignable_v<decltype(free_function)>);
}

TEST(util_defer, move_ref_capture_lambda) {
    int a = 0;
    {
        defer action([&]() noexcept { a++; });
        ASSERT_EQ(a, 0);
    }
    ASSERT_EQ(a, 1);
}

TEST(util_defer, copy_ref_capture_lambda) {
    int a = 0;
    {
        auto task = [&]() noexcept { a++; };
        defer action(task);
        ASSERT_EQ(a, 0);
    }
    ASSERT_EQ(a, 1);
}


TEST(util_defer, copy_freestanding_function) {
    {
        defer action(&free_function);
        ASSERT_EQ(x, 0);
    }
    ASSERT_EQ(x, 1);
}
