// -*- Mode: c++; tab-width: 8; c-basic-offset: 2; indent-tabs-mode: nil -*-
// NOTE: the first line of this file sets up source code indentation rules
// for Emacs; it is also a hint to anyone modifying this file.

// Created   : Fri Nov 25 15:31:57 MST 2011
// Copyright : Pavel Koshevoy
// License   : MIT -- http://www.opensource.org/licenses/mit-license.php

// aeyae:
#include "yae/api/yae_version.h"
#include "yae/utils/yae_utils.h"

// system:
#ifdef _WIN32
#include <winsock2.h>
#include <windows.h>
#define _OLEAUT32_
#include <unknwn.h>
#endif

// standard:
#include <iostream>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <sstream>
#include <iomanip>
#include <fstream>
#include <string>
#include <vector>
#include <list>
#include <map>
#include <set>

YAE_DISABLE_DEPRECATION_WARNINGS

// boost:
#include <boost/algorithm/string.hpp>
#include <boost/algorithm/string/predicate.hpp>
#include <boost/algorithm/string/replace.hpp>
#include <boost/filesystem.hpp>
#include <boost/process.hpp>
#include <boost/regex.hpp>

YAE_ENABLE_DEPRECATION_WARNINGS

// namespace shortcut:
namespace fs = boost::filesystem;
namespace al = boost::algorithm;
namespace bp = boost::process;
using yae::has;


//----------------------------------------------------------------
// makeGuidStr
//
static std::string
makeGuidStr()
{
  GUID guid;
  CoCreateGuid(&guid);

  wchar_t * wstr = NULL;
  StringFromCLSID(guid, &wstr);

  int sz = WideCharToMultiByte(CP_UTF8, 0,
                               wstr, -1,
                               NULL, 0,
                               NULL, NULL);

  std::vector<char> chars(sz, 0);
  WideCharToMultiByte(CP_UTF8, 0,
                      wstr, -1,
                      &chars[0], sz,
                      NULL, NULL);

  CoTaskMemFree(wstr);
  wstr = NULL;

  std::string str(chars.begin() + 1, chars.end() - 2);
  return str;
}

//----------------------------------------------------------------
// getFileName
//
static std::string
getFileName(const std::string & fullPath)
{
  std::size_t found = fullPath.rfind('\\');
  if (found == std::string::npos)
  {
    return fullPath;
  }

  std::string name = fullPath.substr(found + 1);
  return name;
}

//----------------------------------------------------------------
// detect
//
static bool
detect(const char * pattern,
       const std::string & line,
       std::string & head,
       std::string & tail)
{
  std::size_t found = line.find(pattern);
  if (found == std::string::npos)
  {
    return false;
  }

  std::size_t pattern_sz = strlen(pattern);
  head = line.substr(0, found + pattern_sz);
  tail = line.substr(found + pattern_sz);
  return true;
}

//----------------------------------------------------------------
// tolower
//
static std::string
tolower(const std::string & src)
{
  return yae::to_lower(src);
}

//----------------------------------------------------------------
// append_path
//
static void
append_path(std::string & paths, const std::string & path)
{
  if (!paths.empty())
  {
    paths += ';';
  }

  paths += path;
}

//----------------------------------------------------------------
// DllSearchPath
//
struct DllSearchPath
{
  DllSearchPath(std::string paths = std::string())
  {
    if (paths.empty())
    {
      paths = boost::this_process::environment().get("PATH");
    }

    std::vector<std::string> strs;
    yae::split(strs, ";", paths.c_str());

    for (std::size_t i = 0, n = strs.size(); i < n; ++i)
    {
      const std::string & s = strs[i];
      fs::path p(s);
      paths_.push_back(p);
    }
  }

  void prepend(std::list<std::string> & paths)
  {
    std::list<fs::path> result;

    for (std::list<std::string>::const_iterator
           i = paths.begin(); i != paths.end(); ++i)
    {
      const std::string & s = *i;
      fs::path p(s);
      if (fs::exists(p))
      {
        result.push_back(p);
      }
      else
      {
        std::cerr << "path doesn't exist: " << s << std::endl;
      }
    }

    result.splice(result.end(), paths_);
    paths_.swap(result);
  }

