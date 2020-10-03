// Copyright (C) 2020, Daniela Engert
//
// Use, modification, and distribution is subject to the Boost Software
// License, Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
//
// See http://www.boost.org/libs/optional for documentation.
//
#pragma once
#include <optional>

#ifndef OPTIONAL_NONE_T_CPP20
#define OPTIONAL_NONE_T_CPP20

namespace boost {

struct none_t : std::nullopt_t {
	constexpr explicit none_t(std::nullopt_t) : std::nullopt_t{std::nullopt} {}
};

} // namespace boost

#endif
