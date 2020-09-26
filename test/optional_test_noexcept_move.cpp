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


// these 4 classes have different noexcept signatures in move operations
struct NothrowBoth {
  NothrowBoth(NothrowBoth&&) noexcept(true) {};
  void operator=(NothrowBoth&&) noexcept(true) {};
};
struct NothrowCtor {
  NothrowCtor(NothrowCtor&&) noexcept(true) {};
  void operator=(NothrowCtor&&) noexcept(false) {};
};
struct NothrowAssign {
  NothrowAssign(NothrowAssign&&) noexcept(false) {};
  void operator=(NothrowAssign&&) noexcept(true) {};
};
struct NothrowNone {
  NothrowNone(NothrowNone&&) noexcept(false) {};
  void operator=(NothrowNone&&) noexcept(false) {};
};

#if 0 // these also test type_traits, which are wrong
void test_noexcept_as_defined() // this is a compile-time test
{
  static_assert(::boost::is_nothrow_move_constructible<NothrowBoth>::value);
  static_assert(::boost::is_nothrow_move_assignable<NothrowBoth>::value);
  
  static_assert(::boost::is_nothrow_move_constructible<NothrowCtor>::value);
  static_assert(!::boost::is_nothrow_move_assignable<NothrowCtor>::value);
  
  static_assert(!::boost::is_nothrow_move_constructible<NothrowAssign>::value);
  static_assert(::boost::is_nothrow_move_assignable<NothrowAssign>::value);
  
  static_assert(!::boost::is_nothrow_move_constructible<NothrowNone>::value);
  static_assert(!::boost::is_nothrow_move_assignable<NothrowNone>::value);
}

void test_noexcept_on_optional_with_type_traits() // this is a compile-time test
{
  static_assert(::boost::is_nothrow_move_constructible<optional<NothrowBoth> >::value);
  static_assert(::boost::is_nothrow_move_assignable<optional<NothrowBoth> >::value);
  static_assert(noexcept(optional<NothrowBoth>()));
    
  static_assert(::boost::is_nothrow_move_constructible<optional<NothrowCtor> >::value);
  static_assert(!::boost::is_nothrow_move_assignable<optional<NothrowCtor> >::value);
  static_assert(noexcept(optional<NothrowCtor>()));
    
  static_assert(!::boost::is_nothrow_move_constructible<optional<NothrowAssign> >::value);
  static_assert(!::boost::is_nothrow_move_assignable<optional<NothrowAssign> >::value);
  static_assert(noexcept(optional<NothrowAssign>()));
    
  static_assert(!::boost::is_nothrow_move_constructible<optional<NothrowNone> >::value);
  static_assert(!::boost::is_nothrow_move_assignable<optional<NothrowNone> >::value);
  static_assert(noexcept(optional<NothrowNone>()));
}
#endif

void test_noexcept_optional_with_operator() // compile-time test
{
  typedef optional<NothrowBoth>   ONx2;
  typedef optional<NothrowCtor>   ONxC;
  typedef optional<NothrowAssign> ONxA;
  typedef optional<NothrowNone>   ONx0;
  ONx2 onx2;
  ONxC onxC;
  ONxA onxA;
  ONx0 onx0;
  
  static_assert( noexcept( ONx2() ));
  static_assert( noexcept( ONx2(std::move(onx2)) ));
  static_assert( noexcept( onx2 = ONx2() )); 
  
  static_assert( noexcept( ONxC() ));
  static_assert( noexcept( ONxC(std::move(onxC)) ));
  static_assert(!noexcept( onxC = ONxC() ));
  
  static_assert( noexcept( ONxA() ));
  static_assert(!noexcept( ONxA(std::move(onxA)) ));
  static_assert(!noexcept( onxA = ONxA() ));
  
  static_assert( noexcept( ONx0() ));
  static_assert(!noexcept( ONx0(std::move(onx0)) ));
  static_assert(!noexcept( onx0 = ONx0() ));
}

int main()
{
  return 0;
}


