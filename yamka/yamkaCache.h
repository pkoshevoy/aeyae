// -*- Mode: c++; tab-width: 8; c-basic-offset: 2; indent-tabs-mode: nil -*-
// NOTE: the first line of this file sets up source code indentation rules
// for Emacs; it is also a hint to anyone modifying this file.

// Created   : Sun Mar 18 11:01:36 MDT 2012
// Copyright : Pavel Koshevoy
// License   : MIT -- http://www.opensource.org/licenses/mit-license.php

#ifndef YAMKA_CACHE_H_
#define YAMKA_CACHE_H_

// system includes:
#include <vector>

// boost includes:
#include <boost/cstdint.hpp>


namespace Yamka
{
  
  //----------------------------------------------------------------
  // uint64
  // 
  typedef boost::uint64_t uint64;
  
  
  //----------------------------------------------------------------
  // ICacheDataProvider
  // 
  struct ICacheDataProvider
  {
    virtual ~ICacheDataProvider() {}
    
    virtual bool
    load(uint64 addr, std::size_t * size, unsigned char * dst) = 0;
    
    virtual bool
    save(uint64 addr, std::size_t size, const unsigned char * src) = 0;
  };
  
  
  //----------------------------------------------------------------
  // TCache
  //
  class TCache
  {
    // intentionally disabled:
    TCache();
    TCache(const TCache &);
    TCache & operator = (const TCache &);
    
  public:
    
    TCache(ICacheDataProvider * provider,
           std::size_t maxLines,
           std::size_t lineSize);
    ~TCache();
    
    void clear();
    void resize(std::size_t maxLines, std::size_t lineSize);
    void truncate(uint64 addr);
    
    struct TLine;
    bool flush(TLine * line);
    
    TLine * lookup(uint64 addr);
    TLine * addLine(uint64 addr);
    TLine * getLine(uint64 addr);
    
    std::size_t load(uint64 addr, std::size_t size, unsigned char * dst);
    std::size_t save(uint64 addr, std::size_t size, const unsigned char * src);
    
  protected:
    void adjustLineSize(std::size_t requestSize);
    
    ICacheDataProvider * provider_;
    std::size_t lineSize_;
    std::size_t numLines_;
    std::vector<TLine *> lines_;
    uint64 age_;
  };
}


#endif // YAMKA_CACHE_H_
