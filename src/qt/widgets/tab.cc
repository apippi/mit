/** \file tab.cc */
//
// Copyright (C) 2010 Piotr Galiszewski
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

#include "tab.h"

namespace aptitude
{
  namespace gui
  {
    namespace qt
    {
      tab::tab(tab_type _type, closing_policy _policy, QWidget *parent)
	: QWidget(parent), type(_type), policy(_policy)
      {

      }

      tab::~tab()
      {
      }
    }
  }
}

#include "tab.moc"
