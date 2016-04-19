// cmdline_dump_resolver.cc
//
//   Copyright (C) 2005 Daniel Burrows
//   Copyright (C) 2016 Manuel A. Fernandez Montecelo

//   This program is free software; you can redistribute it and/or
//   modify it under the terms of the GNU General Public License as
//   published by the Free Software Foundation; either version 2 of
//   the License, or (at your option) any later version.

//   This program is distributed in the hope that it will be useful,
//   but WITHOUT ANY WARRANTY; without even the implied warranty of
//   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
//   General Public License for more details.

//   You should have received a copy of the GNU General Public License
//   along with this program; see the file COPYING.  If not, write to
//   the Free Software Foundation, Inc., 51 Franklin St, Fifth Floor,
//   Boston, MA 02110-1301, USA.
//
// Just print out the current resolver state (debugging tool)

#include "cmdline_dump_resolver.h"

#include <generic/apt/aptitude_resolver_universe.h>
#include <generic/problemresolver/dump_universe.h>

#include <apt-pkg/error.h>

using namespace std;

int cmdline_dump_resolver(int argc, char *argv[],
			  const char *status_fname)
{
  _error->DumpErrors();

  OpProgress progress;
  bool operation_needs_lock = true;
  apt_init(&progress, true, operation_needs_lock, status_fname);

  if(_error->PendingError())
    {
      _error->DumpErrors();
      return -1;
    }

  aptitude_universe u(*apt_cache_file);

  dump_universe(u, cout);

  return 0;
}
