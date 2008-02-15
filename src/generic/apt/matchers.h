// matchers.h  -*-c++-*-
//
//  Copyright 2000-2001, 2005, 2007-2008 Daniel Burrows
//
//  This program is free software; you can redistribute it and/or modify
//  it under the terms of the GNU General Public License as published by
//  the Free Software Foundation; either version 2 of the License, or
//  (at your option) any later version.
//
//  This program is distributed in the hope that it will be useful,
//  but WITHOUT ANY WARRANTY; without even the implied warranty of
//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//  GNU General Public License for more details.
//
//  You should have received a copy of the GNU General Public License
//  along with this program; see the file COPYING.  If not, write to
//  the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
//  Boston, MA 02111-1307, USA.
//
//  Provides support for complex string-based matching of packages.  Patterns
// can be compiled for later use or used and discarded.

#ifndef MATCHERS_H
#define MATCHERS_H

#include <memory>
#include <string>
#include <vector>

#include <apt-pkg/pkgcache.h>

class aptitudeDepCache;
class pkgRecords;

namespace aptitude
{
  namespace matching
  {
    /* For the cases where you want to investigate just what you matched a
     * bit more thoroughly.
     *
     * A "group" is a string associated with this match.  Matches may have
     * any nonnegative number of groups, including 0 (in the case that
     * there's no interesting string associated with the match).
     */
    class pkg_match_result
    {
    public:
      virtual unsigned int num_groups()=0;
      virtual const std::string &group(unsigned int n)=0;
      virtual ~pkg_match_result();
    };

    // pkg_matcher is declared with the minimum information a user
    // needs to know in order to use it (its destructor).  The actual
    // match methods are hidden because they take extra arguments that
    // need to be bootstrapped; providing overrides would lead to
    // confusion at the client and interface level, and also would
    // require this header to change whenever the guts of the matching
    // code changed.
    class pkg_matcher
    {
    public:
      virtual ~pkg_matcher();
    };

    /** \brief Apply a matcher to the given apt package
     *  and package version.
     *
     *  \param matcher  the matcher to apply.
     *  \param pkg      the package to test against matcher.
     *  \param ver      the package version (possibly none) to
     *                  test against matcher.
     *  \param cache    the package cache in whose context this
     *                  match should be executed.
     *  \param records  the package records structure corresponding
     *                  to the given cache.
     *
     *  \return \b true if the given package and version are selected
     *                  by matcher.
     */
    bool apply_matcher(pkg_matcher *matcher,
		       const pkgCache::PkgIterator &pkg,
		       const pkgCache::VerIterator &ver,
		       aptitudeDepCache &cache,
		       pkgRecords &records);


    /** \brief Apply a matcher to the given apt package.
    *
     *  \param matcher  the matcher to apply.
     *  \param pkg      the package to test against matcher.
     *  \param ver      the package version (possibly none) to
     *                  test against matcher.
     *  \param cache    the package cache in whose context this
     *                  match should be executed.
     *  \param records  the package records structure corresponding
     *                  to the given cache.
     *
     *  \return \b true if any version of the given package is
     *  selected by matcher.
     */
    bool apply_matcher(pkg_matcher *matcher,
		       const pkgCache::PkgIterator &pkg,
		       aptitudeDepCache &cache,
		       pkgRecords &records);


    /** \brief Apply a matcher to the given apt package and package
     *  version and return the matched region.
     *
     *  \param matcher  the matcher to apply.
     *  \param pkg      the package to test against matcher.
     *  \param ver      the package version (possibly none) to
     *                  test against matcher.
     *  \param cache    the package cache in whose context this
     *                  match should be executed.
     *  \param records  the package records structure corresponding
     *                  to the given cache.
     *
     *  \return a non-NULL pkg_match_result if the given package and
     *  version are selected by matcher, NULL otherwise.  If a
     *  non-NULL value is returned, the caller is responsible for
     *  deleting it.
     */
    pkg_match_result *get_match(pkg_matcher *matcher,
				const pkgCache::PkgIterator &pkg,
				const pkgCache::VerIterator &ver,
				aptitudeDepCache &cache,
				pkgRecords &records);

    /** \brief Apply a matcher to the given apt package and return the
     * matched region.
     *
     *  \param matcher  the matcher to apply.
     *  \param pkg      the package to test against matcher.
     *  \param cache    the package cache in whose context this
     *                  match should be executed.
     *  \param records  the package records structure corresponding
     *                  to the given cache.
     *
     *  \return a non-NULL pkg_match_result if the given package and
     *  version are selected by matcher, NULL otherwise.  If a
     *  non-NULL value is returned, the caller is responsible for
     *  deleting it.
     */
    pkg_match_result *get_match(pkg_matcher *matcher,
				const pkgCache::PkgIterator &pkg,
				aptitudeDepCache &cache,
				pkgRecords &records);

    /** Parse the given pattern, returning a matcher.  start will be
     *  modified to point to the first unparsable location.  The parse
     *  will terminate when a closing parenthesis or an element of
     *  terminators is encountered at the top syntax level.
     *
     *  \param start the beginning of the range to parse
     *  \param end the end of the range to parse
     *  \param terminators a list of strings whose presence at the top
     *                     syntactic level should cause the parse to
     *                     terminate.  It is assumed that none of these
     *                     strings contain whitespace.
     *  \param search_descriptions if \b true, then strings without an
     *                     explicit pattern escape will be used to search
     *                     both names and descriptions rather than
     *                     names alone.
     *  \param flag_errors if \b true, then error messages will be generated
     *                     for incorrect patterns.
     *  \param require_full_parse if \b true, then an error will be signalled
     *                            if the entire range [start,end) can't be
     *                            parsed.
     *
     *  \return the new matcher or \b NULL if an error occurs.
     */
    pkg_matcher *parse_pattern(std::string::const_iterator &start,
			       const std::string::const_iterator &end,
			       const std::vector<const char *> &terminators = std::vector<const char *>(),
			       bool search_descriptions=false,
			       bool flag_errors=true,
			       bool require_full_parse=true);

    inline pkg_matcher *parse_pattern(const std::string &s,
				      const std::vector<const char *> &terminators,
				      bool search_descriptions = false,
				      bool flag_errors = true)
    {
      std::string::const_iterator start = s.begin();
      return parse_pattern(start, s.end(), terminators,
			   search_descriptions, flag_errors,
			   true);
    }

    inline pkg_matcher *parse_pattern(const std::string &s,
				      bool search_descriptions = false,
				      bool flag_errors = true)
    {
      std::string::const_iterator start = s.begin();
      return parse_pattern(start, s.end(),
			   std::vector<const char *>(),
			   search_descriptions, flag_errors, true);
    }

    /** \brief Return a matcher that matches the given package.
     *
     *  This is useful for code that uses matchers to store some sort
     *  of criterion, and that for its own reasons wants to match a
     *  particular package.
     */
    pkg_matcher *make_const_matcher(const pkgCache::PkgIterator &pkg);

    inline bool pkg_matches(const string &s,
			    const pkgCache::PkgIterator &pkg,
			    const pkgCache::VerIterator &ver,
			    aptitudeDepCache &cache,
			    pkgRecords &records)
    {
      std::string::const_iterator start=s.begin();
      std::auto_ptr<pkg_matcher> m(parse_pattern(start, s.end()));
      if(m.get() == NULL)
	return false;
      else
	{
	  bool rval = apply_matcher(m.get(), pkg, ver, cache, records);
	  return rval;
	}
    }
  }
}

#endif
