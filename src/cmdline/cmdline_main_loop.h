// cmdline_main_loop.h           -*-c++-*-
//
// Copyright (C) 2009 Daniel Burrows
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

#ifndef CMDLINE_MAIN_LOOP_H
#define CMDLINE_MAIN_LOOP_H

/** \brief A dummy main loop used to safely invoke processes that
 *  run in the background from the command-line.
 *
 * \file cmdline_main_loop.h
 */

#include <cwidget/generic/threads/event_queue.h>

#include <sigc++/slot.h>

namespace aptitude
{
  namespace cmdline
  {
    /** \brief Run the dummy main loop; exits immediately if the loop
     *  is already running in this thread or in another thread.
     */
    void main_loop();

    /** \brief Add the given thunk to the queue for the currently
     *  running main thread.
     */
    void post_thunk(const sigc::slot<void> &thunk);

    /** \brief If the main loop is running (in any thread), ask it to
     *  exit.
     */
    void exit_main();
  }
}

#endif // CMDLINE_MAIN_LOOP_H
