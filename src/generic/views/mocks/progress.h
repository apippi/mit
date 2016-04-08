/** \file cmdline_progress_display.h */    // -*-c++-*-


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

#ifndef GENERIC_VIEWS_MOCKS_PROGRESS_H
#define GENERIC_VIEWS_MOCKS_PROGRESS_H

// Local includes:
#include <generic/views/progress.h>

// System includes:
#include <gmock/gmock.h>

namespace aptitude
{
  namespace views
  {
    namespace mocks
    {
      /** \brief Mock implementation of views::progress */
      class progress : public aptitude::views::progress
      {
      public:
        MOCK_METHOD1(set_progress, void(const aptitude::util::progress_info &));

        MOCK_METHOD0(done, void());
      };
    }
  }
}

#endif // GENERIC_VIEWS_MOCKS_PROGRESS_H
