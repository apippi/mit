/** \file test_transient_message.cc */


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

// Local includes:
#include <cmdline/mocks/teletype.h>
#include <cmdline/mocks/terminal.h>
#include <cmdline/terminal.h>
#include <cmdline/transient_message.h>

// Global includes:
#include <gmock/gmock.h>
#include <gtest/gtest.h>

namespace mocks = aptitude::cmdline::mocks;

using aptitude::cmdline::create_transient_message;
using aptitude::cmdline::mocks::StrTrimmedRightEq;
using aptitude::cmdline::transient_message;
using boost::shared_ptr;
using testing::InSequence;
using testing::Return;
using testing::Test;
using testing::_;

namespace
{
  // We'll pretend that '=' is two columns wide.
  const wchar_t two_column_char = L'=';

  struct TransientMessage : public Test
  {
    shared_ptr<mocks::terminal> term;
    shared_ptr<mocks::terminal_locale> term_locale;
    shared_ptr<mocks::teletype> teletype;
    shared_ptr<transient_message> message;
    std::wstring widechar;

    TransientMessage()
      : term(mocks::create_combining_terminal()),
        term_locale(mocks::terminal_locale::create()),
        teletype(mocks::create_teletype(term, term_locale)),
        message(create_transient_message(term, term_locale)),
        widechar(1, two_column_char)
    {
      EXPECT_CALL(*term, get_screen_width())
        .WillRepeatedly(Return(80));

      EXPECT_CALL(*term_locale, wcwidth(two_column_char))
        .WillRepeatedly(Return(2));

      // The tests should never scroll past the first line (that's the
      // whole point of the transient message object, after all).
      EXPECT_CALL(*teletype, newline())
        .Times(0);
    }
  };
}

TEST_F(TransientMessage, SetText)
{
  EXPECT_CALL(*teletype, set_last_line(StrTrimmedRightEq(L"abc")));

  message->set_text(L"abc");
}

TEST_F(TransientMessage, ClearText)
{
  EXPECT_CALL(*teletype, set_last_line(StrTrimmedRightEq("abc")));
  EXPECT_CALL(*teletype, set_last_line(StrTrimmedRightEq("")));

  message->set_text(L"abc");
  message->set_text(L"");
}

TEST_F(TransientMessage, ReplaceTextWithShorter)
{
  EXPECT_CALL(*teletype, set_last_line(StrTrimmedRightEq(L"abc")));
  EXPECT_CALL(*teletype, set_last_line(StrTrimmedRightEq(L"a")));

  message->set_text(L"abc");
  message->set_text(L"a");
}

TEST_F(TransientMessage, ReplaceTextWithSameLength)
{
  EXPECT_CALL(*teletype, set_last_line(StrTrimmedRightEq(L"abc")));
  EXPECT_CALL(*teletype, set_last_line(StrTrimmedRightEq(L"xyz")));

  message->set_text(L"abc");
  message->set_text(L"xyz");
}

TEST_F(TransientMessage, ReplaceTextWithLonger)
{
  EXPECT_CALL(*teletype, set_last_line(StrTrimmedRightEq(L"abc")));
  EXPECT_CALL(*teletype, set_last_line(StrTrimmedRightEq(L"xyzw")));

  message->set_text(L"abc");
  message->set_text(L"xyzw");
}

TEST_F(TransientMessage, ReplaceWideCharTextWithShorter)
{
  EXPECT_CALL(*teletype, set_last_line(StrTrimmedRightEq(widechar + widechar + widechar)));
  EXPECT_CALL(*teletype, set_last_line(StrTrimmedRightEq(L"xyz")));

  message->set_text(widechar + widechar + widechar);
  message->set_text(L"xyz");
}

TEST_F(TransientMessage, ReplaceWideCharTextWithSameLength)
{
  EXPECT_CALL(*teletype, set_last_line(StrTrimmedRightEq(widechar + widechar)));
  EXPECT_CALL(*teletype, set_last_line(StrTrimmedRightEq(L"abcd")));

  message->set_text(widechar + widechar);
  message->set_text(L"abcd");
}

TEST_F(TransientMessage, ReplaceWideCharTextWithLonger)
{
  EXPECT_CALL(*teletype, set_last_line(StrTrimmedRightEq(widechar)));
  EXPECT_CALL(*teletype, set_last_line(StrTrimmedRightEq(L"abc")));

  message->set_text(widechar);
  message->set_text(L"abc");
}

TEST_F(TransientMessage, TruncateLongLine)
{
  EXPECT_CALL(*term, get_screen_width())
    .WillRepeatedly(Return(4));

  EXPECT_CALL(*teletype, set_last_line(StrTrimmedRightEq("abcd")));

  message->set_text(L"abcdefghijklmnopqrstuvwxyz");
}

TEST_F(TransientMessage, ReplaceTruncatedLongLineWithNonTruncated)
{
  EXPECT_CALL(*term, get_screen_width())
    .WillRepeatedly(Return(4));

  EXPECT_CALL(*teletype, set_last_line(StrTrimmedRightEq("abcd")));
  EXPECT_CALL(*teletype, set_last_line(StrTrimmedRightEq("xyz")));

  message->set_text(L"abcdefghijklmnopqrstuvwxyz");
  message->set_text(L"xyz");
}

TEST_F(TransientMessage, ReplaceTruncatedLongLineWithTruncated)
{
  EXPECT_CALL(*term, get_screen_width())
    .WillRepeatedly(Return(4));

  EXPECT_CALL(*teletype, set_last_line(StrTrimmedRightEq("abcd")));
  EXPECT_CALL(*teletype, set_last_line(StrTrimmedRightEq("zyxw")));

  message->set_text(L"abcdefghijklmnopqrstuvwxyz");
  message->set_text(L"zyxwvuts");
}

TEST_F(TransientMessage, TruncateWideCharLine)
{
  EXPECT_CALL(*term, get_screen_width())
    .WillRepeatedly(Return(4));

  EXPECT_CALL(*teletype, set_last_line(StrTrimmedRightEq(L"ab" + widechar)));

  message->set_text(L"ab" + widechar + L"cdef");
}

TEST_F(TransientMessage, TruncateWideCharLineWithSplit)
{
  EXPECT_CALL(*term, get_screen_width())
    .WillRepeatedly(Return(4));

  EXPECT_CALL(*teletype, set_last_line(StrTrimmedRightEq(L"abc")));

  message->set_text(L"abc" + widechar + L"def");
}

TEST_F(TransientMessage, ReplaceTruncatedWideCharLine)
{
  EXPECT_CALL(*term, get_screen_width())
    .WillRepeatedly(Return(4));

  EXPECT_CALL(*teletype, set_last_line(StrTrimmedRightEq(widechar + widechar)));
  EXPECT_CALL(*teletype, set_last_line(StrTrimmedRightEq(L"z")));

  message->set_text(widechar + widechar + L"abcdef");
  message->set_text(L"z");
}