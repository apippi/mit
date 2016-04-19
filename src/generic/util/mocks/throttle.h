/** \file throttle.h */  // -*-c++-*-


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
// the Free Software Foundation, Inc., 51 Franklin St, Fifth Floor,
// Boston, MA 02110-1301, USA.

#ifndef APTITUDE_UTIL_MOCKS_THROTTLE_H
#define APTITUDE_UTIL_MOCKS_THROTTLE_H

// Local includes:
#include <generic/util/throttle.h>

// System includes:
#include <gmock/gmock.h>

namespace aptitude
{
  namespace util
  {
    namespace mocks
    {
      /** \brief Mock implementation of aptitude::util::throttle. */
      class throttle : public aptitude::util::throttle
      {
      public:
        MOCK_METHOD0(update_required, bool());
        MOCK_METHOD0(reset_timer, void());
      };
    }
  }
}

#endif // APTITUDE_UTIL_MOCKS_THROTTLE_H
