// -*- Mode: c++; tab-width: 8; c-basic-offset: 2; indent-tabs-mode: nil -*-
// NOTE: the first line of this file sets up source code indentation rules
// for Emacs; it is also a hint to anyone modifying this file.

// Created      : Sun Dec 29 15:13:25 MST 2013
// Copyright    : Pavel Koshevoy
// License      : MIT -- http://www.opensource.org/licenses/mit-license.php

#ifndef YAE_WAV_FILE_H_
#define YAE_WAV_FILE_H_

// aeyae:
#include "yae/api/yae_api.h"
#include "yae/utils/yae_utils.h"

// standard:
#include <string>
#include <stdio.h>


namespace yae
{
  //----------------------------------------------------------------
  // WavFile
  //
  // Helper for dumping audio data to .wav file(s),
  // useful for debugging audio problems.
  //
  struct YAE_API WavFile
  {
    enum { kHeadSz = 12 };
    enum { kFrmtSz = 24 };

    WavFile():
      file_(NULL),
      nout_(0),
      nstart_(0),
      sampleRate_(0),
      numChannels_(0),
      bitsPerSample_(0),
      floatSamples_(false)
    {}

    ~WavFile()
    {
      close();
    }

    void close()
    {
      if (!file_)
      {
        return;
      }

      // re-write RIFF chunk size:
      unsigned int uint4b = 0;

      int err = fseek(file_, 4, SEEK_SET);
      if (!err)
      {
        uint4b = kHeadSz + kFrmtSz + nout_;
        fwrite(&uint4b, 1, 4, file_);
      }

      // re-write data chunk size:
      err = fseek(file_, 40, SEEK_SET);
      if (!err)
      {
        uint4b = (unsigned int)nout_;
        fwrite(&uint4b, 1, 4, file_);
      }

      fclose(file_);
      file_ = NULL;
      nstart_ += nout_;
      nout_ = 0;
    }

    bool open(const char * fn, const AudioTraits & atts)
    {
      int nchan = atts.ch_layout_.nb_channels;
      int sampleRate = (unsigned int)(atts.sample_rate_);
      int bitsPerSample = atts.get_bytes_per_sample() * 8;
      bool floatSamples = atts.sample_format_ == AV_SAMPLE_FMT_FLT;
      return open(fn, nchan, sampleRate, bitsPerSample, floatSamples);
    }

    bool open(const char * fn,
              unsigned int nchan,
              unsigned int sampleRate,
              unsigned int bitsPerSample,
              bool floatSamples = false)
    {
      close();

      file_ = fopenUtf8(fn, "wb");
      if (!file_)
      {
        return false;
      }

      numChannels_ = nchan;
      sampleRate_ = sampleRate;
      bitsPerSample_ = bitsPerSample;
      floatSamples_ = floatSamples;

      return open(fn);
    }

    bool save(unsigned int numSamples,
              const void * samples,
              const char * newFilePrefix = NULL)
    {
      std::size_t size = (numSamples * numChannels_ * bitsPerSample_) / 8;
      return save(samples, size, newFilePrefix);
    }

    bool save(const void * data,
              std::size_t dataSize,
              const char * newFilePrefix = NULL)
    {
      if (newFilePrefix)
      {
        close();

        std::ostringstream os;
        TTime t(nstart_, (bitsPerSample_ * numChannels_ * sampleRate_) / 8);

        os << newFilePrefix << t.to_hhmmss_us("", ".") << ".wav";
        open(os.str().c_str());
      }

      if (!file_)
      {
        return false;
      }

      const unsigned char * src = (const unsigned char *)data;
      const unsigned char * end = src + dataSize;

      while (src < end)
      {
        std::size_t z = (end - src);
        std::size_t n = fwrite(src, 1, z, file_);

        if (n == 0 && ferror(file_))
        {
          YAE_ASSERT(false);
          return false;
        }

        nout_ += n;
        src += n;
      }

      return true;
    }

    static bool save(const char * fn,
                     const AudioTraits & atts,
                     const void * data,
                     std::size_t size)
    {
      WavFile wav;

      bool ok = wav.open(fn, atts);
      if (ok)
      {
        ok = wav.save(data, size);
      }

      return ok;
    }

    // for debugging:
    std::string nextFileTimestamp() const
    {
      std::size_t start = nstart_ + nout_;
      TTime t(start, (bitsPerSample_ * numChannels_ * sampleRate_) / 8);
      std::string ts = t.to_hhmmss_us("", ".");
      return ts;
    }

  private:
    WavFile(const WavFile &);
    WavFile & operator = (const WavFile &);

    // helper:
    bool open(const char * fn)
    {
      close();

      file_ = fopenUtf8(fn, "wb");
      if (!file_)
      {
        return false;
      }

      // write the file header:
      unsigned short int uint2b = 0;
      unsigned int uint4b = 0;

      // 'RIFF' chunk:
      fwrite("RIFF", 1, 4, file_);
      uint4b = kHeadSz + kFrmtSz;
      fwrite(&uint4b, 1, 4, file_);
      fwrite("WAVE", 1, 4, file_);

      // 'fmt ' sub-chunk:
      fwrite("fmt ", 1, 4, file_);

      // format structure size:
      uint4b = 16;
      fwrite(&uint4b, 1, 4, file_);

      // PCM == 1, IEEE FLOAT == 3:
      uint2b = floatSamples_ ? 3 : 1;
      fwrite(&uint2b, 1, 2, file_);

      // number of channels:
      uint2b = (unsigned short int)numChannels_;
      fwrite(&uint2b, 1, 2, file_);

      // sample rate:
      uint4b = sampleRate_;
      fwrite(&uint4b, 1, 4, file_);

      // byte rate:
      uint4b = (bitsPerSample_ * numChannels_ * sampleRate_) / 8;
      fwrite(&uint4b, 1, 4, file_);

      // block align (stride):
      uint2b = (bitsPerSample_ * numChannels_) / 8;
      fwrite(&uint2b, 1, 2, file_);

      // bits per sample:
      uint2b = (unsigned short int)bitsPerSample_;
      fwrite(&uint2b, 1, 2, file_);

      // 'data' sub-chunk:
      fwrite("data", 1, 4, file_);
      uint4b = 0;
      fwrite(&uint4b, 1, 4, file_);

      return true;
    }

    FILE * file_;
    std::size_t nout_;
    std::size_t nstart_;
    unsigned int sampleRate_;
    unsigned int numChannels_;
    unsigned int bitsPerSample_;
    bool floatSamples_;
  };
}


#endif // YAE_WAV_FILE_H_
