// Copyright (C) 2014 Andrzej Krzemienski.
//
// Use, modification, and distribution is subject to the Boost Software
// License, Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
//
// See http://www.boost.org/lib/optional for documentation.
//
// You are welcome to contact the author at:
//  akrzemi1@gmail.com

#include "boost/optional/optional.hpp"

#ifdef BOOST_BORLANDC
#pragma hdrstop
#endif

using boost::optional;

struct X {};
struct Y {};
  
struct Resource
{
  explicit Resource(const X&) {}
};

static_assert(  std::is_constructible_v<Resource, const X&> );
static_assert( !std::is_constructible_v<Resource, const Y&> );

static_assert(  std::is_constructible_v<optional<Resource>, const X&> );
static_assert( !std::is_constructible_v<optional<Resource>, const Y&> || std::is_constructible_v<std::optional<Resource>, const Y&>);

static_assert(  std::is_constructible_v< optional< optional<int> >, optional<int> > );
static_assert( !std::is_constructible_v< optional<int>, optional< optional<int> > > );

static_assert(  std::is_constructible_v< optional< optional<int> >, const optional<int>& > );
static_assert( !std::is_constructible_v< optional<int>, const optional< optional<int> >& > );

static_assert(  std::is_constructible_v<optional<Resource>, const optional<X>&> || !std::is_constructible_v<std::optional<Resource>, const optional<X>&>);
static_assert( !std::is_constructible_v<optional<Resource>, const optional<Y>&> );
  
int main() { }
