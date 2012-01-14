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
#include <stdio.h>
#include <sstream>
#include <iomanip>
#include <fstream>
#include <string>
#include <vector>
#include <list>

// local imports:
#include <yaeVersion.h>


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
// main
// 
int
main(int argc, char ** argv)
{
  for (int i = 0; i < argc; i++)
  {
    std::cerr << i << '\t' << argv[i] << std::endl;
  }
  
  // get runtime parameters:
  if (argc != 7)
  {
    std::cerr << "USAGE: " << argv[0]
              << " module pathToIconFile"
              << " pathToDependsExe dlls;allowed;search;path;list"
              << " pathWixCandleExe pathWixLightExe"
              << std::endl;
    return 1;
  }
  
  std::string dependsLog("depends-exe-log.txt");
  std::string module(argv[1]);
  std::string iconFile(argv[2]);
  std::string dependsExe(argv[3]);
  std::string allowedPaths(argv[4]);
  std::string wixCandleExe(argv[5]);
  std::string wixLightExe(argv[6]);

  // call depends.exe:
  {
    std::ostringstream os;
    os << dependsExe << " /c /a:0 /f:1 /ot:" << dependsLog << " " << module;
    
    std::string cmd(os.str().c_str());
    std::cerr << cmd << std::endl;
    
    int r = system(cmd.c_str());
    std::cerr << dependsExe << " returned: " << r << std::endl;
  }
  
  // parse allowed paths:
  std::string head;
  std::string tail;
  
  std::list<std::string> allowed;
  while (allowedPaths.size())
  {
    if (detect(";", allowedPaths, head, tail))
    {
      if (head.size() > 1)
      {
        allowed.push_back(tolower(head.substr(0, head.size() - 1)));
      }
      
      allowedPaths = tail;
    }
    else
    {
      allowed.push_back(tolower(allowedPaths));
      break;
    }
  }

  // parse depends.exe log:
  FILE * in = fopen(dependsLog.c_str(), "rb");
  std::vector<std::string> deps;
  deps.push_back(module);
  
  std::list<char> tmpAcc;
  TState state = kParsing;
  while (true)
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
          if (detect("e", line, head, tail) || // load failure
              detect("?", line, head, tail) || // missing
              detect("^", line, head, tail) || // duplicate
              detect("!", line, head, tail))   // invalid
          {
            continue;
          }

          // check if the path is allowed:
          for (std::list<std::string>::const_iterator i = allowed.begin();
               i != allowed.end(); ++i)
          {
            const std::string & pfx = *i;
            if (detect(pfx.c_str(), path, head, tail))
            {
              std::cerr << "depends: " << path << std::endl;
              deps.push_back(path);
              break;
            }
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

  std::string installerName;
  {
    std::ostringstream os;
    os << "apprenticevideo-revision-" << YAE_REVISION;
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
  std::string guidUpgrade = makeGuidStr();
  
  out << " <Product Name='Apprentice Video' "
      << "Id='" << guidProduct << "' "
      << "UpgradeCode='" << guidUpgrade << "' "
      << "Language='1033' Codepage='1252' "
      << "Version='0.0.0." << YAE_REVISION << "' "
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
      << "InstallerVersion='200' "
      << "Languages='1033' Compressed='yes' SummaryCodepage='1252' />"
      << std::endl;

  out << "  <Media Id='1' Cabinet='product.cab' EmbedCab='yes' />\n"
      << "  <Directory Id='TARGETDIR' Name='SourceDir'>\n";
  
#ifdef _WIN64
  out << "   <Directory Id='ProgramFiles64Folder' Name='PFiles'>\n";
#else
  out << "   <Directory Id='ProgramFilesFolder' Name='PFiles'>\n";
#endif
  
  out << "    <Directory Id='ApprenticeVideo' Name='Apprentice Video'>"
      << std::endl;

  
  std::string icon = getFileName(iconFile);
  
  for (std::size_t i = 0; i < deps.size(); i++)
  {
    const std::string & path = deps[i];
    std::string name = getFileName(path);
    std::string guid = makeGuidStr();
    
    out << "     <Component Id='Component" << i << "' Guid='" << guid << "'"
#ifdef _WIN64
        << " Win64='yes'"
#else
        << " Win64='no'"
#endif
        << ">" << std::endl;

    if (i == 0)
    {
      // executable:
      out << "      <File Id='File" << i << "' "
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
        "gif", "jpg", "mod", "mov", "mpeg", "mpg", "mp3", "mp4", "m2t", "m2ts",
        "m2v", "m4a", "m4v", "mka", "mkv", "mts", "mxf", "ogg", "ogm", "ogv",
        "png", "ra", "rm", "tif", "tiff", "ts", "vob", "wav", "wma", "wmv",
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
            << "      </ProgId>"
            << std::endl;
      }
    }
    else
    {
      // dlls:
      out << "      <File Id='File" << i << "' "
          << "Name='" << name << "' DiskId='1' "
          << "Source='" << path << "' "
          << "KeyPath='yes' />\n";
    }
    
    out << "     </Component>"
        << std::endl;
  }

  out << "    </Directory>\n"
      << "   </Directory>"
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
  for (std::size_t i = 0; i < deps.size(); ++i)
  {
    out << "   <ComponentRef Id='Component" << i << "' />\n";
  }
  
  out << "   <ComponentRef Id='ProgramMenuDir' />\n"
      << "  </Feature>\n"
      << "  <Icon Id='" << icon << "' "
      << "SourceFile='" << iconFile << "' />"
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
  }
  
  // call light.exe:
  {
    std::ostringstream os;
    os << '"' << wixLightExe << "\" " << installerName << ".wixobj";
    
    std::string cmd(os.str().c_str());
    std::cerr << cmd << std::endl;
    
    int r = system(cmd.c_str());
    std::cerr << wixLightExe << " returned: " << r << std::endl;
  }
  
  return 0;
}
