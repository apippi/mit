// cmdline_util.cc
//
// Copyright (C) 2004, 2010 Daniel Burrows
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
#include "cmdline_util.h"

#include "cmdline_common.h"
#include "cmdline_progress.h"
#include "cmdline_show.h" // For operator<<
#include "text_progress.h"
#include "terminal.h"

#include <aptitude.h>
#include <ui.h>
#include <progress.h>

#include <generic/apt/apt.h>
#include <generic/apt/config_signal.h>
#include <generic/apt/download_manager.h>
#include <generic/apt/download_signal_log.h>
#include <generic/apt/matching/match.h>
#include <generic/apt/matching/parse.h>
#include <generic/apt/matching/pattern.h>
#include <generic/apt/tasks.h>

// System includes:
#include <apt-pkg/error.h>
#include <apt-pkg/indexfile.h>
#include <apt-pkg/metaindex.h>
#include <apt-pkg/pkgsystem.h>
#include <apt-pkg/sourcelist.h>
#include <apt-pkg/version.h>

#include <cwidget/fragment.h>
#include <cwidget/toplevel.h>
#include <cwidget/generic/util/ssprintf.h>

#include <sigc++/bind.h>
#include <sigc++/functors/ptr_fun.h>

#include <algorithm>

namespace cw = cwidget;

using aptitude::cmdline::create_cmdline_download_progress;
using aptitude::cmdline::make_text_progress;
using aptitude::cmdline::terminal_input;
using aptitude::cmdline::terminal_locale;
using aptitude::cmdline::terminal_metrics;
using aptitude::cmdline::terminal_output;
using aptitude::controllers::acquire_download_progress;

namespace
{
  const int no_install_run_from_ui_preview_return = -1;
  const int ui_preview_install_failed_return = -1;

  // Completion routine for the UI preview; causes the program to
  // return 0 if the install succeded and 1 otherwise.
  //
  // There's a question in my mind of what to do if the user cancels
  // the preview or runs multiple previews.  At the moment we return
  // -1 if the preview is cancelled and the result of the last install
  // run otherwise.
  void ui_preview_complete(bool success, int *result)
  {
    *result = success ? 0 : ui_preview_install_failed_return;
  }
}

void ui_preview()
{
  int result = no_install_run_from_ui_preview_return;

  ui_init();
  file_quit.connect(sigc::ptr_fun(cwidget::toplevel::exitmain));
  install_finished.connect(sigc::bind(sigc::ptr_fun(ui_preview_complete),
				      &result));
  do_package_run_or_show_preview();
  ui_main();
  exit(result);
}

void ui_solution_screen()
{
  ui_init();
  file_quit.connect(sigc::ptr_fun(cwidget::toplevel::exitmain));

  progress_ref p = gen_progress_bar();
  do_new_package_view(*p->get_progress().unsafe_get_ref());

  do_examine_solution();
  ui_main();
  exit(0);
}

void cmdline_show_stringlist(strvector &items,
                             const std::shared_ptr<terminal_metrics> &term_metrics)
{
  const unsigned int screen_width = term_metrics->get_screen_width();

  int loc=2;

  printf("  ");

  for(strvector::iterator i=items.begin(); i!=items.end(); ++i)
    {
      if(loc + i->size() > (unsigned)(screen_width - 5))
	{
	  printf("\n  ");
	  loc=2;
	}

      printf("%s ", i->c_str());
      loc+=i->size()+1;
    }

  printf("\n");
}

void cmdline_show_pkglist(pkgvector &items,
                          const std::shared_ptr<terminal_metrics> &term_metrics)
{
  strvector tmp;

  for(pkgvector::iterator i=items.begin(); i!=items.end(); ++i)
    tmp.push_back(i->FullName(true));

  cmdline_show_stringlist(tmp, term_metrics);
}

