// -*- Mode: c++; tab-width: 8; c-basic-offset: 2; indent-tabs-mode: nil -*-
// NOTE: the first line of this file sets up source code indentation rules
// for Emacs; it is also a hint to anyone modifying this file.

// Created   : Thu May 27 21:03:47 MDT 2010
// Copyright : Pavel Koshevoy
// License   : MIT -- http://www.opensource.org/licenses/mit-license.php

// aeyae:
#include "yae_ffmpeg_utils.h"

// standard C++ library:
#include <string>
#include <cstring>


namespace yae
{

  //----------------------------------------------------------------
  // dump_averror
  //
  std::ostream &
  dump_averror(std::ostream & os, int err)
  {
#ifdef AV_ERROR_MAX_STRING_SIZE
    static const std::size_t buffer_size = AV_ERROR_MAX_STRING_SIZE;
#else
    static const std::size_t buffer_size = 256;
#endif

    char errbuf[buffer_size] = { 0 };
    av_strerror(err, errbuf, sizeof(errbuf));
    os << "AVERROR: " << errbuf << std::endl;
    return os;
  }

  //----------------------------------------------------------------
  // lookup_src
  //
  AVFilterContext *
  lookup_src(AVFilterContext * filter, const char * name)
  {
    if (!filter)
    {
      return NULL;
    }

    if (filter->nb_inputs == 0 &&
        filter->nb_outputs == 1 &&
        std::strcmp(filter->filter->name, name) == 0)
    {
      return filter;
    }

    for (unsigned int i = 0; i < filter->nb_inputs; i++)
    {
      AVFilterContext * found = lookup_src(filter->inputs[i]->src, name);
      if (found)
      {
        return found;
      }
    }

    return NULL;
  }

  //----------------------------------------------------------------
  // lookup_sink
  //
  AVFilterContext *
  lookup_sink(AVFilterContext * filter, const char * name)
  {
    if (!filter)
    {
      return NULL;
    }

    if (filter->nb_inputs == 1 &&
        filter->nb_outputs == 0 &&
        std::strcmp(filter->filter->name, name) == 0)
    {
      return filter;
    }

    for (unsigned int i = 0; i < filter->nb_outputs; i++)
    {
      AVFilterContext * found = lookup_sink(filter->outputs[i]->dst, name);
      if (found)
      {
        return found;
      }
    }

    return NULL;
  }

}
