//
// ©2026 by Simone Tellini - https://tellini.info
//
// Licensed under the Creative Commons Attribution-NonCommercial-ShareAlike 4.0
// International License (CC BY-NC-SA 4.0).
//

#ifndef FRIENDLYBOT_ENUMUTILS_H
#define FRIENDLYBOT_ENUMUTILS_H

#include <type_traits>

template<typename T>
constexpr auto to_underlying( T value ) noexcept
{
    if constexpr( std::is_enum_v<T> )
        return static_cast<std::underlying_type_t<T>>( value );
    else if constexpr( std::is_integral_v<T> )
        return value;
    else
        static_assert( std::is_enum_v<T> || std::is_integral_v<T>, "Invalid type" );
}

#endif //FRIENDLYBOT_ENUMUTILS_H