pkgCache::VerIterator cmdline_find_ver(pkgCache::PkgIterator pkg,
				       cmdline_version_source source,
				       string sourcestr)
{
  switch(source)
    {
    case cmdline_version_curr_or_cand:
      if(!pkg.CurrentVer().end())
	return pkg.CurrentVer();
      // Fall-through.
    case cmdline_version_cand:
      {
	pkgCache::VerIterator candver=(*apt_cache_file)[pkg].CandidateVerIter(*apt_cache_file);

	if(candver.end())
	  {
	    if(source == cmdline_version_cand)
	      printf(_("No candidate version found for %s\n"), pkg.FullName(true).c_str());
	    else
	      printf(_("No current or candidate version found for %s\n"), pkg.FullName(true).c_str());
	  }

	return candver;
      }
    case cmdline_version_archive:
      for(pkgCache::VerIterator ver=pkg.VersionList(); !ver.end(); ++ver)
	for(pkgCache::VerFileIterator verfile=ver.FileList();
	    !verfile.end(); ++verfile)
	  {
	    pkgCache::PkgFileIterator pkgfile=verfile.File();
	    if(pkgfile.Archive() && sourcestr==pkgfile.Archive())
	      return ver;
	  }

      for(pkgCache::VerIterator ver = pkg.VersionList(); !ver.end(); ++ver)
	for(pkgCache::VerFileIterator verfile = ver.FileList();
	    !verfile.end(); ++verfile)
	  {
	    pkgCache::PkgFileIterator pkgfile = verfile.File();
	    if(pkgfile.Codename() && sourcestr == pkgfile.Codename())
	      return ver;
	  }

      printf(_("Unable to find an archive \"%s\" for the package \"%s\"\n"),
	     sourcestr.c_str(),
	     pkg.FullName(true).c_str());

      return pkgCache::VerIterator(*apt_cache_file, 0);
    case cmdline_version_version:
      for(pkgCache::VerIterator ver=pkg.VersionList(); !ver.end(); ++ver)
	if(sourcestr==ver.VerStr())
	  return ver;

      printf(_("Unable to find a version \"%s\" for the package \"%s\"\n"),
	     sourcestr.c_str(),
	     pkg.FullName(true).c_str());

      return pkgCache::VerIterator(*apt_cache_file, 0);
    default:
      printf(_("Internal error: invalid value %i passed to cmdline_find_ver!\n"),
	     source);
      return pkg.VersionList();
    }
}

bool cmdline_parse_source(const string &input,
			  cmdline_version_source &source,
			  string &package,
			  string &sourcestr)
{
  string scratch=input;

  source=cmdline_version_cand;
  sourcestr="";

  if(scratch.find('/')!=scratch.npos)
    {
      source=cmdline_version_archive;
      // Use the last one.
      string::size_type loc=scratch.rfind('/');

      sourcestr=string(scratch, loc+1);
      scratch.erase(loc);
    }

  if(scratch.find('=')!=scratch.npos)
    {
      if(source==cmdline_version_archive)
	{
	  printf(_("You cannot specify both an archive and a version for a package\n"));
	  return false;
	}

      source=cmdline_version_version;
      string::size_type loc=scratch.rfind('=');

      sourcestr=string(scratch, loc+1);
      scratch.erase(loc);
    }

  package=scratch;

  return true;
}

bool cmdline_parse_task(string pattern,
                        aptitude::apt::task &task,
                        string &arch)
{
  const string::size_type archfound = pattern.find_last_of(':');
  arch = "native";
  if(archfound != string::npos)
    {
      arch = pattern.substr(archfound+1);
      pattern.erase(archfound);
    }

  if(pattern[pattern.length() - 1] != '^')
    return false;
  pattern.erase(pattern.length() - 1);

  const aptitude::apt::task *t = aptitude::apt::find_task(pattern);
  if(t == NULL)
    return _error->Error(_("Couldn't find task '%s'"), pattern.c_str());

  task = *t;
  return true;
}

namespace
{
  /** \brief Used to display some statistics about how the cache
   *  changed after an update.
   *
   *  Perhaps this should be extended to other commands too?
   */
  class stats
  {
    int num_broken;
    int num_upgradable;
    int num_new;
    // A set of package names that are obsolete.  Names are stored
    // since iterators aren't preserved over cache refreshes.
    std::set<std::string> obsolete;

