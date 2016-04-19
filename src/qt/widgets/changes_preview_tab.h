/** \file changes_preview_tab.h */   // -*-c++-*-
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

#ifndef APTITUDE_QT_CHANGES_PREVIEW_TAB_H
#define APTITUDE_QT_CHANGES_PREVIEW_TAB_H

#include "tab.h"

namespace aptitude
{
  namespace gui
  {
    namespace qt
    {
      /** \brief Tab showing detailed informations about requested
       *  packages' changes.
       */
      class changes_preview_tab : public tab
      {
        Q_OBJECT

      public:
	/** \brief Create a new changes_preview_tab object. */
	explicit changes_preview_tab(QWidget *parent = 0);

	/** \brief Destroy a changes_preview_tab object. */
	virtual ~changes_preview_tab();
      };
    }
  }
}

#endif // APTITUDE_QT_CHANGES_PREVIEW_TAB_H
