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

    bool open(const std::string & fn, const AudioTraits & atts)
    {
      int nchan = atts.ch_layout_.nb_channels;
      int sampleRate = (unsigned int)(atts.sample_rate_);
      int bitsPerSample = atts.get_bytes_per_sample() * 8;
      bool floatSamples = atts.sample_format_ == AV_SAMPLE_FMT_FLT;
      return open(fn, nchan, sampleRate, bitsPerSample, floatSamples);
    }

    bool open(const std::string & fn,
              unsigned int nchan,
              unsigned int sampleRate,
              unsigned int bitsPerSample,
              bool floatSamples = false)
    {
      close();

      file_ = fopen_utf8(fn, "wb");
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
    bool open(const std::string & fn)
    {
      close();

      file_ = fopen_utf8(fn, "wb");
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



  //----------------------------------------------------------------
  // WavFileReader
  //
  struct WavFileReader
  {
    enum { kPCM_integer = 1, kIEEE754_float = 3 };

    yae::Data wav_;
    yae::Bitstream bs_;

    // 1: PCM integer, 3: IEEE 754 float
    uint16_t audio_format_ = 0;

    uint16_t num_channels_ = 0;

    // in Hertz
    uint32_t sample_rate_ = 0;

    // Frequency * BytePerBloc)
    uint32_t bytes_per_sec_ = 0;

    // NbrChannels * BitsPerSample / 8
    uint16_t bytes_per_block_ = 0;

    // Number of bits per sample
    uint16_t bits_per_sample_ = 0;

    uint32_t sample_data_size_ = 0;
    std::size_t data_start_byte_pos_ = 0;

    WavFileReader(const std::string & fn = std::string())
    {
      if (!fn.empty())
      {
        YAE_THROW_IF(!this->open(fn));
      }
    }

    WavFileReader(const WavFileReader & other)
    {
      this->operator=(other);
    }

    WavFileReader & operator = (const WavFileReader & other)
    {
      wav_ = other.wav_;
      audio_format_ = other.audio_format_;
      num_channels_ = other.num_channels_;
      sample_rate_ = other.sample_rate_;
      bytes_per_sec_ = other.bytes_per_sec_;
      bytes_per_block_ = other.bytes_per_block_;
      bits_per_sample_ = other.bits_per_sample_;
      sample_data_size_ = other.sample_data_size_;
      data_start_byte_pos_ = other.data_start_byte_pos_;
      bs_.reset(wav_);
      bs_.seek(other.bs_.position());
      return *this;
    }

    // helper:
    bool open(const std::string & fn)
    {
      if (!yae::load_file(wav_, fn, "rb"))
      {
        return false;
      }

      bs_.reset(wav_);

      // read the file header:
      if (!bs_.expect_fourcc("RIFF"))
      {
        return false;
      }

      // file size minus 8, little-endian:
      uint32_t file_size_minus_8 = 0;
      bs_.read_bytes((uint8_t *)&file_size_minus_8, 4);
      if (wav_.size() != file_size_minus_8 + 8)
      {
        return false;
      }

      if (!bs_.expect_fourcc("WAVE"))
      {
        return false;
      }

      // 'fmt ' sub-chunk:
      if (!bs_.expect_fourcc("fmt "))
      {
        return false;
      }

      // format structure size minus 8, little endian:
      uint32_t fmt_chunk_data_size = 0;
      bs_.read_bytes((uint8_t *)&fmt_chunk_data_size, 4);
      if (fmt_chunk_data_size != 16)
      {
        return false;
      }

      bs_.read_bytes((uint8_t *)&audio_format_, 2);
      if (audio_format_ != kPCM_integer &&
          audio_format_ != kIEEE754_float)
      {
        return false;
      }

      bs_.read_bytes((uint8_t *)&num_channels_, 2);
      if (num_channels_ < 1 || num_channels_ > 8)
      {
        return false;
      }

      // sample rate:
      bs_.read_bytes((uint8_t *)&sample_rate_, 4);

      // byte rate:
      bs_.read_bytes((uint8_t *)&bytes_per_sec_, 4);

      // block align (stride):
      bs_.read_bytes((uint8_t *)&bytes_per_block_, 2);

      // bits per sample:
      bs_.read_bytes((uint8_t *)&bits_per_sample_, 2);

      if (bytes_per_sec_ !=
          (bits_per_sample_ * num_channels_ * sample_rate_) / 8)
      {
        return false;
      }

      // skip to "data" chunk:
      uint8_t fourcc[5];
      fourcc[4] = 0;
      while (true)
      {
        if (!bs_.has_enough_bytes(4))
        {
          return false;
        }

        bs_.read_bytes(fourcc, 4);
        if (memcmp(fourcc, "data", 4) == 0)
        {
          break;
        }

        // skip this chunk:
        uint32_t payload_size = 0;
        bs_.read_bytes((uint8_t *)&payload_size, 4);
        if (!bs_.has_enough_bytes(payload_size))
        {
          return false;
        }

        bs_.skip_bytes(payload_size);
      }

      bs_.read_bytes((uint8_t *)&sample_data_size_, 4);
      if (!bs_.has_enough_bytes(sample_data_size_))
      {
        return false;
      }

      data_start_byte_pos_ = bs_.byte_pos();
      return true;
    }

    bool save(const std::string & fn) const
    {
      yae::TOpenFile file(fn, "wb");
      if (!file.is_open())
      {
        return false;
      }

      // data can be clipped via data_start_byte_pos_ and sample_data_size_:
      uint32_t sample_data_size = wav_.size() - data_start_byte_pos_;
      YAE_ASSERT(sample_data_size_ <= sample_data_size);
      if (sample_data_size < sample_data_size_)
      {
        return false;
      }

      file.write("RIFF", 4);

      // file size minus 8, little-endian:
      uint32_t file_size_minus_8 =
        yae::WavFile::kHeadSz +
        yae::WavFile::kFrmtSz +
        sample_data_size_;
      file.write(&file_size_minus_8, 4);

      file.write("WAVE", 4);
      file.write("fmt ", 4);

      // format structure size minus 8, little endian:
      uint32_t fmt_chunk_data_size = 16;
      file.write(&fmt_chunk_data_size, 4);
      file.write(&audio_format_, 2);
      file.write(&num_channels_, 2);
      file.write(&sample_rate_, 4);
      file.write(&bytes_per_sec_, 4);
      file.write(&bytes_per_block_, 2);
      file.write(&bits_per_sample_, 2);

      // write "data" chunk:
      file.write("data", 4);
      file.write(&sample_data_size_, 4);
      file.write(wav_.get() + data_start_byte_pos_, sample_data_size_);

      return true;
    }

    // returns number of samples loaded:
    int load_frame(yae::Data & data, int num_samples = 1536)
    {
      uint8_t bytes_per_sample = (bits_per_sample_ >> 3);
      YAE_THROW_IF(bits_per_sample_ != (bytes_per_sample << 3));

      uint32_t frame_size = num_samples * (num_channels_ * bytes_per_sample);
      frame_size = std::min<uint32_t>(frame_size, bs_.bytes_left());

      int out_samples = frame_size / (num_channels_ * bytes_per_sample);
      frame_size = out_samples * (num_channels_ * bytes_per_sample);
      data.resize(frame_size);

      if (frame_size > 0)
      {
        bs_.read_bytes(data.get(), frame_size);
      }

      return out_samples;
    }

    inline void rewind()
    {
      bs_.seek_to_byte_pos(data_start_byte_pos_);
    }

    inline int64_t get_pts() const
    {
      std::size_t pos = std::max(bs_.byte_pos(), data_start_byte_pos_);
      int64_t pts = (pos - data_start_byte_pos_) / bytes_per_block_;
      return pts;
    }

    inline int64_t get_dur() const
    {
      std::size_t data_size = wav_.size() - data_start_byte_pos_;
      int64_t dur = data_size / bytes_per_block_;
      return dur;
    }

    inline void seek_to(int64_t pts)
    {
      std::size_t pos = data_start_byte_pos_ + pts * bytes_per_block_;
      bs_.seek_to_byte_pos(pos);
    }
  };

}


#endif // YAE_WAV_FILE_H_
