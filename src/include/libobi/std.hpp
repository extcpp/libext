#pragma once
#include <vector>

namespace std {
    //add get for vector - be sure what you do when using it
    template<typename std::size_t _Int, typename _Type> constexpr
    _Type& get(vector<_Type>& _vec){
        return _vec[_Int];
    }
}
