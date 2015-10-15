// cmdline_show.cc                               -*-c++-*-
//
// Copyright (C) 2004, 2010 Daniel Burrows
// Copyright (C) 2015 Manuel A. Fernandez Montecelo
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
// the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
// Boston, MA 02111-1307, USA.


// Local includes:
#include "cmdline_show.h"

#include <aptitude.h>
#include <desc_render.h>

#include "cmdline_common.h"
#include "cmdline_util.h"
#include "terminal.h"
#include "text_progress.h"

#include <generic/apt/apt.h>
#include <generic/apt/config_signal.h>
#include <generic/apt/matching/match.h>
#include <generic/apt/matching/parse.h>
#include <generic/apt/matching/pattern.h>


// System includes:
#include <cwidget/fragment.h>
#include <cwidget/generic/util/transcode.h>

#include <apt-pkg/error.h>
#include <apt-pkg/init.h>
#include <apt-pkg/pkgcache.h>
#include <apt-pkg/pkgrecords.h>
#include <apt-pkg/progress.h>
#include <apt-pkg/strutl.h>
#include <apt-pkg/version.h>

#include <iostream>

namespace cw = cwidget;
using aptitude::cmdline::create_terminal;
using aptitude::cmdline::make_text_progress;
using aptitude::cmdline::terminal_io;
using aptitude::cmdline::terminal_metrics;
using cwidget::fragf;
using cwidget::fragment;
using namespace std;

ostream &operator<<(ostream &out, const cwidget::fragment_contents &contents)
{
  for(cwidget::fragment_contents::const_iterator i=contents.begin();
      i!=contents.end(); ++i)
    {
      wstring s;
      // Drop the attributes.
      for(cwidget::fragment_line::const_iterator j=i->begin(); j!=i->end(); ++j)
	s.push_back((*j).ch);

      out << cw::util::transcode(s) << endl;
    }

  return out;
}

static cwidget::fragment *dep_lst_frag(pkgCache::DepIterator dep,
				       const string& title, pkgCache::Dep::DepType T)
{
  using cwidget::fragment;
  using cwidget::fragf;

  vector<cw::fragment *> fragments;

  while(!dep.end())
    {
      pkgCache::DepIterator start, end;
      dep.GlobOr(start, end);

      vector<cw::fragment *> or_fragments;

      if(start->Type==T)
	do
	  {
	    cw::fragment *verfrag;

	    if(start.TargetVer())
	      verfrag=cw::fragf(" (%s %s)", start.CompType(), start.TargetVer());
	    else
	      verfrag=cw::fragf("");

	    or_fragments.push_back(cw::fragf("%s%F",
					     start.TargetPkg().FullName(true).c_str(),
					     verfrag));

	    if(start==end)
	      break;
	    ++start;
	  } while(1);

      if(!or_fragments.empty())
	fragments.push_back(cw::join_fragments(or_fragments, L" | "));
    }

  if(fragments.size()==0)
    return cw::fragf("");
  else
    {
      // correctly calculate the indentation for multi-byte encoded strings
      size_t rest_indent = cw::util::transcode(title).length() + 2;
      return cw::fragf("%s: %F",
		       title.c_str(),
		       indentbox(0, rest_indent,
				 flowbox(cw::join_fragments(fragments, L", "))));
    }
}

static cwidget::fragment *prv_lst_frag(pkgCache::PrvIterator prv,
				       bool reverse,
				       int verbose,
				       const std::string &title)
{
  using cwidget::fragment;
  using cwidget::fragf;

  std::vector<std::string> packagevers;

  for ( ; !prv.end(); ++prv)
    {
      string name = reverse ? prv.OwnerPkg().FullName(true) : prv.ParentPkg().FullName(true);
      const char* version = reverse ? prv.OwnerVer().VerStr() : prv.ProvideVersion();

      if (version)
	{
	  // versioned provides have the '=' symbol, and only that one at the
	  // moment, and it is hardcoded in dpkg and apt -- so joining the club
	  const char* symbol = reverse ? "" : "= ";

	  packagevers.push_back(name + " (" + symbol + version + ")");
	}
      else
	{
	  packagevers.push_back(name);
	}
    }

  std::sort(packagevers.begin(), packagevers.end());

  vector<cw::fragment *> fragments;

  for (const auto& it : packagevers)
    {
      fragments.push_back(cw::fragf("%s", it.c_str()));
    }

  if (fragments.empty())
    return cw::fragf("");
  else
    {
      // correctly calculate the indentation for multi-byte encoded strings
      size_t rest_indent = cw::util::transcode(title).length() + 2;
      return cw::fragf("%s: %F",
		       title.c_str(),
		       indentbox(0, rest_indent,
				 flowbox(cw::join_fragments(fragments, L", "))));
    }
}

