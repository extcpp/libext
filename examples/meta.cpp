// Copyright - 2020 - Jan Christoph Uhde <Jan@UhdeJC.com>
// Please see LICENSE.md for license or visit https://github.com/extcpp/basics
#include <iostream>

#include <ext/meta/is_one_of.hpp>

// using namespace std;
using namespace ext::meta;

struct leet {
    using type = void;
    static const int value = 1337;
};

struct unleet {};

int main() {
    std::cout << "is_one_of<int,double,float,int>(): " << is_one_of<int, double, float, int>() << std::endl;
    std::cout << "is_one_of<int,double,float>():     " << is_one_of<int, double, float>() << std::endl;
    std::cout << "is_one_of<void_t<int>,int,void>(): " << is_one_of<std::void_t<int>, int, void>() << std::endl;

    return 0;
}