  public:
    stats(int _num_broken, int _num_upgradable, int _num_new,
	  const std::set<std::string> &_obsolete)
      : num_broken(_num_broken),
	num_upgradable(_num_upgradable),
	num_new(_num_new),
	obsolete(_obsolete)
    {
    }

    int get_num_broken() const { return num_broken; }
    int get_num_upgradable() const { return num_upgradable; }
    int get_num_new() const { return num_new; }
    const std::set<std::string> &get_obsolete() const { return obsolete; }
  };

  stats compute_apt_stats()
  {
    int num_upgradable = 0;
    int num_new = 0;
    int num_broken = 0;
    std::set<std::string> obsolete;

    if(apt_cache_file == NULL)
      return stats(0, 0, 0, obsolete);

    for(pkgCache::PkgIterator p = (*apt_cache_file)->PkgBegin();
	!p.end(); ++p)
      {
	if(pkg_obsolete(p))
	  obsolete.insert(p.FullName(true));

	const pkgDepCache::StateCache &state = (*apt_cache_file)[p];
	const aptitudeDepCache::aptitude_state &estate =
	  (*apt_cache_file)->get_ext_state(p);

	// Q: this will count to-be-removed packages, should it?
	if(state.Status != 2 && state.Upgradable())
	  ++num_upgradable;

	if(state.NowBroken())
	  ++num_broken;

	if((state.Status != 2 || state.CandidateVer != NULL) &&
	   estate.new_package)
	  ++num_new;
      }

    return stats(num_broken, num_upgradable, num_new, obsolete);
  }

  void show_stats_change(stats initial, stats final,
			 bool show_all,
			 bool show_unchanged,
                         const std::shared_ptr<terminal_metrics> &term_metrics)
  {
    using cw::fragf;
    using cw::util::ssprintf;

    std::vector<cw::fragment *> fragments;

    if(show_all ||
       (show_unchanged && final.get_num_broken() != 0) ||
       final.get_num_broken() != initial.get_num_broken())
      {
	std::string change =
	  ssprintf(ngettext("%d (%+d) broken",
			    "%d (%+d) broken",
			    final.get_num_broken()),
		   final.get_num_broken(),
		   final.get_num_broken() - initial.get_num_broken());

	fragments.push_back(cw::text_fragment(change));
      }

    if(show_all ||
       (show_unchanged && final.get_num_upgradable() != 0) ||
       final.get_num_upgradable() != initial.get_num_upgradable())
      {
	std::string change =
	  ssprintf(ngettext("%d (%+d) upgradable",
			    "%d (%+d) upgradable",
			    final.get_num_upgradable()),
		   final.get_num_upgradable(),
		   final.get_num_upgradable() - initial.get_num_upgradable());

	fragments.push_back(cw::text_fragment(change));
      }

    if(show_all ||
       (show_unchanged && final.get_num_new() != 0) ||
       final.get_num_new() != initial.get_num_new())
      {
	std::string change =
	  ssprintf(ngettext("%d (%+d) new",
			    "%d (%+d) new",
			    final.get_num_new()),
		   final.get_num_new(),
		   final.get_num_new() - initial.get_num_new());

	fragments.push_back(cw::text_fragment(change));
      }

    std::vector<cw::fragment *> output_fragments;
    if(fragments.size() > 0)
      output_fragments.push_back(fragf(_("Current status: %F."),
				       cw::join_fragments(fragments, L", ")));

    std::cout << std::endl;
    std::vector<std::string> new_obsolete;
    std::set_difference(final.get_obsolete().begin(), final.get_obsolete().end(),
			initial.get_obsolete().begin(), initial.get_obsolete().end(),
			std::back_insert_iterator<std::vector<std::string> >(new_obsolete));
    const unsigned int obsolete_list_limit = aptcfg->FindI(PACKAGE "::Max-Obsolete-List-Length", 50);
    if(new_obsolete.size() > obsolete_list_limit)
      {
	output_fragments.push_back(cw::text_fragment(ssprintf(ngettext("There is %d newly obsolete package.",
								       "There are %d newly obsolete packages.",
								       new_obsolete.size()),
							      (int)new_obsolete.size())));
      }
    else if(new_obsolete.size() > 0)
      {
	std::vector<cw::fragment *> package_name_fragments;
	for(std::vector<std::string>::const_iterator it =
	      new_obsolete.begin(); it != new_obsolete.end(); ++it)
	  package_name_fragments.push_back(cw::text_fragment(*it));

	output_fragments.push_back(cw::dropbox(cw::text_fragment(ssprintf(ngettext("There is %d newly obsolete package: ",
										   "There are %d newly obsolete packages: ",
										   new_obsolete.size()),
									  (int)new_obsolete.size())),
					       wrapbox(cw::join_fragments(package_name_fragments, L", "))));
      }

    if(output_fragments.size() > 0)
      {
	cw::fragment *f = join_fragments(output_fragments, L"\n");

        const unsigned int screen_width = term_metrics->get_screen_width();
	std::cout << f->layout(screen_width, screen_width, cw::style());
	delete f;
      }
  }
}