  std::string find(const std::string & name) const
  {
    for (std::list<fs::path>::const_iterator
           i = paths_.begin(); i != paths_.end(); ++i)
    {
      fs::path p = (*i) / name;
      if (fs::exists(p))
      {
        std::string found = p.make_preferred().string();
        return found;
      }
    }

    return std::string();
  }

  std::list<fs::path> paths_;
};

//----------------------------------------------------------------
// DepsResolver
//
struct DepsResolver
{

  //----------------------------------------------------------------
  // add
  //
  // NOTE: this is recursive:
  //
  void
  add(const std::string & name, const std::string & path)
  {
    if (yae::has(resolved_, name))
    {
      return;
    }

    resolved_[name] = path;

    std::list<std::string> dlls = this->get_dll_names(path);
    for (std::list<std::string>::const_iterator
           i = dlls.begin(); i != dlls.end(); ++i)
    {
      const std::string & dll_name = *i;
      if (yae::has(resolved_, dll_name))
      {
        continue;
      }

      std::string dll_path = search_path_.find(dll_name);
      if (dll_path.empty())
      {
        std::cerr << "file not found: " << dll_name << std::endl;
        resolved_[dll_name] = std::string();
      }
      else
      {
        this->add(dll_name, dll_path);
      }
    }
  }

  //----------------------------------------------------------------
  // get_dll_names
  //
  std::list<std::string>
  get_dll_names(const std::string & object)
  {
    static std::string dumpbin_path = search_path_.find("dumpbin.exe");
    YAE_THROW_IF(dumpbin_path.empty());

    std::string dumpbin;
    {
      std::ostringstream oss;
      oss << dumpbin_path << " /dependents " << object;
      dumpbin = oss.str();
    }

    bp::ipstream pipe_stream;
    bp::child c(dumpbin, bp::std_out > pipe_stream);
    std::list<std::string> dlls;
    {
      bool found_start = false;
      bool reached_end = false;

      std::string line;
      while (pipe_stream && std::getline(pipe_stream, line) && !line.empty())
      {
        std::string token = yae::trim_ws(line);
        if (token.empty())
        {
          continue;
        }

        if (!found_start)
        {
          found_start = (token == "Image has the following dependencies:");
        }
        else if (!reached_end)
        {
          reached_end = (token == "Summary");

          if (!reached_end && al::ends_with(token, ".dll"))
          {
            dlls.push_back(token);
          }
        }
      }
    }

    return dlls;
  }

  //----------------------------------------------------------------
  // get_dll_paths
  //
  void
  get_paths(std::vector<std::string> & results,
            const std::list<std::string> & allowed,
            const std::string & ext) const
  {
    for (std::map<std::string, std::string>::const_iterator
           i = resolved_.begin(); i != resolved_.end(); ++i)
    {
      const std::string & name = i->first;
      const std::string & path = i->second;

      if (path.empty())
      {
        // file not found:
        continue;
      }

      if (!al::ends_with(name, ext))
      {
        continue;
      }

      bool not_allowed = true;
      for (std::list<std::string>::const_iterator
             j = allowed.begin(); j != allowed.end(); ++j)
      {
        const std::string & prefix = *j;
        if (al::starts_with(path, prefix))
        {
          not_allowed = false;
          break;
        }
      }

      if (not_allowed)
      {
        continue;
      }

      std::cout << "adding: " << path << std::endl;
      results.push_back(path);
    }
  }

  DllSearchPath search_path_;
  std::map<std::string, std::string> resolved_; // name: path
};

//----------------------------------------------------------------
// InstallerData
//
struct InstallerData
{
  std::string installer_name_; // apprenticevideo
  std::string product_id_; // ApprenticeVideo
  std::string product_name_; // Apprentice Video
  std::string guid_upgrade_; // "a4a297db-1d6c-4320-b015-80add2a8d07c"
  std::string comments_; // A Video Player
  std::vector<std::string> ext_; // { "aac", "avi", "mov", "mp4", "mkv"...
};

