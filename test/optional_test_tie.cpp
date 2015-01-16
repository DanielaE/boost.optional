// Copyright (C) 2003, Fernando Luis Cacciola Carballal.
// Copyright (C) 2015 Andrzej Krzemienski.
//
// Use, modification, and distribution is subject to the Boost Software
// License, Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
//
// See http://www.boost.org/lib/optional for documentation.
//
// You are welcome to contact the author at:
//  fernando_cacciola@hotmail.com

#include "boost/optional/optional.hpp"

#ifdef __BORLANDC__
#pragma hdrstop
#endif

#include "boost/core/lightweight_test.hpp"
#include "boost/none.hpp"
#include "boost/tuple/tuple.hpp"

struct counting_oracle
{
  int val;
  counting_oracle() : val() { ++default_ctor_count; }
  counting_oracle(int v) : val(v) { ++val_ctor_count; }
  counting_oracle(const counting_oracle& rhs) : val(rhs.val) { ++copy_ctor_count; }
  counting_oracle& operator=(const counting_oracle& rhs) { val = rhs.val; ++copy_assign_count; return *this; }
  ~counting_oracle() { ++dtor_count; }
  
  static int dtor_count;
  static int default_ctor_count;
  static int val_ctor_count;
  static int copy_ctor_count;
  static int copy_assign_count;
  static int equals_count;
  
  friend bool operator==(const counting_oracle& lhs, const counting_oracle& rhs) { ++equals_count; return lhs.val == rhs.val; }
};

int counting_oracle::dtor_count = 0;
int counting_oracle::default_ctor_count = 0;
int counting_oracle::val_ctor_count = 0;
int counting_oracle::copy_ctor_count = 0;
int counting_oracle::copy_assign_count = 0;
int counting_oracle::equals_count = 0;

int count_copy_ctors_on_copy() // checks if we have copy elision
{
  counting_oracle::copy_ctor_count = 0;
  
  counting_oracle c(1);
  counting_oracle c2(c);
  int ans = counting_oracle::copy_ctor_count;
  counting_oracle::copy_ctor_count = 0;
  counting_oracle::val_ctor_count = 0;
  return ans;
}

// Test boost::tie() interoperability.
int main()
{
  int copy_factor = count_copy_ctors_on_copy();
  const std::pair<counting_oracle, counting_oracle> pair(1, 2);
  boost::optional<counting_oracle> o1, o2;
  boost::tie(o1, o2) = pair;
  
  BOOST_TEST(o1);
  BOOST_TEST(o2);
  BOOST_TEST(*o1 == counting_oracle(1));
  BOOST_TEST(*o2 == counting_oracle(2));
  BOOST_TEST_EQ(2 * copy_factor, counting_oracle::copy_ctor_count);
  BOOST_TEST_EQ(0, counting_oracle::copy_assign_count);
  BOOST_TEST_EQ(0, counting_oracle::default_ctor_count);

  return boost::report_errors();
}