namespace
{
  template<typename T>
  void assign(const T &val, T *target)
  {
    *target = val;
  }
}

download_manager::result cmdline_do_download(download_manager *m,
					     int verbose,
                                             const std::shared_ptr<terminal_input> &term_input,
                                             const std::shared_ptr<terminal_locale> &term_locale,
                                             const std::shared_ptr<terminal_metrics> &term_metrics,
                                             const std::shared_ptr<terminal_output> &term_output)
{
  stats initial_stats(0, 0, 0, std::set<std::string>());
  std::shared_ptr<OpProgress> progress = make_text_progress(false, term_locale, term_metrics, term_output);

  if(aptcfg->FindI("Quiet", 0) == 0)
    {
      // This does exactly what we want: nothing if the cache is
      // already loaded, and loads the cache with a minimum of frills
      // otherwise.
      OpProgress tmpProgress;
      // Dump errors so we don't spuriously think we failed.
      _error->DumpErrors();
      apt_load_cache(&tmpProgress, false, NULL);
      initial_stats = compute_apt_stats();
    }

  std::pair<download_signal_log *, std::shared_ptr<acquire_download_progress> >
    progress_display = create_cmdline_download_progress(term_input,
                                                        term_locale,
                                                        term_metrics,
                                                        term_output);

  std::unique_ptr<download_signal_log> log(progress_display.first);

  // Dump errors here because prepare() might check for pending errors
  // and think something failed.
  _error->DumpErrors();

  if(!m->prepare(*progress, *log.get(), log.get()))
    return download_manager::failure;

  download_manager::result finish_res;

  do
    {
      pkgAcquire::RunResult download_res = m->do_download();
      m->finish(download_res, progress.get(),
		sigc::bind(sigc::ptr_fun(&assign<download_manager::result>),
			   &finish_res));
    }
  while(finish_res == download_manager::do_again);

  stats final_stats(0, 0, 0, std::set<std::string>());
  if(aptcfg->FindI("Quiet", 0) == 0)
    {
      OpProgress tmpProgress;
      apt_load_cache(&tmpProgress, false, NULL);
      final_stats = compute_apt_stats();
      show_stats_change(initial_stats, final_stats,
			verbose >= 1, verbose >= 2,
                        term_metrics);
    }

  return finish_res;
}

namespace aptitude
{
  namespace cmdline
  {
    namespace
    {
      source_package find_source_package(const std::string &source_name,
					 const std::string &source_version)
      {
	if(apt_source_list == NULL)
	  return NULL;

	pkgSrcRecords records(*apt_source_list);
	records.Restart();

	pkgSrcRecords::Parser *parser = records.Find(source_name.c_str());
	while(parser != NULL && parser->Version() != source_version)
	  parser = records.Find(source_name.c_str());

	return parser;
      }

