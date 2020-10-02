// Copyright - 2020 - Jan Christoph Uhde <Jan@UhdeJC.com>
// Please see LICENSE.md for license or visit https://github.com/extcpp/basics
#include <ext/util/lru_cache.hpp>
#include <ext/macros/compiler.hpp>
#include <gtest/gtest.h>

namespace eu = ::ext::util;

TEST(util_lru_cache, put_exists) {
    eu::lru_cache<int,int> cache(3);
    cache.put(1,1);
    cache.put(2,2);
    cache.put(3,3);

    ASSERT_TRUE(cache.exists(1));
    ASSERT_TRUE(cache.exists(2));
    ASSERT_TRUE(cache.exists(3));
    ASSERT_FALSE(cache.exists(4));

    cache.put(4,4);

    ASSERT_FALSE(cache.exists(1));
    ASSERT_TRUE(cache.exists(4));

    // trigger splice
    cache.put(3,3);
    cache.put(3,3);
    cache.put(4,4);

    ASSERT_TRUE(cache.exists(3));
    ASSERT_TRUE(cache.exists(4));
}

TEST(util_lru_cache, put_get) {
    eu::lru_cache<int,int> cache(3);
    cache.put(1,1);
    cache.put(2,2);
    cache.put(3,3);

    ASSERT_TRUE(cache.get(1).second);
    ASSERT_TRUE(cache.get(2).second);
    ASSERT_TRUE(cache.get(3).second);
    ASSERT_FALSE(cache.get(4).second);

    ASSERT_EQ(cache.get(1).first,1);
    ASSERT_EQ(cache.get(2).first,2);
    ASSERT_EQ(cache.get(3).first,3);

    cache.put(4,4);

    ASSERT_FALSE(cache.get(1).second);
    ASSERT_TRUE(cache.get(4).second);
}

TEST(util_lru_cache, exists_predicates) {
    eu::lru_cache<int,int> cache(3);
    cache.put(1,1);
    cache.put(2,2);
    cache.put(3,3);

    // 3:3 2:2 1:1
    int rv;

    auto update = [&rv](int& x) { x*=2; rv=x; };
    auto remove = [](int const&){ return true; };

    ASSERT_TRUE(cache.exists(1,update));
    ASSERT_EQ(rv,2);
    ASSERT_TRUE(cache.exists(3,update));
    ASSERT_EQ(rv,6);
    ASSERT_TRUE(cache.exists(2,update));
    ASSERT_EQ(rv,4);

    // 3:6 2:4 1:2
    ASSERT_FALSE(cache.exists(2, update, remove));
    ASSERT_EQ(rv,4);
}

TEST(util_lru_cache, update) {
    eu::lru_cache<int,int> cache(3);
    cache.put(1,1);
    cache.put(2,2);
    cache.put(3,3);

    // 3:3 2:2 1:1
    ASSERT_TRUE(cache.exists(1));
    ASSERT_TRUE(cache.exists(2));
    ASSERT_TRUE(cache.exists(3));


    auto rv = cache.get(3,[](int& x){
        x = 300;
    });

    // 3:300 2:2 1:1
    ASSERT_TRUE(rv.second);
    ASSERT_EQ(rv.first, 300);

    cache.put(4,4);

    // 4:4 3:300 2:2
    ASSERT_FALSE(cache.exists(1));

    cache.get(3,[](int&){},[](int const& x) {
        return x == 300;
    });

    // 4:4 2:2
    ASSERT_EQ(cache.size(), 2);
    ASSERT_TRUE(cache.exists(2));
    ASSERT_TRUE(cache.exists(4));
}

TEST(util_lru_cache, remove) {
    eu::lru_cache<int,int> cache(3);
    cache.put(1,1);
    cache.put(2,2);
    cache.put(3,3);

    // 3:3 2:2 1:1
    ASSERT_EQ(cache.size(), 3);

    ASSERT_TRUE(cache.remove(2));
    ASSERT_FALSE(cache.remove(4));

    // 3:3 1:1
    ASSERT_EQ(cache.size(), 2);

    ASSERT_TRUE(cache.exists(1));
    ASSERT_TRUE(cache.exists(3));
}

TEST(util_lru_cache, remove_predicates) {
    eu::lru_cache<int,int> cache(3);
    cache.put(1,1);
    cache.put(2,2);
    cache.put(3,3);

    // 3:3 2:2 1:1
    ASSERT_EQ(cache.size(), 3);

    cache.remove([](int& x){
        return x > 300 ;
    });

    // 3:3 2:2 1:1
    ASSERT_EQ(cache.size(), 3);

    cache.remove([](int& x){
        return x < 300 ;
    });

    ASSERT_EQ(cache.size(), 0);
}
