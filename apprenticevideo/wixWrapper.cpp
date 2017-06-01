// -*- Mode: c++; tab-width: 8; c-basic-offset: 2; indent-tabs-mode: nil -*-
// NOTE: the first line of this file sets up source code indentation rules
// for Emacs; it is also a hint to anyone modifying this file.

// Created   : Fri Nov 25 15:31:57 MST 2011
// Copyright : Pavel Koshevoy
// License   : MIT -- http://www.opensource.org/licenses/mit-license.php

// system imports:
#include <windows.h>
#define _OLEAUT32_
#include <unknwn.h>
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

// boost:
#include <boost/algorithm/string.hpp>
#include <boost/algorithm/string/predicate.hpp>
#include <boost/algorithm/string/replace.hpp>
#include <boost/filesystem.hpp>
#include <boost/regex.hpp>

// local imports:
#include <yaeVersion.h>

// namespace shortcut:
namespace fs = boost::filesystem;
namespace al = boost::algorithm;


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
// TState
//
enum TState
{
  kParsing,
  kFoundModuleDependencyTree,
  kFoundOpeningBracket,
  kFoundClosingBracket,
  kFoundModuleList
};

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
  std::string dst = src;
  for (std::size_t i = 0; i < src.size(); i++)
  {
    dst[i] = tolower(dst[i]);
  }

  return dst;
}

//----------------------------------------------------------------
// has
//
template <typename TDataContainer, typename TData>
bool
has(const TDataContainer & container, const TData & value)
{
  typename TDataContainer::const_iterator found =
    std::find(container.begin(), container.end(), value);

  return found != container.end();
}