//----------------------------------------------------------------
// make_installer
//
static int
make_installer(const InstallerData & data,
               const std::string & dependsExe,
               const std::string & allowedPaths,
               const std::string & vcRedistMsm,
               const std::string & wixCandleExe,
               const std::string & wixLightExe,
               const std::string & iconFile,
               const std::string & helpLink,
               const std::list<std::string> & deploy,
               const std::map<std::string, std::set<std::string> > & deployTo,
               const std::map<std::string, std::string> & deployFrom)
{
  // add paths of the deployed objects to the allowed paths:
  std::list<std::string> allowed;
  for (std::list<std::string>::const_iterator
         i = deploy.begin(); i != deploy.end(); ++i)
  {
    fs::path path = tolower(*i);
    std::string dir = yae::trim_ws(path.parent_path().make_preferred().string());
    allowed.push_back(dir);
  }

  // parse allowed paths:
  {
    std::string paths = allowedPaths;
    std::string head;
    std::string tail;

    while (paths.size())
    {
      if (detect(";", paths, head, tail))
      {
        if (head.size() > 1)
        {
          std::string path = yae::trim_ws(tolower(head.substr(0, head.size() - 1)));
          path = fs::path(path).make_preferred().string();
          allowed.push_back(path);
        }

        paths = tail;
      }
      else
      {
        std::string path = yae::trim_ws(tolower(paths));
        path = fs::path(path).make_preferred().string();
        allowed.push_back(path);
        break;
      }
    }
  }

  // add allowed paths to env PATH, so dumpbin would search there:
  {
    std::string path;
    const char * pathEnv = getenv("PATH");
    if (pathEnv)
    {
      path = pathEnv;
    }

    path = yae::trim_ws(path);

    for (std::list<std::string>::const_iterator
           i = allowed.begin(); i != allowed.end(); ++i)
    {
      const std::string & p = *i;
      append_path(path, p);
    }

    _putenv((std::string("PATH=") + path).c_str());
  }

  // call depends.exe:
  std::vector<std::string> exes;
  std::vector<std::string> deps;
  {
    DepsResolver deps_resolver;
    deps_resolver.search_path_.prepend(allowed);

    for (std::list<std::string>::const_iterator
           i = deploy.begin(); i != deploy.end(); ++i)
    {
      const std::string & object = *i;
      // get_dependencies(deps, dependsExe, allowed, object);
      std::string name = getFileName(object);
      deps_resolver.add(name, object);
    }

    deps_resolver.get_paths(exes, allowed, ".exe");
    deps_resolver.get_paths(deps, allowed, ".dll");
  }

  std::string installerName;
  {
    std::ostringstream os;
    os << data.installer_name_ << "-" << YAE_REVISION;
#ifdef _WIN64
    os << "-win32-x64";
#else
    os << "-win32-x86";
#endif

    installerName.assign(os.str().c_str());
  }

  std::string installerNameWxs = installerName + ".wxs";
  std::fstream out;
  out.open(installerNameWxs.c_str(), std::ios::out);

  out << "<?xml version='1.0' encoding='utf-8'?>" << std::endl
      << "<Wix xmlns='http://schemas.microsoft.com/wix/2006/wi'>" << std::endl
      << std::endl;

  unsigned int major = 0;
  unsigned int minor = 0;
  unsigned int patch = 0;
  yae_version(&major, &minor, &patch);

  out << " <Product Name='" << data.product_name_ << "' "
      << "Id='" << makeGuidStr() << "' "
      << "UpgradeCode='" << data.guid_upgrade_ << "' "
      << "Language='1033' Codepage='1252' "
      << "Version='" << major << '.' << minor << '.' << patch << "' "
      << "Manufacturer='Pavel Koshevoy'>"
      << std::endl;

  out << "  <Package Id='*' Keywords='Installer' "
#ifdef _WIN64
      << "Platform='x64' "
#else
      << "Platform='x86' "
#endif
      << "Description='" << data.product_name_ << " Installer' "
      << "Comments='" << data.comments_ << "' "
      << "Manufacturer='Pavel Koshevoy' "
      << "InstallerVersion='300' "
      << "Languages='1033' Compressed='yes' SummaryCodepage='1252' />\n"
      << std::endl;

  out << "  <MajorUpgrade AllowDowngrades='yes' />\n"
      << std::endl;

  out << "  <DirectoryRef Id=\"TARGETDIR\">\n"
      << "   <Merge Id=\"VCRedist\" SourceFile=\""
      << vcRedistMsm
      << "\" DiskId=\"1\" Language=\"0\"/>\n"
      << "  </DirectoryRef>\n\n"
      << "  <Feature Id=\"VCRedist\" Title=\"Visual C++ Runtime\" "
      << "AllowAdvertise=\"no\" Display=\"hidden\" Level=\"1\">\n"
      << "   <MergeRef Id=\"VCRedist\"/>\n"
      << "  </Feature>\n"
      << std::endl;

  out << "  <Media Id='1' Cabinet='product.cab' EmbedCab='yes' />\n\n"
      << "  <Directory Id='TARGETDIR' Name='SourceDir'>\n";

#ifdef _WIN64
  out << "   <Directory Id='ProgramFiles64Folder' Name='PFiles'>\n";
#else
  out << "   <Directory Id='ProgramFilesFolder' Name='PFiles'>\n";
#endif

  out << "    <Directory Id='" << data.product_id_ << "' Name='"
      << data.product_name_ << "'>"
      << std::endl;


  std::string icon = getFileName(iconFile);
  std::size_t fileIndex = 0;

  for (std::size_t i = 0; i < exes.size(); i++, fileIndex++)
  {
    const std::string & path = exes[i];
    std::string name = getFileName(path);
    std::string guid = makeGuidStr();

    out << "     <Component Id='Component" << fileIndex
        << "' Guid='" << guid << "'"
#ifdef _WIN64
        << " Win64='yes'"
#else
        << " Win64='no'"
#endif
        << ">" << std::endl;

    // executable:
    out << "      <File Id='File" << fileIndex << "' "
        << "Name='" << name << "' DiskId='1' "
        << "Source='" << path << "' "
        << "KeyPath='yes'>"
        << std::endl;

    out << "       <Shortcut Id='startmenu" << data.product_id_ << "' "
        << "Directory='ProgramMenuDir' "
        << "Name='" << data.product_name_ << "' "
        << "WorkingDirectory='INSTALLDIR' "
        << "Icon='" << icon << "' "
        << "IconIndex='0' "
        << "Advertise='yes' />"
        << std::endl;

    out << "       <Shortcut Id='desktop" << data.product_id_ << "' "
        << "Directory='DesktopFolder' "
        << "Name='" << data.product_name_ << "' "
        << "WorkingDirectory='INSTALLDIR' "
        << "Icon='" << icon <<"' "
        << "IconIndex='0' "
        << "Advertise='yes' />"
        << std::endl;

    out << "      </File>"
        << std::endl;

    std::size_t numSupported = data.ext_.size();
    for (std::size_t j = 0; j < numSupported; j++)
    {
      const char * ext = data.ext_[j].c_str();
      out << "      <ProgId Id='" << data.product_id_ << "." << ext << "' "
          << "Icon='" << icon << "' IconIndex='0' Advertise='yes' "
          << "Description='media file format supported by "
          << data.product_name_ << "'>\n"
          << "       <Extension Id='" << ext << "'>\n"
          << "        <Verb Id='open' Command='Open' "
          << "Argument='&quot;%1&quot;' />\n"
          << "       </Extension>\n"
          << "      </ProgId>\n"
          << std::endl;
    }

    out << "     </Component>\n"
        << std::endl;
  }

  // dlls:
  for (std::size_t i = 0; i < deps.size(); i++, fileIndex++)
  {
    const std::string & path = deps[i];
    std::string name = getFileName(path);
    std::string guid = makeGuidStr();

    out << "     <Component Id='Component" << fileIndex
        << "' Guid='" << guid << "'"
#ifdef _WIN64
        << " Win64='yes'"
#else
        << " Win64='no'"
#endif
        << ">" << std::endl;

    out << "      <File Id='File" << fileIndex << "' "
        << "Name='" << name << "' DiskId='1' "
        << "Source='" << path << "' "
        << "KeyPath='yes' />\n";

    out << "     </Component>\n"
        << std::endl;
  }

  typedef std::map<std::string, std::set<std::string> > TDeployTo;
  for (TDeployTo::const_iterator
         i = deployTo.begin(); i != deployTo.end(); ++i)
  {
    const std::string & dst = i->first;
    const std::string & src = yae::at(deployFrom, dst);
    const std::set<std::string> & files = i->second;

    out << "     <Directory Id='" << dst << "' Name='" << dst << "'>"
        << std::endl;

    for (std::set<std::string>::const_iterator
           j = files.begin(); j != files.end(); ++j, fileIndex++)
    {
      const std::string & path = *j;
      std::string name = fs::path(path).filename().string();
      std::string guid = makeGuidStr();

      out << "      <Component Id='Component" << fileIndex
          << "' Guid='" << guid << "'"
#ifdef _WIN64
          << " Win64='yes'"
#else
          << " Win64='no'"
#endif
          << ">\n"
          << "       <File Id='File" << fileIndex << "' "
          << "Name='" << name << "' DiskId='1' "
          << "Source='" << path << "' "
          << "KeyPath='yes' />\n"
          << "      </Component>\n"
          << std::endl;
    }

    out << "     </Directory>"
        << std::endl;
  }

  out << "    </Directory>\n"
      << "   </Directory>\n"
      << std::endl;

  out << "   <Directory Id='ProgramMenuFolder' Name='Programs'>\n"
      << "    <Directory Id='ProgramMenuDir' Name='"
      << data.product_name_ << "'>\n"
      << "     <Component Id='ProgramMenuDir' Guid='"
      << makeGuidStr() << "'>\n"
      << "      <RemoveFolder Id='ProgramMenuDir' On='uninstall' />\n"
      << "      <RegistryValue Root='HKCU' "
      << "Key='Software\\[Manufacturer]\\[ProductName]' "
      << "Type='string' Value='' KeyPath='yes' />\n"
      << "     </Component>\n"
      << "    </Directory>\n"
      << "   </Directory>\n"
      << "   <Directory Id='DesktopFolder' Name='Desktop' />"
      << std::endl;

  out << "  </Directory>\n"
      << std::endl;

  out << "  <Feature Id='Complete' Title='"
      << data.product_name_ << "' Level='1'>\n";
  for (std::size_t i = 0; i < fileIndex; ++i)
  {
    out << "   <ComponentRef Id='Component" << i << "' />\n";
  }

  out << "   <ComponentRef Id='ProgramMenuDir' />\n"
      << "  </Feature>\n\n"
      << "  <Icon Id='" << icon << "' "
      << "SourceFile='" << iconFile << "' />\n"
      << "  <Property Id='ARPPRODUCTICON' Value='" << icon << "' />\n"
      << "  <Property Id='ARPHELPLINK' Value='" << helpLink << "' />\n"
      << std::endl;

  out << " </Product>\n"
      << "</Wix>"
      << std::endl;

  out.close();

  // call candle.exe:
  {
    std::ostringstream os;
    os << '"' << wixCandleExe << "\" " << installerName << ".wxs";

    std::string cmd(os.str().c_str());
    std::cerr << cmd << std::endl;

    int r = system(cmd.c_str());
    std::cerr << wixCandleExe << " returned: " << r << std::endl;

    if (r != 0)
    {
      return r;
    }
  }

  // call light.exe:
  {
    std::ostringstream os;
    os << '"' << wixLightExe << "\" " << installerName << ".wixobj";

    std::string cmd(os.str().c_str());
    std::cerr << cmd << std::endl;

    int r = system(cmd.c_str());
    std::cerr << wixLightExe << " returned: " << r << std::endl;

    if (r != 0)
    {
      return r;
    }
  }

  return 0;
}

