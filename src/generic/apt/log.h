// log.h                                          -*-c++-*-
//
//   Copyright (C) 2005 Daniel Burrows
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
//   the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
//   Boston, MA 02111-1307, USA.
//

#ifndef LOG_H
#define LOG_H

/** \brief A routine to write aptitude's automatic installation log.
 *
 *  \file log.h
 */

/** Look up the log file's location in the apt configuration, and
 *  write a log stanza to it.
 */
void log_changes();

#endif
