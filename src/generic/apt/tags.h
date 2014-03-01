// tags.h                                            -*-c++-*-
//
//   Copyright (C) 2005, 2007, 2010 Daniel Burrows
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

#ifndef TAGS_H
#define TAGS_H

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

// If ept is unavailable, we use our own (broken!) code to build an
// in-memory database of package tags.  Otherwise, this code just
// handles initializing it, destroying it, and extracting information
// from it.  Note that this means that all callers have to be
// conditionalized on HAVE_EPT: the "tag" class this used to return is
// broken wrt hierarchies and just using ept is simpler.

#ifndef HAVE_EPT

#include <set>
#include <string>

#include <apt-pkg/pkgcache.h>

/** \brief A parser for tags.
 * 
 *  \file tags.h
 */

class OpProgress;

class tag
{
  std::string s;

  int cmp(const tag &other) const;
public:
  class const_iterator
  {
    std::string::const_iterator start, finish, limit;

    friend class tag;
  public:
    const_iterator(const std::string::const_iterator &_start,
		   const std::string::const_iterator &_finish,
		   const std::string::const_iterator &_limit)
      :start(_start), finish(_finish), limit(_limit)
    {
    }

    const_iterator &operator++();

    const_iterator &operator=(const const_iterator &other)
    {
      start = other.start;
      finish = other.finish;
      limit = other.limit;

      return *this;
    }

    bool operator==(const const_iterator &other) const
    {
      return start == other.start && finish == other.finish && limit == other.limit;
    }

    bool operator!=(const const_iterator &other) const
    {
      return start != other.start || finish != other.finish || limit != other.limit;
    }

    std::string operator*() const
    {
      return std::string(start, finish);
    }
  };

  tag(std::string::const_iterator _start,
      std::string::const_iterator _finish);

  tag &operator=(const tag &other)
  {
    s = other.s;

    return *this;
  }

  bool operator<(const tag &other) const
  {
    return cmp(other) < 0;
  }

  bool operator<=(const tag &other) const
  {
    return cmp(other) <= 0;
  }

  bool operator==(const tag &other) const
  {
    return cmp(other) == 0;
  }

  bool operator!=(const tag &other) const
  {
    return cmp(other) != 0;
  }

  bool operator>(const tag &other) const
  {
    return cmp(other) > 0;
  }

  bool operator>=(const tag &other) const
  {
    return cmp(other) >= 0;
  }

  const_iterator begin() const;
  const_iterator end() const
  {
    return const_iterator(s.end(), s.end(), s.end());
  }

  std::string str() const
  {
    return s;
  }
};

class tag_list
{
  // The string to parse.
  std::string s;
public:
  class const_iterator
  {
    std::string::const_iterator start, finish, limit;
  public:
    const_iterator(const std::string::const_iterator &_start,
		   const std::string::const_iterator &_finish,
		   const std::string::const_iterator &_limit)
      :start(_start), finish(_finish), limit(_limit)
    {
    }

    const_iterator operator=(const const_iterator &other)
    {
      start = other.start;
      finish = other.finish;
      limit = other.limit;

      return *this;
    }

    bool operator==(const const_iterator &other)
    {
      return other.start == start && other.finish == finish && other.limit == limit;
    }

    bool operator!=(const const_iterator &other)
    {
      return other.start != start || other.finish != finish || other.limit != limit;
    }

    const_iterator &operator++();

    tag operator*()
    {
      return tag(start, finish);
    }
  };

  tag_list(const char *start, const char *finish)
    :s(start, finish)
  {
  }

  tag_list &operator=(const tag_list &other)
  {
    s=other.s;

    return *this;
  }

  const_iterator begin() const;
  const_iterator end() const
  {
    return const_iterator(s.end(), s.end(), s.end());
  }
};

// Grab the tags for the given package:
const std::set<tag> *get_tags(const pkgCache::PkgIterator &pkg);

// Load tags for all packages (call before get_tags)
void load_tags(OpProgress &progress);



// Interface to the tag vocabulary file; tag vocabularies are assumed
// to not change over time.
std::string facet_description(const std::string &facet);

// Here "Tag" is a fully qualified tag name.
std::string tag_description(const std::string &tag);

#else // HAVE_EPT

#include <apt-pkg/pkgcache.h>

#include <ept/debtags/debtags.h>

#include <set>

namespace aptitude
{
  namespace apt
  {
#ifdef EPT_DEBTAGS_GETTAGSOFITEM_RETURNS_STRINGS
    typedef std::string tag;
    inline std::string get_fullname(const std::string &t)
    {
      return t;
    }
#else
    // Probably means a new version of libept does something the
    // configure checks can't recognize.
#error "Don't know how to represent a debtags tag."
#endif

    const std::set<tag> get_tags(const pkgCache::PkgIterator &pkg);

    /** \brief Initialize the cache of debtags information. */
    void load_tags();

    /** \brief Get the name of the facet corresponding to a tag. */
    std::string get_facet_name(const tag &t);

    /** \brief Get the name of a tag (the full name minus the facet). */
    std::string get_tag_name(const tag &t);

    /** \brief Get the short description of a tag. */
    std::string get_tag_short_description(const tag &t);

    /** \brief Get the long description of a tag. */
    std::string get_tag_long_description(const tag &t);

    // \note This interface could be more efficient if it just used
    // facet names like libept does.  Using tags is a concession to
    // backwards compatibility (it's hard to implement one interface
    // that covers both cases without a lot of cruft).  In any event,
    // this shouldn't be called enough to matter.

    /** \brief Get the short description of the facet corresponding to a tag. */
    std::string get_facet_short_description(const tag &t);

    /** \brief Get the long description of the facet corresponding to a tag. */
    std::string get_facet_long_description(const tag &t);
  }
}

#endif // HAVE_EPT

#endif