      // Find the most recent source package for the given name.
      source_package find_source_package(const std::string &source_name)
      {
	if(apt_source_list == NULL)
	  return NULL;

	pkgSrcRecords records(*apt_source_list);
	records.Restart();

	pkgSrcRecords::Parser *parser = records.Find(source_name.c_str());
	source_package rval = parser;

	while(parser != NULL)
	  {
	    if(_system->VS->CmpVersion(rval.get_version(), parser->Version()) < 0)
	      rval = parser;
	    parser = records.Find(source_name.c_str());
	  }

	return rval;
      }
    }

    source_package::source_package()
    {
    }

    source_package::source_package(pkgSrcRecords::Parser *parser)
    {
      if(parser != NULL)
	{
	  package = parser->Package();
	  version = parser->Version();
	  maintainer = parser->Maintainer();
	  section = parser->Section();

	  if(parser->Binaries() != NULL)
	    {
	      for(const char **b = parser->Binaries(); *b != NULL; ++b)
		binaries.push_back(*b);
	    }

	  parser->BuildDepends(build_deps, false);
	}
    }

    source_package find_source_by_archive(const std::string &source_name,
					  const std::string &archive)
    {
      if(apt_source_list == NULL)
	return NULL;

      for(pkgSourceList::const_iterator i = apt_source_list->begin();
	  i != apt_source_list->end(); ++i)
	{
	  if((*i)->GetDist() != archive)
	    continue;

	  vector<pkgIndexFile *> *indexes = (*i)->GetIndexFiles();

	  for(vector<pkgIndexFile *>::const_iterator j = indexes->begin();
	      j != indexes->end(); ++j)
	    {
	      std::unique_ptr<pkgSrcRecords::Parser> p((*j)->CreateSrcParser());

	      if(_error->PendingError())
		return source_package();
	      if(p.get() != 0)
		{
		  // Step through the file until we reach the end or find
		  // the package:
		  while(p.get()->Step() == true)
		    {
		      if(_error->PendingError() == true)
			return source_package();

		      if(p.get()->Package() == source_name)
			return source_package(p.get());
		    }
		}
	    }
	}

      return source_package();
    }

    source_package find_source_package(const std::string &package,
				       cmdline_version_source version_source,
				       const std::string &version_source_string_orig)
    {
      // This will be set below to the package archive if necessary.
      std::string version_source_string(version_source_string_orig);
      if(apt_cache_file == NULL || apt_package_records == NULL || apt_source_list == NULL)
	return NULL;

      string default_release = aptcfg->Find("APT::Default-Release");
      if(version_source == cmdline_version_cand && !default_release.empty())
	{
	  version_source        = cmdline_version_archive;
	  version_source_string = default_release;
	}

      pkgCache::PkgIterator pkg = (*apt_cache_file)->FindPkg(package);
      std::string source_package_name = package;

      source_package rval;

      // First look for a source of the given name.
      switch(version_source)
	{
	case cmdline_version_cand:
	  rval = find_source_package(source_package_name);
	  break;

	case cmdline_version_curr_or_cand:
	  break;
	  // Do this later since we need to refer to the installed
	  // version.

	case cmdline_version_archive:
	  _error->DumpErrors();
	  rval = find_source_by_archive(source_package_name, version_source_string);
	  break;

	case cmdline_version_version:
	  rval = find_source_package(source_package_name, version_source_string);
	  break;
	}

      if(rval.valid())
	return rval;

      // If no source of that name exists, try to find a real package
      // and use its source.
      if(!pkg.end())
	{
	  pkgCache::VerIterator ver = cmdline_find_ver(pkg,
						       version_source,
						       version_source_string);

	  if(!ver.end())
	    {
#if APT_PKG_MAJOR >= 5
	      // with apt-1.1:
	      //
	      // - SourcePkg (and Version) are in the binary cache and available via
	      //   the VerIterator; much faster than parsing the pkgRecord
	      //
	      // - defaults to package name, no need to check if it's empty
	      source_package_name = ver.SourcePkgName();
	      source_version = ver.SourceVerStr();

	      rval = find_source_package(source_package_name, source_version);
#else
	      pkgRecords::Parser &rec =
		apt_package_records->Lookup(ver.FileList());

	      if(!rec.SourcePkg().empty())
		source_package_name = rec.SourcePkg();

	      if(version_source == cmdline_version_version
		 || version_source == cmdline_version_curr_or_cand)
		{
		  const std::string source_version =
		    rec.SourceVer().empty() ? ver.VerStr() : rec.SourceVer();

		  rval = find_source_package(source_package_name, source_version);
		}
#endif
	    }
	  // Last-ditch effort: if no matching version was found but
	  // a source package can be found, use that and try again below.
	  else if(!pkg.VersionList().end())
	    {
	      pkgRecords::Parser &rec =
		apt_package_records->Lookup(pkg.VersionList().FileList());

	      if(!rec.SourcePkg().empty())
		source_package_name = rec.SourcePkg();
	    }
	}

      if(!rval.valid())
	{
	  switch(version_source)
	    {
	    case cmdline_version_cand:
	      rval = find_source_package(source_package_name);
	      break;

	    case cmdline_version_curr_or_cand:
	      break; // We would have tried already if it was possible.

	    case cmdline_version_archive:
	      _error->DumpErrors();
	      rval = find_source_by_archive(source_package_name, version_source_string);
	      break;

	    case cmdline_version_version:
	      rval = find_source_package(source_package_name, version_source_string);
	      break;
	    }
	}

      return rval;
    }

