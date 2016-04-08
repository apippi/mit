// apt_undo_tree.cc
//
//   Copyright (C) 2005 Daniel Burrows
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

#include "apt_undo_tree.h"

#include "ui.h"

#include <generic/apt/apt.h>

#include <generic/util/undo.h>

apt_undo_tree::apt_undo_tree()
{
}

bool apt_undo_tree::undo_undo_enabled()
{
  return apt_undos != NULL && apt_undos->size() > 0;
}

bool apt_undo_tree::undo_undo()
{
  if(apt_undos != NULL && apt_undos->size() > 0)
    {
      apt_undos->undo();
      package_states_changed();
      return true;
    }
  else
    return false;
}