static cwidget::fragment *archive_lst_frag(pkgCache::VerFileIterator vf,
					   const std::string &title)
{
  vector<cwidget::fragment *> fragments;

  for( ; !vf.end(); ++vf)
    {
      if(vf.File().Archive() == 0)
	fragments.push_back(cwidget::text_fragment(_("<NULL>")));
      else
	fragments.push_back(cwidget::text_fragment(vf.File().Archive()));
    }

  if(fragments.size()==0)
    return cw::fragf("");
  else
    {
      // correctly calculate the indentation for multi-byte encoded strings
      size_t rest_indent = cw::util::transcode(title).length() + 2;
      return cw::fragf("%s: %F",
		       title.c_str(),
		       indentbox(0, rest_indent,
				 flowbox(cw::join_fragments(fragments, L", "))));
    }
}

static const char *current_state_string(const pkgCache::PkgIterator& pkg, const pkgCache::VerIterator& ver)
{
  if(!ver.end() && ver != pkg.CurrentVer())
    return _("not installed");

  switch(pkg->CurrentState)
    {
    case pkgCache::State::NotInstalled:
      return _("not installed");
    case pkgCache::State::UnPacked:
      return _("unpacked");
    case pkgCache::State::HalfConfigured:
      return _("partially configured");
    case pkgCache::State::HalfInstalled:
      return _("partially installed");
    case pkgCache::State::ConfigFiles:
      return _("not installed (configuration files remain)");
    case pkgCache::State::TriggersAwaited:
      return _("awaiting trigger processing by other package(s)");
    case pkgCache::State::TriggersPending:
      return _("awaiting trigger processing");
    case pkgCache::State::Installed:
      return _("installed");
    default:
      return "Internal error: bad state flag!";
    }
}

/** \brief Return a cw::fragment describing the reason that the package
 *  with the given state is being deleted.
 *
 *  statestr is passed to avoid screwing up translations (otherwise I'd
 *  have to break the format string into two pieces).
 */
static cwidget::fragment *deletion_fragment(const char *statestr,
					    const pkgDepCache::StateCache &state,
					    const aptitudeDepCache::aptitude_state &estate)
{
  bool unused_delete=(estate.remove_reason!=aptitudeDepCache::manual);

  if(!state.Delete())
    return cw::fragf("Internal error: no InstVer for a non-deleted package");
  else if(state.iFlags&pkgDepCache::Purge)
    {
      if(unused_delete)
	return cw::fragf(_("%s; will be purged because nothing depends on it"), statestr);
      else
	return cw::fragf(_("%s; will be purged"), statestr);
    }
  else
    {
      if(unused_delete)
	return cw::fragf(_("%s; will be removed because nothing depends on it"), statestr);
      else
	return cw::fragf(_("%s; will be removed"), statestr);
    }
}

static cwidget::fragment *version_change_fragment(const char *statestr,
						  const char *holdstr,
						  pkgCache::VerIterator curver,
						  pkgCache::VerIterator instver)
{
  const char *curverstr=curver.VerStr();
  const char *instverstr=instver.VerStr();

  int vercmp=_system->VS->DoCmpVersion(curverstr,
				       curverstr+strlen(curverstr),
				       instverstr,
				       instverstr+strlen(instverstr));

  if(vercmp>0)
    return cw::fragf(_("%s%s; will be downgraded [%s -> %s]"),
		 statestr, holdstr, curverstr, instverstr);
  else if(vercmp<0)
    return cw::fragf(_("%s%s; will be upgraded [%s -> %s]"),
		 statestr, holdstr, curverstr, instverstr);
  else
    return cw::fragf("%s%s", statestr, holdstr);
}