    void apply_user_tags(const std::vector<tag_application> &user_tags)
    {
      using namespace matching;
      cwidget::util::ref_ptr<search_cache> search_info(search_cache::create());
      for(pkgCache::PkgIterator pkg = (*apt_cache_file)->PkgBegin();
	  !pkg.end(); ++pkg)
	{
	  for(std::vector<tag_application>::const_iterator it = 
		user_tags.begin(); it != user_tags.end(); ++it)
	    {
	      bool applicable = false;
	      if(it->get_pattern().valid())
		{
		  if(matching::get_match(it->get_pattern(),
					 pkg,
					 search_info,
					 *apt_cache_file,
					 *apt_package_records).valid())
		    applicable = true;
		}
	      else
		{
		  const pkgDepCache::StateCache &state = (*apt_cache_file)[pkg];
		  // Perhaps we should somehow filter out automatic
		  // changes here, but that's hard and might be
		  // unpredictable (which would make the user sad).
		  // Instead we just add the tag to all packages that
		  // are being modified.
		  if(!state.Keep())
		    applicable = true;
		}

	      if(applicable)
		{
		  if(it->get_is_add())
		    (*apt_cache_file)->attach_user_tag(pkg, it->get_tag(), NULL);
		  else
		    (*apt_cache_file)->detach_user_tag(pkg, it->get_tag(), NULL);
		}
	    }
	}
    }

    std::wstring de_columnize(const cwidget::config::column_definition_list &columns,
			      cwidget::config::column_generator &columnizer,
			      cwidget::config::column_parameters &p)
    {
      using namespace cwidget::config;

      // TODO: this should move into cwidget in the future, as a new
      // mode of operation for layout_columns().
      std::wstring output;

      for(column_definition_list::const_iterator it = columns.begin();
	  it != columns.end();
	  ++it)
	{
	  if(it->type == column_definition::COLUMN_LITERAL)
	    output += it->arg;
	  else
	    {
	      eassert(it->type == column_definition::COLUMN_GENERATED ||
		      it->type == column_definition::COLUMN_PARAM);

	      if(it->type == column_definition::COLUMN_GENERATED)
		{
		  cwidget::column_disposition disp = columnizer.setup_column(it->ival);
		  output += disp.text;
		}
	      else
		{
		  if(p.param_count() <= it->ival)
		    output += L"###";
		  else
		    output += p.get_param(it->ival);
		}
	    }
	}

      return output;
    }
  }
}
