// Copyright - 2020 - Jan Christoph Uhde <Jan@UhdeJC.com>
// Please see LICENSE.md for license or visit https://github.com/extcpp/basics
#ifndef EXT_UTIL_BIT_TRICKS_HEADER
#define EXT_UTIL_BIT_TRICKS_HEADER
#include <ext/util/cast.hpp>

#include <type_traits>
#include <bitset>
#include <string>

namespace ext::util {


/// returns number stripped of all bits, but the least significant.
template<typename T>
constexpr inline T lsb(T number) {
    static_assert(-1 == ~0, "not 2's complement");

    if constexpr(std::is_signed_v<T>) {
        return (number & -number);
    } else {
        using siged_type = std::make_signed_t<T>;
        siged_type const signed_number = convert_checked<siged_type>(number);
        siged_type const result = (signed_number & -signed_number);
        return convert_checked<T>(result);
    }
}

template<typename T
        ,typename = std::enable_if_t<std::is_integral_v<T> && std::is_unsigned_v<T>>
        >
inline constexpr bool is_power_of_two(T number) {
    // alignment is expected to be a power of 2
    return (number != 0) && ((number & (number - 1)) == 0);
}

template<typename T, typename = std::enable_if_t<std::is_integral_v<T>>>
std::string encode_binary(T integer) {
    return std::bitset<sizeof(T) * 8>(integer).to_string();
}

} // namespace ext::util
#endif // EXT_UTIL_BIT_TRICKS_HEADER