static cwidget::fragment *state_fragment(pkgCache::PkgIterator pkg, pkgCache::VerIterator ver)
{
  if(pkg.end() || pkg.VersionList().end())
    return cw::fragf(_("not a real package"));

  pkgCache::VerIterator curver=pkg.CurrentVer();
  pkgCache::VerIterator instver=(*apt_cache_file)[pkg].InstVerIter(*apt_cache_file);

  pkgDepCache::StateCache &state=(*apt_cache_file)[pkg];
  aptitudeDepCache::aptitude_state &estate=(*apt_cache_file)->get_ext_state(pkg);

  const char *statestr=current_state_string(pkg, ver);

  const char *holdstr="";

  if(ver == curver && estate.selection_state==pkgCache::State::Hold)
    holdstr=_(" [held]");

  if(!ver.end())
    {
      if(instver != ver && curver != ver)
	return cw::fragf("%s%s", statestr, holdstr);
      else if(instver == ver)
	{
	  if(curver != instver)
	    {
	      if(pkg->CurrentState == pkgCache::State::NotInstalled ||
		 pkg->CurrentState == pkgCache::State::ConfigFiles)
		{
		  if(state.Flags & pkgCache::Flag::Auto)
		    return cw::fragf(_("%s%s; will be installed"), statestr, holdstr);
		  else
		    return cw::fragf(_("%s%s; will be installed automatically"), statestr, holdstr);
		}
	      else
		return version_change_fragment(statestr, holdstr, curver, instver);
	    }
	  else
	    return cw::fragf("%s%s", statestr, holdstr);
	}
      else if(instver.end()) // Now curver is ver.
	return deletion_fragment(statestr, state, estate);
      else
	return version_change_fragment(statestr, holdstr, curver, instver);
    }
  else if(curver.end())
    {
      if(state.Install() &&
	 (pkg->CurrentState==pkgCache::State::NotInstalled ||
	  pkg->CurrentState==pkgCache::State::ConfigFiles))
	{
	  if(state.Flags & pkgCache::Flag::Auto)
	    return cw::fragf(_("%s; version %s will be installed"),
			 statestr,
			 instver.VerStr());
	  else
	    return cw::fragf(_("%s; version %s will be installed automatically"),
			 statestr,
			 instver.VerStr());
	}
      else if(state.Delete() && (state.iFlags&pkgDepCache::Purge) &&
	      pkg->CurrentState!=pkgCache::State::NotInstalled)
	return cw::fragf(_("%s; will be purged"), statestr);
      else
	return cw::fragf("%s", statestr);
    }
  else if(instver.end())
    {
      return deletion_fragment(statestr, state, estate);
    }
  else
    {
      return version_change_fragment(statestr, holdstr, curver, instver);
    }
}

/** \brief Shows information about a package. */
static void show_package(pkgCache::PkgIterator pkg, int verbose,
                         const std::shared_ptr<terminal_metrics> &term_metrics)
{
  vector<cw::fragment *> fragments;

  fragments.push_back(cw::fragf("%s%s%n", _("Package: "), pkg.FullName(true).c_str()));
  fragments.push_back(cw::fragf("%s: %F%n", _("State"), state_fragment(pkg, pkgCache::VerIterator())));
  fragments.push_back(prv_lst_frag(pkg.ProvidesList(), true, verbose, _("Provided by")));

  cw::fragment *f=cw::sequence_fragment(fragments);

  const unsigned int screen_width = term_metrics->get_screen_width();
  cout << f->layout(screen_width, screen_width, cwidget::style());

  delete f;
}

