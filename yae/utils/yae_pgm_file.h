// -*- Mode: c++; tab-width: 8; c-basic-offset: 2; indent-tabs-mode: nil -*-
// NOTE: the first line of this file sets up source code indentation rules
// for Emacs; it is also a hint to anyone modifying this file.

// Created      : Sun Dec 29 15:13:25 MST 2013
// Copyright    : Pavel Koshevoy
// License      : MIT -- http://www.opensource.org/licenses/mit-license.php

#ifndef YAE_PGM_FILE_H_
#define YAE_PGM_FILE_H_

// aeyae:
#include "yae/api/yae_api.h"
#include "yae/utils/yae_utils.h"

// standard:
#include <string>
#include <stdio.h>


namespace yae
{

  //----------------------------------------------------------------
  // PgmFile
  //
  // Helper for dumping YUV data to .pgm file(s),
  // useful for debugging video problems.
  //
  struct YAE_API PgmFile
  {
    PgmFile(const char * filename = NULL):
      file_(NULL)
    {
      if (filename)
      {
        open(filename);
      }
    }

    ~PgmFile()
    {
      close();
    }

    PgmFile & close()
    {
      if (file_)
      {
        fclose(file_);
        file_ = NULL;
      }

      return *this;
    }

    PgmFile & open(const char * filename)
    {
      close();

      if (filename)
      {
        file_ = fopenUtf8(filename, "wb");
      }

      return *this;
    }

    bool save(const unsigned char * data, int w, int h)
    {
      if (!file_)
      {
        return false;
      }

      std::string header;
      header += "P5\n";
      header += toText(w) + ' '  + toText(h) + '\n';
      header += "255\n";
      fwrite(header.c_str(), 1, header.size(), file_);

      const std::size_t size = w * h;
      const unsigned char * src = data;
      const unsigned char * end = data + size;

      while (src < end)
      {
        std::size_t z = (end - src);
        std::size_t n = fwrite(src, 1, z, file_);

        if (n == 0 && ferror(file_))
        {
          YAE_ASSERT(false);
          close();
          return false;
        }

        src += n;
      }

      close();
      return true;
    }

  private:
    PgmFile(const PgmFile &);
    PgmFile & operator = (const PgmFile &);

    FILE * file_;
  };
}


#endif // YAE_PGM_FILE_H_
