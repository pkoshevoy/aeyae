// -*- Mode: c++; tab-width: 8; c-basic-offset: 2; indent-tabs-mode: nil -*-
// NOTE: the first line of this file sets up source code indentation rules
// for Emacs; it is also a hint to anyone modifying this file.

// Created   : Sun Mar 18 11:01:36 MDT 2012
// Copyright : Pavel Koshevoy
// License   : MIT -- http://www.opensource.org/licenses/mit-license.php

// yamka includes:
#include <yamkaCache.h>

// system includes:
#include <assert.h>
#include <iostream>
#include <string.h>
#include <vector>


namespace Yamka
{

  //----------------------------------------------------------------
  // TCache::TLine
  //
  struct TCache::TLine
  {

    //----------------------------------------------------------------
    // TLine
    //
    TLine(std::size_t size):
      data_(size),
      age_(0)
    {
      init((std::size_t)(~0));
    }

    //----------------------------------------------------------------
    // init
    //
    inline void init(uint64 addr)
    {
      head_ = addr;
      tail_ = head_;
      ready_ = false;
      dirty_ = false;
    }

    //----------------------------------------------------------------
    // contains
    //
    inline bool contains(uint64 addr) const
    { return head_ <= addr && addr < tail_; }

    std::vector<unsigned char> data_;
    uint64 head_;
    uint64 tail_;
    uint64 age_;
    bool ready_;
    bool dirty_;
  };


  //----------------------------------------------------------------
  // TCache::TCache
  //
  TCache::TCache(ICacheDataProvider * provider,
                 std::size_t maxLines,
                 std::size_t lineSize):
    provider_(provider),
    lineSize_(lineSize),
    numLines_(0),
    age_(0)
  {
    lines_.assign(maxLines, NULL);
  }

  //----------------------------------------------------------------
  // TCache::~TCache
  //
  TCache::~TCache()
  {
    clear();
  }

  //----------------------------------------------------------------
  // TCache::clear
  //
  void
  TCache::clear()
  {
    const std::size_t numLines = lines_.size();
    for (std::size_t i = 0; i < numLines; i++)
    {
      TLine * line = lines_[i];
      this->flush(line);
      delete line;
      lines_[i] = NULL;
    }

    numLines_ = 0;
    age_ = 0;
  }

  //----------------------------------------------------------------
  // TCache::resize
  //
  void
  TCache::resize(std::size_t maxLines, std::size_t lineSize)
  {
    if (lineSize_ != lineSize || lines_.size() != maxLines)
    {
      clear();

      lines_.assign(maxLines, NULL);
      lineSize_ = lineSize;
    }

#if 0
    std::cerr << "cache resized: "
              << maxLines << " x " << lineSize
              << std::endl;
#endif
  }

  //----------------------------------------------------------------
  // TCache::truncate
  //
  void
  TCache::truncate(uint64 addr)
  {
    // must properly align the address:
    uint64 head = addr - addr % lineSize_;

    for (std::size_t i = 0; i < numLines_; i++)
    {
      TLine * line = lines_[i];
      if (line->head_ > head)
      {
        delete line;
        lines_[i] = NULL;
        numLines_--;
        std::swap(lines_[i], lines_[numLines_]);
      }
      else if (line->head_ == head)
      {
        assert(line->ready_);
        line->tail_ = addr;
      }
    }
  }

  //----------------------------------------------------------------
  // TCache::flush
  //
  bool
  TCache::flush(TLine * line)
  {
    if (!line || !line->dirty_ || line->tail_ <= line->head_ || !provider_)
    {
      return true;
    }

    // must flush the line:
    const unsigned char * src = &(line->data_[0]);
    std::size_t numBytes = (std::size_t)(line->tail_ - line->head_);

    bool ok = provider_->save(line->head_, numBytes, src);
    line->dirty_ = !ok;

    assert(ok);
    return ok;
  }

  //----------------------------------------------------------------
  // TCache::lookup
  //
  TCache::TLine *
  TCache::lookup(uint64 addr)
  {
    // must properly align the address:
    addr -= addr % lineSize_;

    for (std::size_t i = 0; i < numLines_; i++)
    {
      TLine * line = lines_[i];
      if (line->head_ == addr)
      {
        age_++;
        line->age_ = age_;
        return line;
      }
    }

    return NULL;
  }

