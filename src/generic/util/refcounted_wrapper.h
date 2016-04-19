// refcounted_wrapper.h                     -*-c++-*-
//
//   Copyright (C) 2008-2009 Daniel Burrows
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
//   the Free Software Foundation, Inc., 51 Franklin St, Fifth Floor,
//   Boston, MA 02110-1301, USA.

#ifndef REFCOUNTED_WRAPPER_H
#define REFCOUNTED_WRAPPER_H

#include "refcounted_base.h"

// \todo Lift this to a more generic location.
namespace aptitude
{
  namespace util
  {
    /** \brief Wrap an arbitrary value in a class suitable for use
     *  with ref_ptr.
     */
    template<typename T>
    class refcounted_wrapper : public refcounted_base_threadsafe
    {
      T *p;

    public:
      refcounted_wrapper(T *_p)
	: p(_p)
      {
      }

      ~refcounted_wrapper()
      {
	delete p;
      }
    };
  }
}

#endif // REFCOUNTED_WRAPPER_H