cw::fragment *version_file_fragment(const pkgCache::VerIterator &ver,
				    const pkgCache::VerFileIterator &vf,
				    int verbose)
{
  vector<cw::fragment *> fragments;

  pkgCache::PkgIterator pkg=ver.ParentPkg();
  pkgRecords::Parser &rec=apt_package_records->Lookup(vf);
  aptitudeDepCache::aptitude_state &estate=(*apt_cache_file)->get_ext_state(pkg);
  pkgDepCache::StateCache &state = (*apt_cache_file)[pkg];

  fragments.push_back(cw::fragf("%s%s%n", _("Package: "), pkg.FullName(true).c_str()));
  if((pkg->Flags & pkgCache::Flag::Essential)==pkgCache::Flag::Essential)
    fragments.push_back(cw::fragf("%s%s%n", _("Essential: "),  _("yes")));

  if(estate.new_package)
    fragments.push_back(cw::fragf("%s: %s%n",
			      _("New"), _("yes")));

  fragments.push_back(cw::fragf("%s: %F%n", _("State"), state_fragment(pkg, ver)));
  if(!estate.forbidver.empty())
    fragments.push_back(cw::fragf("%s: %s%n",
			      _("Forbidden version"),
			      estate.forbidver.c_str()));

  if(!pkg.CurrentVer().end())
    fragments.push_back(cw::fragf("%s: %s%n", _("Automatically installed"),
			      (state.Flags & pkgCache::Flag::Auto)
			      ? _("yes") : _("no")));

  const string multiarch(multiarch_type(ver->MultiArch));
  if(!multiarch.empty())
    fragments.push_back(cw::fragf("%s%s%n", _("Multi-Arch: "), multiarch.c_str()));

  fragments.push_back(cw::fragf("%s%s%n", _("Version: "), ver.VerStr()));
  fragments.push_back(cw::fragf("%s%s%n", _("Priority: "),
				const_cast<pkgCache::VerIterator &>(ver).PriorityType() ? const_cast<pkgCache::VerIterator &>(ver).PriorityType() : _("N/A")));
  fragments.push_back(cw::fragf("%s%s%n", _("Section: "),
			    ver.Section()?ver.Section():_("N/A")));
  fragments.push_back(cw::fragf("%s%s%n", _("Maintainer: "),
			    rec.Maintainer().c_str()));
  fragments.push_back(cw::fragf("%s%s%n", _("Architecture: "),
				const_cast<pkgCache::VerIterator &>(ver).Arch()));

  fragments.push_back(cw::fragf("%s%s%n", _("Uncompressed Size: "),
			    SizeToStr(ver->InstalledSize).c_str()));
  if(verbose>0)
    {
      fragments.push_back(cw::fragf("%s%s%n", _("Compressed Size: "),
				SizeToStr(ver->Size).c_str()));
      fragments.push_back(cw::fragf("%s%s%n", _("Filename: "),
				rec.FileName().c_str()));
      fragments.push_back(cw::fragf("%s%s%n", _("MD5sum: "),
				rec.MD5Hash().c_str()));

      if(verbose<2) // Show all archives in a list.
	fragments.push_back(archive_lst_frag(ver.FileList(), _("Archive")));
      else
	{
	  fragments.push_back(cw::fragf("%s: %s%n", _("Archive"), vf.File().Archive()?vf.File().Archive():_("<NULL>")));
	}
    }

  fragments.push_back(dep_lst_frag(ver.DependsList(),
				   _("Depends"), pkgCache::Dep::Depends));
  fragments.push_back(dep_lst_frag(ver.DependsList(),
				   _("PreDepends"), pkgCache::Dep::PreDepends));
  fragments.push_back(dep_lst_frag(ver.DependsList(),
				   _("Recommends"), pkgCache::Dep::Recommends));
  fragments.push_back(dep_lst_frag(ver.DependsList(),
				   _("Suggests"), pkgCache::Dep::Suggests));
  fragments.push_back(dep_lst_frag(ver.DependsList(),
				   _("Conflicts"), pkgCache::Dep::Conflicts));
  fragments.push_back(dep_lst_frag(ver.DependsList(),
				   _("Breaks"), pkgCache::Dep::DpkgBreaks));
  fragments.push_back(dep_lst_frag(ver.DependsList(),
				   _("Replaces"), pkgCache::Dep::Replaces));
  fragments.push_back(dep_lst_frag(ver.DependsList(),
				   _("Obsoletes"), pkgCache::Dep::Obsoletes));
  fragments.push_back(dep_lst_frag(ver.DependsList(),
				   _("Enhances"), pkgCache::Dep::Enhances));

  fragments.push_back(prv_lst_frag(ver.ProvidesList(), false, verbose, _("Provides")));
  fragments.push_back(prv_lst_frag(ver.ParentPkg().ProvidesList(), true, verbose, _("Provided by")));

  fragments.push_back(cw::fragf("%s%ls%n",
			    _("Description: "),
			    get_short_description(ver, apt_package_records).c_str()));
  fragments.push_back(indentbox(1, 1, make_desc_fragment(get_long_description(ver, apt_package_records))));

  if(rec.Homepage() != "")
    fragments.push_back(cw::dropbox(cwidget::text_fragment(_("Homepage: ")),
				cw::hardwrapbox(cwidget::text_fragment(rec.Homepage()))));

  cw::fragment *tags = make_tags_fragment(pkg);
  if(tags)
    fragments.push_back(cw::fragf("%F", tags));

  return cw::sequence_fragment(fragments);
}