//----------------------------------------------------------------
// usage
//
static void
usage(char ** argv, const char * message = NULL)
{
  std::cerr
    << "USAGE: " << argv[0]
    << " -what [apprenticevideo|apprenticevideo-classic|aeyaeremux|yaetv]"
    << " -allow dlls;allowed;search;path;list"
    << " -wix-candle pathWixCandleExe"
    << " -wix-light pathWixLightExe"
    << " -vc-redist pathVCRedistMsm"
    << " -icon pathToIconFile"
    << " -url helpLinkURL"
    << " -deploy pathto.exe [pathto.dll]*"
    << " -deploy-to targetSubFolder sourceFolder regex"
    << std::endl;

  if (message != NULL)
  {
    std::cerr << "ERROR: " << message << std::endl;
  }

  std::cerr << "VERSION: " << YAE_REVISION_TIMESTAMP
            << std::endl;
  ::exit(1);
}

//----------------------------------------------------------------
// main
//
int
main(int argc, char ** argv)
{
  // dump command line to stderr, for easier troubleshooting:
  {
    for (int i = 0; i < argc; i++)
    {
      std::cerr << argv[i] << ' ';
    }
    std::cerr << std::endl;
  }

  // get runtime parameters:
  std::string what;
  std::string dependsExe;
  std::string allowedPaths;
  std::string vcRedistMsm;
  std::string wixCandleExe;
  std::string wixLightExe;
  std::string iconFile;
  std::string helpLink;
  std::list<std::string> deploy;
  std::map<std::string, std::set<std::string> > deployTo;
  std::map<std::string, std::string> deployFrom;

  for (int i = 1; i < argc; i++)
  {
    if (strcmp(argv[i], "-allow") == 0)
    {
      if ((argc - i) <= 1) usage(argv, "malformed -allow parameter");
      i++;
      allowedPaths.assign(argv[i]);
    }
    else if (strcmp(argv[i], "-vc-redist") == 0)
    {
      if ((argc - i) <= 1) usage(argv, "malformed -vc-redist parameter");
      i++;
      vcRedistMsm.assign(argv[i]);
    }
    else if (strcmp(argv[i], "-wix-candle") == 0)
    {
      if ((argc - i) <= 1) usage(argv, "malformed -wix-candle parameter");
      i++;
      wixCandleExe.assign(argv[i]);
    }
    else if (strcmp(argv[i], "-wix-light") == 0)
    {
      if ((argc - i) <= 1) usage(argv, "malformed -wix-light parameter");
      i++;
      wixLightExe.assign(argv[i]);
    }
    else if (strcmp(argv[i], "-icon") == 0)
    {
      if ((argc - i) <= 1) usage(argv, "malformed -icon parameter");
      i++;
      iconFile.assign(argv[i]);
    }
    else if (strcmp(argv[i], "-url") == 0)
    {
      if ((argc - i) <= 1) usage(argv, "malformed -url parameter");
      i++;
      helpLink.assign(argv[i]);
    }
    else if (strcmp(argv[i], "-deploy-to") == 0)
    {
      if ((argc - i) <= 3) usage(argv, "malformed -deploy-to parameters");
      i++;
      std::string dst = tolower(fs::path(argv[i]).make_preferred().string());
      i++;
      std::string src = tolower(fs::path(argv[i]).make_preferred().string());
      i++;
      std::string regex = tolower(fs::path(argv[i]).make_preferred().string());

      deployFrom[dst] = src;
      deployTo[dst] = std::set<std::string>();
      yae::CollectMatchingFiles visitor(deployTo[dst], regex);
      yae::for_each_file_at(src, visitor);
    }
    else if (strcmp(argv[i], "-deploy") == 0)
    {
      if ((argc - i) <= 1) usage(argv, "malformed -deploy parameter");
      i++;
      std::string path = tolower(fs::path(argv[i]).make_preferred().string());
      if (!has(deploy, path))
      {
        deploy.push_back(path);
      }
    }
    else if (strcmp(argv[i], "-what") == 0)
    {
      if ((argc - i) <= 1) usage(argv, "malformed -what parameter");
      i++;
      what.assign(argv[i]);
    }
    else if (!(deploy.empty() || argv[i][0] == '-'))
    {
      std::string path = tolower(fs::path(argv[i]).make_preferred().string());
      if (!has(deploy, path))
      {
        deploy.push_back(path);
      }
    }
    else
    {
      usage(argv, argv[i]);
    }
  }

  InstallerData data;

  if (what == "apprenticevideo")
  {
    data.installer_name_ = "apprenticevideo";
    data.product_id_ = "ApprenticeVideo";
    data.product_name_ = "Apprentice Video";
    data.guid_upgrade_ = "a4a297db-1d6c-4320-b015-80add2a8d07c";
    data.comments_ = "A Video Player";

    static const char * supported[] = {
      "3gp", "aac", "ac3", "aiff", "asf", "avi", "divx", "dv", "flv", "f4v",
      "mod", "mov", "mpeg", "mpg", "mp3", "mp4", "m2t", "m2ts",
      "m2v", "m4a", "m4v", "mka", "mkv", "mts", "mxf", "ogg", "ogm", "ogv",
      "ra", "rm", "ts", "vob", "wav", "wma", "wmv",
      "weba", "webm"
    };

    static const std::size_t n_ext = sizeof(supported) / sizeof(supported[0]);
    data.ext_.resize(n_ext);
    for (std::size_t i = 0; i < n_ext; i++)
    {
      data.ext_[i] = supported[i];
    }
  }
  else if (what == "apprenticevideo-classic")
  {
    data.installer_name_ = "apprenticevideo-classic";
    data.product_id_ = "ApprenticeVideoClassic";
    data.product_name_ = "Apprentice Video Classic";
    data.guid_upgrade_ = "40fbf9bf-db60-459c-bd2a-444b306ed21d";
    data.comments_ = "A Lightweight Video Player";

    static const char * supported[] = {
      "3gp", "aac", "ac3", "aiff", "asf", "avi", "divx", "dv", "flv", "f4v",
      "mod", "mov", "mpeg", "mpg", "mp3", "mp4", "m2t", "m2ts",
      "m2v", "m4a", "m4v", "mka", "mkv", "mts", "mxf", "ogg", "ogm", "ogv",
      "ra", "rm", "ts", "vob", "wav", "wma", "wmv",
      "weba", "webm"
    };

    static const std::size_t n_ext = sizeof(supported) / sizeof(supported[0]);
    data.ext_.resize(n_ext);
    for (std::size_t i = 0; i < n_ext; i++)
    {
      data.ext_[i] = supported[i];
    }
  }
  else if (what == "aeyaeremux")
  {
    data.installer_name_ = "aeyaeremux";
    data.product_id_ = "AeyaeRemux";
    data.product_name_ = "Aeyae Remux";
    data.guid_upgrade_ = "9CEE9377-E8C7-491F-8BB9-2061A8A90C44";
    data.comments_ = "A Video Remuxer";

    static const char * supported[] = {
      "3gp", "asf", "avi", "divx", "dv", "flv", "f4v",
      "mov", "mpeg", "mpg", "mp4", "m2t", "m2ts",
      "m2v", "m4v", "mkv", "mts", "mxf", "ogm", "ogv",
      "ts", "wmv","webm", "yaerx"
    };

    static const std::size_t n_ext = sizeof(supported) / sizeof(supported[0]);
    data.ext_.resize(n_ext);
    for (std::size_t i = 0; i < n_ext; i++)
    {
      data.ext_[i] = supported[i];
    }
  }
  else if (what == "yaetv")
  {
    data.installer_name_ = "yaetv";
    data.product_id_ = "yaetv";
    data.product_name_ = "yaetv";
    data.guid_upgrade_ = "5E37BF6D-0389-47AD-9CC1-54EBC31BD292";
    data.comments_ = "Digital Video Recorder";
    data.ext_.clear();
  }
  else
  {
    usage(argv, "invalid -what parameter");
  }

  if (deploy.empty())
  {
    usage(argv, "missing -deploy parameter");
  }

  if (vcRedistMsm.empty())
  {
    usage(argv, "missing -vc-redist parameter");
  }

  if (wixCandleExe.empty())
  {
    usage(argv, "missing -wix-candle parameter");
  }

  if (wixLightExe.empty())
  {
    usage(argv, "missing -wix-light parameter");
  }

  if (iconFile.empty())
  {
    usage(argv, "missing -icon parameter");
  }

  if (helpLink.empty())
  {
    usage(argv, "missing -url parameter");
  }

  return make_installer(data,
                        dependsExe,
                        allowedPaths,
                        vcRedistMsm,
                        wixCandleExe,
                        wixLightExe,
                        iconFile,
                        helpLink,
                        deploy,
                        deployTo,
                        deployFrom);
}