  //----------------------------------------------------------------
  // TCache::addLine
  //
  TCache::TLine *
  TCache::addLine(uint64 addr)
  {
    // must properly align the address:
    addr -= addr % lineSize_;

    TLine * line = NULL;
    if (numLines_ < lines_.size())
    {
      // add another line:
      line = new TLine(lineSize_);
      lines_[numLines_] = line;
      numLines_++;
    }
    else
    {
      // find the oldest line to reuse:
      line = lines_[0];

      for (std::size_t i = 1; i < numLines_; i++)
      {
        if (lines_[i]->age_ < line->age_)
        {
          line = lines_[i];
        }
      }

      if (!this->flush(line))
      {
        assert(false);
        return NULL;
      }
    }

    age_++;
    line->age_ = age_;
    line->init(addr);

    return line;
  }

  //----------------------------------------------------------------
  // TCache::getLine
  //
  TCache::TLine *
  TCache::getLine(uint64 addr)
  {
    TLine * line = lookup(addr);
    if (!line)
    {
      line = addLine(addr);
    }

    if (line && !line->ready_)
    {
      // must properly align the address:
      uint64 head = addr - addr % lineSize_;
      std::size_t numBytes = lineSize_;
      unsigned char * dst = &(line->data_[0]);
      line->ready_ = provider_->load(head, &numBytes, dst) && numBytes;
      line->tail_ = head + numBytes;
    }

    return line;
  }

  //----------------------------------------------------------------
  // TCache::load
  //
  std::size_t
  TCache::load(uint64 addr, std::size_t size, unsigned char * dst)
  {
    if (lineSize_ < size && size <= 16777216)
    {
      adjustLineSize(size);
    }

    std::size_t todo = size;

    uint64 addr0 = addr - addr % lineSize_;
    uint64 addr1 = addr + size;

    for (uint64 head = addr0; todo && head < addr1; head += lineSize_)
    {
      TLine * line = getLine(head);
      if (!line || !line->ready_)
      {
        // assert(false);
        return 0;
      }

      assert(head == line->head_);
      unsigned char * src = &(line->data_[0]);

      uint64 a0 = (head < addr) ? addr : head;
      uint64 a1 = (line->tail_ < addr1) ? (line->tail_) : addr1;

      std::size_t lineOffset = (std::size_t)(a0 - head);
      std::size_t numBytes = (std::size_t)(a1 - a0);

      memcpy(dst, src + lineOffset, numBytes);
      dst += numBytes;
      todo -= numBytes;

      uint64 tail = head + lineSize_;
      if (tail != line->tail_)
      {
        assert(!todo);
        break;
      }
    }

    std::size_t done = size - todo;
    return done;
  }

  //----------------------------------------------------------------
  // TCache::save
  //
  std::size_t
  TCache::save(uint64 addr, std::size_t size, const unsigned char * src)
  {
    if (lineSize_ < size && size <= 16777216)
    {
      adjustLineSize(size);
    }

    std::size_t todo = size;

    uint64 addr0 = addr - addr % lineSize_;
    uint64 addr1 = addr + size;

    for (uint64 head = addr0; todo && head < addr1; head += lineSize_)
    {
      TLine * line = getLine(head);
      if (!line || !line->ready_)
      {
        assert(false);
        return 0;
      }

      assert(head == line->head_);
      unsigned char * dst = &(line->data_[0]);

      uint64 tail = head + lineSize_;
      uint64 a0 = (head < addr) ? addr : head;
      uint64 a1 = (tail < addr1) ? tail : addr1;

      std::size_t lineOffset = (std::size_t)(a0 - head);
      std::size_t numBytes = (std::size_t)(a1 - a0);

      if (line->tail_ < a1)
      {
        line->tail_ = a1;
      }

      if (numBytes)
      {
        line->ready_ = true;
        line->dirty_ = true;

        memcpy(dst + lineOffset, src, numBytes);
        src += numBytes;
        todo -= numBytes;
      }
    }

    std::size_t done = size - todo;
    return done;
  }

  //----------------------------------------------------------------
  // TCache::adjustLineSize
  //
  void
  TCache::adjustLineSize(std::size_t requestSize)
  {
    std::size_t n = (requestSize + lineSize_ - 1) / lineSize_;
    std::size_t z = n * lineSize_;
    resize(lines_.size(), z);
  }

}