static void show_version(pkgCache::VerIterator ver, int verbose,
                         const std::shared_ptr<terminal_metrics> &term_metrics)
{
  if(ver.FileList().end())
    {
      cw::fragment *f=version_file_fragment(ver, ver.FileList(), verbose);

      const unsigned int screen_width = term_metrics->get_screen_width();
      cout << f->layout(screen_width, screen_width, cwidget::style());

      delete f;
    }
  else
    {
      for(pkgCache::VerFileIterator vf=ver.FileList(); !vf.end(); ++vf)
	{
	  cw::fragment *f=version_file_fragment(ver, vf, verbose);

          const unsigned int screen_width = term_metrics->get_screen_width();
	  cout << f->layout(screen_width, screen_width, cwidget::style()) << endl;

	  delete f;

	  // If verbose<2, only show the first file.
	  if(verbose<2)
	    break;
	}
    }
}

static
bool do_cmdline_show_target(const pkgCache::PkgIterator &pkg,
			    cmdline_version_source source,
			    const string &sourcestr,
			    int verbose,
			    bool has_explicit_source,
                            const std::shared_ptr<terminal_metrics> &term_metrics)
{
  if(verbose == 0 || has_explicit_source)
    {
      // HACK: default to current-or-candidate behavior.  This should be
      // done in a more up-front way :-(.
      if(source == cmdline_version_cand)
	source = cmdline_version_curr_or_cand;
      pkgCache::VerIterator ver = cmdline_find_ver(pkg, source, sourcestr);

      if(ver.end())
	ver = pkg.VersionList();

      if(!ver.end())
	show_version(ver, verbose, term_metrics);
      else
	show_package(pkg, verbose, term_metrics);
    }
  else if(!pkg.VersionList().end())
    for(pkgCache::VerIterator ver=pkg.VersionList(); !ver.end(); ++ver)
      show_version(ver, verbose, term_metrics);
  else
    show_package(pkg, verbose, term_metrics);

  return true;
}

bool do_cmdline_show(const string& s, int verbose, const std::shared_ptr<terminal_metrics> &term_metrics)
{
  cmdline_version_source source;
  string name, sourcestr;
  bool has_explicit_source = false;

  if(!cmdline_parse_source(s, source, name, sourcestr))
    return false;

  has_explicit_source = (source != cmdline_version_cand);

  bool is_pattern = aptitude::matching::is_pattern(name);
  pkgCache::PkgIterator pkg;

  if(!is_pattern)
    {
      pkg=(*apt_cache_file)->FindPkg(name);

      if(pkg.end())
	{
	  _error->Error(_("Unable to locate package %s"), s.c_str());
	  return false;
	}
    }

  if(!is_pattern && !pkg.end())
    return do_cmdline_show_target(pkg,
                                  source,
                                  sourcestr,
                                  verbose,
                                  has_explicit_source,
                                  term_metrics);
  else if(is_pattern)
    {
      using namespace aptitude::matching;
      using cwidget::util::ref_ptr;

      ref_ptr<pattern> p(parse(name));

      if(!p.valid())
	{
	  _error->Error(_("Unable to parse pattern %s"), name.c_str());
	  return false;
	}

      std::vector<std::pair<pkgCache::PkgIterator, ref_ptr<structural_match> > > matches;
      ref_ptr<search_cache> search_info(search_cache::create());
      search(p, search_info,
	     matches,
	     *apt_cache_file,
	     *apt_package_records);

      for(std::vector<std::pair<pkgCache::PkgIterator, ref_ptr<structural_match> > >::const_iterator
	    it = matches.begin(); it != matches.end(); ++it)
	{
	  if(!do_cmdline_show_target(it->first,
                                     source,
                                     sourcestr,
                                     verbose,
                                     has_explicit_source,
                                     term_metrics))
	    return false;
	}
    }
  else
    {
      // TODO: print an error message -- Christian will kill me if I
      // make the pofile bigger right now.
    }

  return true;
}

int cmdline_show(int argc, char *argv[], int verbose)
{
  std::shared_ptr<terminal_io> term = create_terminal();

  _error->DumpErrors();

  std::shared_ptr<OpProgress> progress = make_text_progress(true, term, term, term);
  apt_init(progress.get(), false);

  if(_error->PendingError())
    {
      _error->DumpErrors();
      return -1;
    }

  for(int i=1; i<argc; ++i)
    if(!do_cmdline_show(argv[i], verbose, term))
      {
	_error->DumpErrors();
	return -1;
      }

  if(_error->PendingError())
    {
      _error->DumpErrors();
      return -1;
    }

  return 0;
}
