// test_parsers.cc
//
// Copyright (C) 2010 Daniel Burrows
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License as
// published by the Free Software Foundation; either version 2 of the
// License, or (at your option) any later version.
//
// This program is distributed in the hope that it will be useful, but
// WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
// General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; see the file COPYING.  If not, write to
// the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
// Boston, MA 02111-1307, USA.

#include <generic/util/parsers.h>

#include <cppunit/extensions/HelperMacros.h>

#include <boost/numeric/conversion/cast.hpp>

using namespace parsers;

class ParsersTest : public CppUnit::TestFixture
{
  CPPUNIT_TEST_SUITE(ParsersTest);

  CPPUNIT_TEST(testParseChar);
  CPPUNIT_TEST(testParseAnyChar);
  CPPUNIT_TEST(testWhitespace);

  CPPUNIT_TEST_SUITE_END();

public:

  void testParseChar()
  {
    ch_p<char> comma(','), semicolon(';');

    std::string input(",;;,,,;a");

    std::string::const_iterator begin = input.begin(), end = input.end();

    CPPUNIT_ASSERT_THROW(semicolon.parse(begin, end), ParseException);
    CPPUNIT_ASSERT_EQUAL(0, begin - input.begin());

    CPPUNIT_ASSERT_EQUAL(',', comma.parse(begin, end));
    CPPUNIT_ASSERT_EQUAL(1, begin - input.begin());

    CPPUNIT_ASSERT_THROW(comma.parse(begin, end), ParseException);
    CPPUNIT_ASSERT_EQUAL(1, begin - input.begin());

    CPPUNIT_ASSERT_EQUAL(';', semicolon.parse(begin, end));
    CPPUNIT_ASSERT_EQUAL(2, begin - input.begin());

    CPPUNIT_ASSERT_EQUAL(';', semicolon.parse(begin, end));
    CPPUNIT_ASSERT_EQUAL(3, begin - input.begin());

    CPPUNIT_ASSERT_EQUAL(',', comma.parse(begin, end));
    CPPUNIT_ASSERT_EQUAL(4, begin - input.begin());

    CPPUNIT_ASSERT_THROW(semicolon.parse(begin, end), ParseException);
    CPPUNIT_ASSERT_EQUAL(4, begin - input.begin());

    CPPUNIT_ASSERT_EQUAL(',', comma.parse(begin, end));
    CPPUNIT_ASSERT_EQUAL(5, begin - input.begin());

    CPPUNIT_ASSERT_EQUAL(',', comma.parse(begin, end));
    CPPUNIT_ASSERT_EQUAL(6, begin - input.begin());

    CPPUNIT_ASSERT_EQUAL(';', semicolon.parse(begin, end));
    CPPUNIT_ASSERT_EQUAL(7, begin - input.begin());

    CPPUNIT_ASSERT_THROW(comma.parse(begin, end), ParseException);
    CPPUNIT_ASSERT_EQUAL(7, begin - input.begin());

    CPPUNIT_ASSERT_THROW(semicolon.parse(begin, end), ParseException);
    CPPUNIT_ASSERT_EQUAL(7, begin - input.begin());
  }

  void testParseAnyChar()
  {
    anychar_p<char> any;
    std::string input = "abcdefg";
    std::string::const_iterator begin = input.begin(), end = input.end();

    for(std::string::size_type i = 0; i < input.size(); ++i)
      {
        CPPUNIT_ASSERT_EQUAL(i, boost::numeric_cast<std::string::size_type>((begin - input.begin())));
        CPPUNIT_ASSERT_EQUAL(input[i], any.parse(begin, end));
      }

    CPPUNIT_ASSERT_EQUAL(input.size(), boost::numeric_cast<std::string::size_type>(begin - input.begin()));
  }

  void testWhitespace()
  {
    space_p sp(space());

    std::string input = " b";

    std::string::const_iterator begin = input.begin(), end = input.end();

    CPPUNIT_ASSERT_EQUAL(' ', sp.parse(begin, end));
    CPPUNIT_ASSERT_EQUAL(1, begin - input.begin());

    CPPUNIT_ASSERT_THROW(sp.parse(begin, end), ParseException);
    CPPUNIT_ASSERT_EQUAL(1, begin - input.begin());
  }
};

CPPUNIT_TEST_SUITE_REGISTRATION(ParsersTest);