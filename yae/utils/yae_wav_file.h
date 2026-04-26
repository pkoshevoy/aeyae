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
#include "yae/video/yae_video.h"

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

    WavFile();
    ~WavFile();

    void close();

    bool open(const std::string & fn, const AudioTraits & atts);

    bool open(const std::string & fn,
              unsigned int nchan,
              unsigned int sampleRate,
              unsigned int bitsPerSample,
              bool floatSamples = false);

    bool save(unsigned int numSamples,
              const void * samples,
              const char * newFilePrefix = NULL);

    bool save(const void * data,
              std::size_t dataSize,
              const char * newFilePrefix = NULL);

    static bool save(const char * fn,
                     const AudioTraits & atts,
                     const void * data,
                     std::size_t size);

    // for debugging:
    std::string nextFileTimestamp() const;

  private:
    WavFile(const WavFile &);
    WavFile & operator = (const WavFile &);

    // helper:
    bool open(const std::string & fn);

    FILE * file_;
    std::size_t nout_;
    std::size_t nstart_;
    unsigned int sampleRate_;
    unsigned int numChannels_;
    unsigned int bitsPerSample_;
    bool floatSamples_;
  };


  //----------------------------------------------------------------
  // WavFileReader
  //
  struct WavFileReader
  {
    enum { kPCM_integer = 1, kIEEE754_float = 3 };

    yae::Data wav_;
    yae::Bitstream bs_;

    // 1: PCM integer, 3: IEEE 754 float
    uint16_t audio_format_;

    uint16_t num_channels_;

    // in Hertz
    uint32_t sample_rate_;

    // Frequency * BytePerBloc)
    uint32_t bytes_per_sec_;

    // NbrChannels * BitsPerSample / 8
    uint16_t bytes_per_block_;

    // Number of bits per sample
    uint16_t bits_per_sample_;

    uint32_t sample_data_size_;
    std::size_t data_start_byte_pos_;

    WavFileReader(const std::string & fn = std::string());
    WavFileReader(const WavFileReader & other);

    WavFileReader & operator = (const WavFileReader & other);

    // helper:
    bool open(const std::string & fn);
    bool save(const std::string & fn) const;

    // returns number of samples loaded:
    int load_frame(yae::Data & data, int num_samples = 1536);

    void rewind();
    void seek_to(int64_t pts);

    int64_t get_pts() const;
    int64_t get_dur() const;
  };

}


#endif // YAE_WAV_FILE_H_
