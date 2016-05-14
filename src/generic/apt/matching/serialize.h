// serialize.h      -*-c++-*-
//
//   Copyright (C) 2008-2009 Daniel Burrows
//   Copyright (C) 2014-2016 Manuel A. Fernandez Montecelo
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

#ifndef SERIALIZE_H
#define SERIALIZE_H

/** \file serialize.h */

#include <cwidget/generic/util/ref_ptr.h>

#include <iosfwd>
#include <string>

namespace aptitude
{
  namespace matching
  {
    class pattern;

    /** \brief Output a string that will parse to the given
     *  pattern.
     *
     *  \param p   The pattern to serialize.
     *  \param out Where to write the serialized string.
     *             The serialization will be appended to out.
     */
    void serialize_pattern(const cwidget::util::ref_ptr<pattern> &p,
			   std::string &out);

    /** \brief Output a string that will parse to the given
     *  pattern.
     *
     *  \param p   The pattern to serialize.
     *  \param out Where to write the serialized string.
     */
    void serialize_pattern(const cwidget::util::ref_ptr<pattern> &p,
			   std::ostream &out);

    inline std::ostream &operator<<(std::ostream &out, const cwidget::util::ref_ptr<pattern> &p)
    {
      serialize_pattern(p, out);
      return out;
    }

    /** \brief Return a string that will parse to the given pattern.
     *
     *  \param p   The pattern to serialize.
     */
    std::string serialize_pattern(const cwidget::util::ref_ptr<pattern> &p);
  }
}

#endif
