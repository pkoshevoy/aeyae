// -*- Mode: c++; tab-width: 8; c-basic-offset: 2; indent-tabs-mode: nil -*-
// NOTE: the first line of this file sets up source code indentation rules
// for Emacs; it is also a hint to anyone modifying this file.

// Created   : Fri Nov 25 15:31:57 MST 2011
// Copyright : Pavel Koshevoy
// License   : MIT -- http://www.opensource.org/licenses/mit-license.php

// system imports:
#include <windows.h>
#include <iostream>
#include <string.h>
#include <stdio.h>
#include <sstream>
#include <iomanip>
#include <string>
#include <vector>
#include <list>


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
              << " module pathToDependsExe pathWixCandleExe pathWixLitExe pathWixLightExe allowed;search;path;list"
              << std::endl;
    return 1;
  }
  
  std::string module(argv[1]);
  std::string dependsExe(argv[2]);
  std::string dependsLog("depends-exe-log.txt");
  std::string wixCandleExe(argv[3]);
  std::string wixLitExe(argv[4]);
  std::string wixLightExe(argv[5]);
  std::string allowedPaths(argv[6]);

  // call depends.exe:
  {
    std::ostringstream os;
    os << dependsExe << " /c /a:0 /f:1 /ot:" << dependsLog << " " << module;
    
    std::string cmd(os.str().c_str());
    std::cerr << cmd << std::endl;
    
    int r = system(cmd.c_str());
    std::cerr << "returned: " << r << std::endl;
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
  std::list<char> tmpAcc;
  
  unsigned int wsxIndex = 0;
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
              deps.push_back(path);
              /*
              std::ostringstream os;
              os << "heat.exe file "
                 << path << " -gg -g1 -sfrag -srd -out heat-"
                 << std::setw(3)
                 << std::setfill('0')
                 << wsxIndex
                 << ".wsx";
              
              std::string cmd(os.str().c_str());
              std::cout << path << "\n" << cmd << std::endl;
              system(cmd.c_str());
              */
              std::cerr << path << std::endl;
              wsxIndex++;
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
  
  return 0;
}
