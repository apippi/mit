// download_manager.cc
//
//   Copyright (C) 2005 Daniel Burrows
//   Copyright (C) 2016 Manuel A. Fernandez Montecelo
//
//   This program is free software; you can redistribute it and/or
//   modify it under the terms of the GNU General Public License as
//   published by the Free Software Foundation; either version 2 of
//   the License, or (at your option) any later version.
//
//   This program is distributed in the hope that it will be useful,
//   but WITHOUT ANY WARRANTY; without even the implied warranty of
//   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
//   General Public License for more details.
//
//   You should have received a copy of the GNU General Public License
//   along with this program; see the file COPYING.  If not, write to
//   the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
//   Boston, MA 02111-1307, USA.

#include "download_manager.h"

download_manager::download_manager() :
  fetcher { nullptr }
{
}

download_manager::~download_manager()
{
  delete fetcher;
}

pkgAcquire::RunResult download_manager::do_download()
{
  return fetcher->Run();
}

pkgAcquire::RunResult download_manager::do_download(int PulseInterval)
{
  return fetcher->Run(PulseInterval);
}
