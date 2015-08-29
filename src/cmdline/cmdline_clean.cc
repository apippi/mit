// cmdline_clean.cc
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
#include "cmdline_clean.h"

#include "text_progress.h"
#include "terminal.h"

#include <aptitude.h>

#include <generic/apt/apt.h>
#include <generic/apt/config_signal.h>


// System includes:
#include <apt-pkg/acquire.h>
#include <apt-pkg/clean.h>
#include <apt-pkg/error.h>
#include <apt-pkg/strutl.h>

#include <iostream>

#include <memory>

#include <cstdio>
#include <cstdint>
#include <sys/stat.h>

using namespace std;

using aptitude::cmdline::create_terminal;
using aptitude::cmdline::make_text_progress;
using aptitude::cmdline::terminal_io;
using aptitude::cmdline::terminal_locale;

int cmdline_clean(int argc, char *argv[], bool simulate)
{
  const string archivedir = aptcfg->FindDir("Dir::Cache::archives");

  _error->DumpErrors();

  if(argc != 1)
    {
      fprintf(stderr, _("E: The clean command takes no arguments\n"));
      return -1;
    }  

  if(simulate)
    {
      printf(_("Del %s* %spartial/*\n"), archivedir.c_str(), archivedir.c_str());
      return 0;
    }

  // Lock the archive directory
  FileFd lock;
  if (_config->FindB("Debug::NoLocking",false) == false)
    {
      lock.Fd(GetLock(archivedir + "lock"));
      if (_error->PendingError() == true)
        _error->Error(_("Unable to lock the download directory"));
    }

  if(_error->PendingError())
    {
      _error->DumpErrors();
      return -1;
    }

  pkgAcquire fetcher;
  fetcher.Clean(archivedir);
  fetcher.Clean(archivedir+"partial/");

  int rval=_error->PendingError() ? -1 : 0;

  _error->DumpErrors();

  return rval;
}

// Shamelessly stolen from apt-get:
class LogCleaner : public pkgArchiveCleaner
{
  bool simulate;

  uint64_t total_size;

protected:
  virtual void Erase(const char *File,string Pkg,string Ver,struct stat &St) 
  {
    printf(_("Del %s %s [%sB]\n"),
	   Pkg.c_str(),
	   Ver.c_str(),
	   SizeToStr(St.st_size).c_str());

    if (!simulate)
      {
	if(unlink(File)==0)
	  total_size+=St.st_size;
      }
    else
      total_size+=St.st_size;
  }

public:
  LogCleaner(bool _simulate) : simulate(_simulate), total_size(0) { }

  uint64_t get_total_size() const { return total_size; }
};

int cmdline_autoclean(int argc, char *argv[], bool simulate)
{
  const string archivedir = aptcfg->FindDir("Dir::Cache::archives");
  const std::shared_ptr<terminal_io> term = create_terminal();

  _error->DumpErrors();

  if(argc != 1)
    {
      fprintf(stderr, _("E: The autoclean command takes no arguments\n"));
      return -1;
    }  

  // Lock the archive directory
  FileFd lock;
  if (!simulate &&
      _config->FindB("Debug::NoLocking",false) == false)
    {
      lock.Fd(GetLock(archivedir + "lock"));
      if (_error->PendingError() == true)
        {
          _error->Error(_("Unable to lock the download directory"));
          _error->DumpErrors();
          return -1;
        }
    }

  std::shared_ptr<OpProgress> progress = make_text_progress(false, term, term, term);

  apt_init(progress.get(), false);

  if(_error->PendingError())
    {
      _error->DumpErrors();
      return -1;
    }

  LogCleaner cleaner(simulate);
  int rval=0;
  if(!(cleaner.Go(archivedir, *apt_cache_file) &&
       cleaner.Go(archivedir+"partial/", *apt_cache_file)) ||
     _error->PendingError())
    rval=-1;

  _error->DumpErrors();

  if(simulate)
    printf(_("Would free %sB of disk space\n"),
	   SizeToStr(cleaner.get_total_size()).c_str());
  else
    printf(_("Freed %sB of disk space\n"),
	   SizeToStr(cleaner.get_total_size()).c_str());

  return rval;
}