//----------------------------------------------------------------
// parse_depends_log
//
static void
get_dependencies(std::vector<std::string> & deps,
                 const std::string & dependsExe,
                 const std::list<std::string> & allowed,
                 const std::string & module)
{
  std::string dependsLog("depends-exe-log.txt");
  {
    std::ostringstream os;
    os << dependsExe << " /c /a:0 /f:1 /ot:" << dependsLog << " " << module;

    std::string cmd(os.str().c_str());
    std::cerr << cmd << std::endl;

    int r = system(cmd.c_str());
    std::cerr << dependsExe << " returned: " << r << std::endl;
  }

  // parse depends.exe log:
  FILE * in = fopen(dependsLog.c_str(), "rb");
  if (!has(deps, module))
  {
    deps.push_back(module);
  }

  std::string head;
  std::string tail;
  std::list<char> tmpAcc;
  TState state = kParsing;

  while (in)
  {
    char ch = 0;
    std::size_t nb = fread(&ch, 1, 1, in);
    if (!nb)
    {
      break;
    }

    if (ch != '\r' && ch != '\n')
    {
      tmpAcc.push_back(tolower(ch));
      continue;
    }

    if (tmpAcc.empty())
    {
      continue;
    }

    std::string line(tmpAcc.begin(), tmpAcc.end());
    tmpAcc.clear();

    if (state == kParsing)
    {
      if (detect("*| module dependency tree |*", line, head, tail))
      {
        state = kFoundModuleDependencyTree;
      }
    }
    else if (state == kFoundModuleDependencyTree)
    {
      if (detect("*| module list |*", line, head, tail))
      {
        state = kFoundModuleList;
      }
      else if (detect("[", line, head, tail))
      {
        line = tail;

        std::string path;
        if (detect("] ", line, head, path))
        {
          line = head;
          if (// detect("e", line, head, tail) || // load failure
              detect("?", line, head, tail) || // missing
              detect("^", line, head, tail) || // duplicate
              detect("!", line, head, tail))   // invalid
          {
            continue;
          }

          // check if the path is allowed:
          bool allow = false;

          for (std::list<std::string>::const_iterator i = allowed.begin();
               i != allowed.end(); ++i)
          {
            const std::string & pfx = *i;
            if (detect(pfx.c_str(), path, head, tail))
            {
              if (!has(deps, path))
              {
                std::cerr << "depends: " << path << std::endl;
                deps.push_back(path);
              }

              allow = true;
              break;
            }
          }

          if (!allow)
          {
            std::cerr << "NOT ALLOWED: " << path << std::endl;
          }
        }
      }
    }
  }

  if (in)
  {
    fclose(in);
    in = NULL;
  }
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
// usage
//
static void
usage(char ** argv, const char * message = NULL)
{
  std::cerr
    << "USAGE: " << argv[0]
    << " -dep-walker pathToDependsExe"
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
// TOpenFolder
//
struct TOpenFolder
{
  TOpenFolder(const std::string & folderPathUtf8):
    path_(fs::absolute(fs::path(folderPathUtf8))),
    iter_(path_)
  {
    if (iter_ == fs::directory_iterator())
    {
      std::ostringstream oss;
      oss << "\"" << path_.string() << "\" folder is empty";
      throw std::runtime_error(oss.str().c_str());
    }
  }

  bool parseNextItem()
  {
    ++iter_;
    bool ok = iter_ != fs::directory_iterator();
    return ok;
  }

  inline std::string folderPath() const
  {
    return path_.string();
  }

  inline bool itemIsFolder() const
  {
    return
      (iter_ != fs::directory_iterator()) &&
      (fs::is_directory(iter_->path()));
  }

  inline std::string itemName() const
  {
    return iter_->path().filename().string();
  }

  inline std::string itemPath() const
  {
    return iter_->path().string();
  }

protected:
  fs::path path_;
  fs::directory_iterator iter_;
};

//----------------------------------------------------------------
// forEachFileAt
//
template <typename TVisitor>
static void
forEachFileAt(const std::string & pathUtf8, TVisitor & callback)
{
  try
  {
    TOpenFolder folder(pathUtf8);
    while (folder.parseNextItem())
    {
      std::string name = folder.itemName();
      std::string path = folder.itemPath();
      bool isSubFolder = folder.itemIsFolder();

      if (isSubFolder)
      {
        if (name == "." || name == "..")
        {
          continue;
        }
      }

      if (!callback(isSubFolder, name, path))
      {
        return;
      }

      if (isSubFolder)
      {
        forEachFileAt(path, callback);
      }
    }
  }
  catch (...)
  {
    std::string name = fs::path(pathUtf8).filename().string();
    callback(false, name, pathUtf8);
  }
}

//----------------------------------------------------------------
// CollectMatchingFiles
//
struct CollectMatchingFiles
{
  CollectMatchingFiles(std::set<std::string> & dst, const std::string & regex):
    pattern_(regex, boost::regex::icase),
    files_(dst)
  {}

  bool operator()(bool isFolder,
                  const std::string & name,
                  const std::string & path)
  {
    if (!isFolder && boost::regex_match(name, pattern_))
    {
      files_.insert(path);
    }

    return true;
  }

protected:
  boost::regex pattern_;
  std::set<std::string> & files_;
};

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
    if (strcmp(argv[i], "-dep-walker") == 0)
    {
      if ((argc - i) <= 1) usage(argv, "malformed -dep-walker parameter");
      i++;
      dependsExe.assign(argv[i]);
    }
    else if (strcmp(argv[i], "-allow") == 0)
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
      CollectMatchingFiles visitor(deployTo[dst], regex);
      forEachFileAt(src, visitor);
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

  if (deploy.empty())
  {
    usage(argv, "missing -deploy parameter");
  }

  if (dependsExe.empty())
  {
    usage(argv, "missing -dep-walker parameter");
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

  // parse allowed paths:
  std::list<std::string> allowed;
  {
    std::string nativePaths;
    std::string paths = allowedPaths;
    std::string head;
    std::string tail;

    while (paths.size())
    {
      if (detect(";", paths, head, tail))
      {
        if (head.size() > 1)
        {
          std::string path = tolower(head.substr(0, head.size() - 1));
          path = fs::path(path).make_preferred().string();
          allowed.push_back(path);
          append_path(nativePaths, path);
        }

        paths = tail;
      }
      else
      {
        std::string path = tolower(paths);
        path = fs::path(path).make_preferred().string();
        allowed.push_back(path);
        append_path(nativePaths, path);
        break;
      }
    }

    allowedPaths = nativePaths;
  }

  // add allowed paths to env PATH, so Dependency Walker would search there:
  {
    std::string path;
    const char * pathEnv = getenv("PATH");
    if (pathEnv)
    {
      path = pathEnv;
    }

    std::size_t pathSize = path.size();
    if (pathSize && path[pathSize - 1] != ';')
    {
      path += ';';
    }

    path += allowedPaths;
    _putenv((std::string("PATH=") + path).c_str());
  }

  // call depends.exe:
  std::vector<std::string> deps;
  for (std::list<std::string>::iterator
         i = deploy.begin(); i != deploy.end(); ++i)
  {
    const std::string & module = *i;
    get_dependencies(deps, dependsExe, allowed, module);
  }

  std::string installerName;
  {
    std::ostringstream os;
    os << "apprenticevideo-" << YAE_REVISION;
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

  std::string guidProduct = makeGuidStr();
  std::string guidUpgrade = "a4a297db-1d6c-4320-b015-80add2a8d07c";

  unsigned int major = 0;
  unsigned int minor = 0;
  unsigned int patch = 0;
  yae_version(&major, &minor, &patch);

  out << " <Product Name='Apprentice Video' "
      << "Id='" << guidProduct << "' "
      << "UpgradeCode='" << guidUpgrade << "' "
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
      << "Description='Apprentice Video Installer' "
      << "Comments='A video player' "
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

  out << "    <Directory Id='ApprenticeVideo' Name='Apprentice Video'>"
      << std::endl;


  std::string icon = getFileName(iconFile);
  std::size_t fileIndex = 0;

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

    if (fileIndex == 0)
    {
      // executable:
      out << "      <File Id='File" << fileIndex << "' "
          << "Name='" << name << "' DiskId='1' "
          << "Source='" << path << "' "
          << "KeyPath='yes'>"
          << std::endl;

      out << "       <Shortcut Id='startmenuApprenticeVideo' "
          << "Directory='ProgramMenuDir' "
          << "Name='Apprentice Video' "
          << "WorkingDirectory='INSTALLDIR' "
          << "Icon='" << icon << "' "
          << "IconIndex='0' "
          << "Advertise='yes' />"
          << std::endl;

      out << "       <Shortcut Id='desktopApprenticeVideo' "
          << "Directory='DesktopFolder' "
          << "Name='Apprentice Video' "
          << "WorkingDirectory='INSTALLDIR' "
          << "Icon='" << icon <<"' "
          << "IconIndex='0' "
          << "Advertise='yes' />"
          << std::endl;

      out << "      </File>"
          << std::endl;

      static const char * supported[] = {
        "3gp", "aac", "ac3", "aiff", "asf", "avi", "divx", "dv", "flv", "f4v",
        "mod", "mov", "mpeg", "mpg", "mp3", "mp4", "m2t", "m2ts",
        "m2v", "m4a", "m4v", "mka", "mkv", "mts", "mxf", "ogg", "ogm", "ogv",
        "ra", "rm", "ts", "vob", "wav", "wma", "wmv",
        "weba", "webm" };

      std::size_t numSupported = sizeof(supported) / sizeof(supported[0]);
      for (std::size_t j = 0; j < numSupported; j++)
      {
        const char * ext = supported[j];
        out << "      <ProgId Id='ApprenticeVideo." << ext << "' "
            << "Icon='" << icon << "' IconIndex='0' Advertise='yes' "
            << "Description='media file format "
            << "supported by Apprentice Video'>\n"
            << "       <Extension Id='" << ext << "'>\n"
            << "        <Verb Id='open' Command='Open' "
            << "Argument='&quot;%1&quot;' />\n"
          // << "        <MIME Advertise='yes' "
          // << "ContentType='video/x-matroska' Default='yes' />\n"
            << "       </Extension>\n"
            << "      </ProgId>\n"
            << std::endl;
      }
    }
    else
    {
      // dlls:
      out << "      <File Id='File" << fileIndex << "' "
          << "Name='" << name << "' DiskId='1' "
          << "Source='" << path << "' "
          << "KeyPath='yes' />\n";
    }

    out << "     </Component>\n"
        << std::endl;
  }

  typedef std::map<std::string, std::set<std::string> > TDeployTo;
  for (TDeployTo::const_iterator
         i = deployTo.begin(); i != deployTo.end(); ++i)
  {
    const std::string & dst = i->first;
    const std::string & src = deployFrom[dst];
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
      << "    <Directory Id='ProgramMenuDir' Name='Apprentice Video'>\n"
      << "     <Component Id='ProgramMenuDir' Guid='" << makeGuidStr() << "'>\n"
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

  out << "  <Feature Id='Complete' Title='Apprentice Video' Level='1'>\n";
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
