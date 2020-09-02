// Copyright (C) 2003, Fernando Luis Cacciola Carballal.
// Copyright (C) 2014, 2015 Andrzej Krzemienski.
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
//
// See http://www.boost.org/libs/optional for documentation.
//
// You are welcome to contact the author at:
//  fernando_cacciola@hotmail.com
//
#ifndef OPTIONAL_NONE_CPP20
#define OPTIONAL_NONE_CPP20

#include <optional>

namespace boost {

using none_t = std::nullopt_t;
inline constexpr none_t none{ std::nullopt };

} // namespace boost

#endif // header guard

