// Copyright (C) 2016 Manuel A. Fernandez Montecelo
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

#ifndef CMDLINE_APT_PROXY_H
#define CMDLINE_APT_PROXY_H

#include "cmdline_common.h"

#include <memory>
#include <string>


/** \file cmdline_apt_proxy.h
 */

namespace aptitude
{
  namespace cmdline
  {
    enum class apt_proxy_command : int {
      source = 1
    };
  }
}


/** Execute apt/apt-get/apt-cache command directly
 *
 */
int cmdline_apt_proxy(int argc, char* argv[]);


#endif
