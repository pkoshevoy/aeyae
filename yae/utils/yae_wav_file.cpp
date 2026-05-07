// -*- Mode: c++; tab-width: 8; c-basic-offset: 2; indent-tabs-mode: nil -*-
// NOTE: the first line of this file sets up source code indentation rules
// for Emacs; it is also a hint to anyone modifying this file.

// Created      : Sun Dec 29 15:13:25 MST 2013
// Copyright    : Pavel Koshevoy
// License      : MIT -- http://www.opensource.org/licenses/mit-license.php

// aeyae:
#include "yae/api/yae_api.h"
#include "yae/utils/yae_utils.h"
#include "yae/utils/yae_wav_file.h"
#include "yae/video/yae_video.h"

// standard:
#include <string>
#include <stdio.h>

// namespace shortcuts:
using yae::WavFile;
using yae::WavFileReader;
using yae::fopen_utf8;


//----------------------------------------------------------------
// WavFile::WavFile
//
WavFile::WavFile():
  nout_(0),
  nstart_(0),
  sampleRate_(0),
  numChannels_(0),
  bitsPerSample_(0),
  floatSamples_(false)
{}

//----------------------------------------------------------------
// WavFile::~WavFile
//
WavFile::~WavFile()
{
  close();
}

//----------------------------------------------------------------
// WavFile::close
//
void
WavFile::close()
{
  if (!file_.is_open())
  {
    return;
  }

  // re-write RIFF chunk size:
  int err = file_.fseek64(4, SEEK_SET);
  if (!err)
  {
    file_.save_32_le(uint32_t(kHeadSz + kFrmtSz + nout_));
  }

  // re-write data chunk size:
  err = file_.fseek64(40, SEEK_SET);
  if (!err)
  {
    file_.save_32_le(uint32_t(nout_));
  }

  file_.close();
  nstart_ += nout_;
  nout_ = 0;
}

//----------------------------------------------------------------
// WavFile::open
//
bool
WavFile::open(const std::string & fn, const yae::AudioTraits & atts)
{
  int nchan = atts.ch_layout_.nb_channels;
  int sampleRate = (unsigned int)(atts.sample_rate_);
  int bitsPerSample = atts.get_bytes_per_sample() * 8;
  bool floatSamples = atts.sample_format_ == AV_SAMPLE_FMT_FLT;
  return this->open(fn, nchan, sampleRate, bitsPerSample, floatSamples);
}

//----------------------------------------------------------------
// WavFile::open
//
bool
WavFile::open(const std::string & fn,
              unsigned int nchan,
              unsigned int sampleRate,
              unsigned int bitsPerSample,
              bool floatSamples)
{
  this->close();

  file_.open(fn, "wb");
  if (!file_.is_open())
  {
    return false;
  }

  numChannels_ = nchan;
  sampleRate_ = sampleRate;
  bitsPerSample_ = bitsPerSample;
  floatSamples_ = floatSamples;

  return this->open(fn);
}

//----------------------------------------------------------------
// WavFile::save
//
bool
WavFile::save(unsigned int numSamples,
              const void * samples,
              const char * newFilePrefix)
{
  std::size_t size = (numSamples * numChannels_ * bitsPerSample_) / 8;
  return this->save(samples, size, newFilePrefix);
}

//----------------------------------------------------------------
// WavFile::save
//
bool
WavFile::save(const void * data,
              std::size_t dataSize,
              const char * newFilePrefix)
{
  if (newFilePrefix)
  {
    this->close();

    std::ostringstream os;
    TTime t(nstart_, (bitsPerSample_ * numChannels_ * sampleRate_) / 8);

    os << newFilePrefix << t.to_hhmmss_us("", ".") << ".wav";
    this->open(os.str().c_str());
  }

  if (!file_.is_open())
  {
    return false;
  }

  yae::Data data_ne = yae::shallow_ref(const_cast<void *>(data), dataSize);
  yae::Data data_le =
    (floatSamples_ && bitsPerSample_ == 64) ? data_ne.ntol<double>() :
    (floatSamples_ && bitsPerSample_ == 32) ? data_ne.ntol<float>() :
    (bitsPerSample_ == 64) ? data_ne.ntol<uint64_t>() :
    (bitsPerSample_ == 32) ? data_ne.ntol<uint32_t>() :
    (bitsPerSample_ == 16) ? data_ne.ntol<uint16_t>() :
    data_ne;

  nout_ += dataSize;
  return file_.save(data_le);
}

//----------------------------------------------------------------
// WavFile::save
//
bool
WavFile::save(const char * fn,
              const yae::AudioTraits & atts,
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

//----------------------------------------------------------------
// WavFile::nextFileTimestamp
//
std::string
WavFile::nextFileTimestamp() const
{
  std::size_t start = nstart_ + nout_;
  TTime t(start, (bitsPerSample_ * numChannels_ * sampleRate_) / 8);
  std::string ts = t.to_hhmmss_us("", ".");
  return ts;
}

//----------------------------------------------------------------
// WavFile::open
//
bool
WavFile::open(const std::string & fn)
{
  this->close();

  file_.open(fn, "wb");
  if (!file_.is_open())
  {
    return false;
  }

  // 'RIFF' chunk:
  file_.write("RIFF", 4);
  file_.save_32_le<uint32_t>(kHeadSz + kFrmtSz);
  file_.write("WAVE", 4);

  // 'fmt ' sub-chunk:
  file_.write("fmt ", 4);

  // format structure size:
  file_.save_32_le<uint32_t>(16);

  // PCM == 1, IEEE FLOAT == 3:
  file_.save_16_le<uint16_t>(floatSamples_ ? 3 : 1);

  // number of channels:
  file_.save_16_le<uint16_t>(numChannels_);

  // sample rate:
  file_.save_32_le<uint32_t>(sampleRate_);

  // byte rate:
  file_.save_32_le<uint32_t>((bitsPerSample_ * numChannels_ * sampleRate_) / 8);

  // block align (stride):
  file_.save_16_le<uint16_t>((bitsPerSample_ * numChannels_) / 8);

  // bits per sample:
  file_.save_16_le<uint16_t>(bitsPerSample_);

  // 'data' sub-chunk:
  file_.write("data", 4);
  file_.save_32_le<uint32_t>(0);

  return true;
}


//----------------------------------------------------------------
// WavFileReader::WavFileReader
//
WavFileReader::WavFileReader(const std::string & fn):
  audio_format_(0),
  num_channels_(0),
  sample_rate_(0),
  bytes_per_sec_(0),
  bytes_per_block_(0),
  bits_per_sample_(0),
  sample_data_size_(0),
  data_start_byte_pos_(0)
{
  if (!fn.empty())
  {
    YAE_THROW_IF(!this->open(fn));
  }
}

//----------------------------------------------------------------
// WavFileReader::WavFileReader
//
WavFileReader::WavFileReader(const WavFileReader & other)
{
  this->operator=(other);
}

//----------------------------------------------------------------
// WavFileReader::operator =
//
WavFileReader &
 WavFileReader::operator = (const WavFileReader & other)
{
  data_ne_ = other.data_ne_;
  audio_format_ = other.audio_format_;
  num_channels_ = other.num_channels_;
  sample_rate_ = other.sample_rate_;
  bytes_per_sec_ = other.bytes_per_sec_;
  bytes_per_block_ = other.bytes_per_block_;
  bits_per_sample_ = other.bits_per_sample_;
  sample_data_size_ = other.sample_data_size_;
  data_start_byte_pos_ = other.data_start_byte_pos_;
  bs_.reset(data_ne_);
  bs_.seek(other.bs_.position());
  return *this;
}

//----------------------------------------------------------------
// WavFileReader::open
//
bool
WavFileReader::open(const std::string & fn)
{
  yae::Data wav;
  if (!yae::load_file(wav, fn, "rb"))
  {
    return false;
  }

  bs_.reset(wav);

  // read the file header:
  if (!bs_.expect_fourcc("RIFF"))
  {
    return false;
  }

  // file size minus 8, little-endian:
  uint32_t file_size_minus_8 = yae::bswap_32(bs_.read<uint32_t>());
  if (wav.size() != file_size_minus_8 + 8)
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
  uint32_t fmt_chunk_data_size = yae::bswap_32(bs_.read<uint32_t>());
  if (fmt_chunk_data_size != 16)
  {
    return false;
  }

  audio_format_ = yae::bswap_16(bs_.read<uint16_t>());
  if (audio_format_ != kPCM_integer &&
      audio_format_ != kIEEE754_float)
  {
    return false;
  }

  num_channels_ = yae::bswap_16(bs_.read<uint16_t>());
  if (num_channels_ < 1 || num_channels_ > 8)
  {
    return false;
  }

  // sample rate:
  sample_rate_ = yae::bswap_32(bs_.read<uint32_t>());

  // byte rate:
  bytes_per_sec_ = yae::bswap_32(bs_.read<uint32_t>());

  // block align (stride):
  bytes_per_block_ = yae::bswap_16(bs_.read<uint16_t>());

  // bits per sample:
  bits_per_sample_ = yae::bswap_16(bs_.read<uint16_t>());

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
    uint32_t payload_size = yae::bswap_32(bs_.read<uint32_t>());
    if (!bs_.has_enough_bytes(payload_size))
    {
      return false;
    }

    bs_.skip_bytes(payload_size);
  }

  sample_data_size_ = yae::bswap_32(bs_.read<uint32_t>());
  if (!bs_.has_enough_bytes(sample_data_size_))
  {
    return false;
  }

  std::size_t data_pos = bs_.byte_pos();
  uint8_t * sample_data = wav.get() + data_pos;
  yae::Data data_le = yae::shallow_ref(sample_data, sample_data_size_);
  bool float_samples = (audio_format_ == kIEEE754_float);

  data_ne_ =
    (float_samples && bits_per_sample_ == 64) ? data_le.ntol<double>() :
    (float_samples && bits_per_sample_ == 32) ? data_le.ntol<float>() :
    (bits_per_sample_ == 64) ? data_le.ntol<uint64_t>() :
    (bits_per_sample_ == 32) ? data_le.ntol<uint32_t>() :
    (bits_per_sample_ == 16) ? data_le.ntol<uint16_t>() :
    data_le;

  data_start_byte_pos_ = 0;
  bs_.reset(data_ne_);

  return true;
}

//----------------------------------------------------------------
// WavFileReader::save
//
bool
WavFileReader::save(const std::string & fn) const
{
  yae::TOpenFile file(fn, "wb");
  if (!file.is_open())
  {
    return false;
  }

  // data can be clipped via data_start_byte_pos_ and sample_data_size_:
  uint32_t sample_data_size = data_ne_.size() - data_start_byte_pos_;
  YAE_ASSERT(sample_data_size_ <= sample_data_size);
  if (sample_data_size < sample_data_size_)
  {
    return false;
  }

  file.write("RIFF", 4);

  // file size minus 8, little-endian:
  file.save_32_le<uint32_t>(yae::WavFile::kHeadSz +
                            yae::WavFile::kFrmtSz +
                            sample_data_size_);

  file.write("WAVE", 4);
  file.write("fmt ", 4);

  // format structure size minus 8, little endian:
  file.save_32_le<uint32_t>(16); // fmt chunk data size
  file.save_16_le<uint16_t>(audio_format_);
  file.save_16_le<uint16_t>(num_channels_);
  file.save_32_le<uint32_t>(sample_rate_);
  file.save_32_le<uint32_t>(bytes_per_sec_);
  file.save_16_le<uint16_t>(bytes_per_block_);
  file.save_16_le<uint16_t>(bits_per_sample_);

  // write "data" chunk:
  file.write("data", 4);
  file.save_32_le<uint32_t>(sample_data_size_);

  bool float_samples = (audio_format_ == kIEEE754_float);
  uint8_t * sample_data = data_ne_.get() + data_start_byte_pos_;
  yae::Data data_ne = yae::shallow_ref(sample_data, sample_data_size_);
  yae::Data data_le =
    (float_samples && bits_per_sample_ == 64) ? data_ne.ntol<double>() :
    (float_samples && bits_per_sample_ == 32) ? data_ne.ntol<float>() :
    (bits_per_sample_ == 64) ? data_ne.ntol<uint64_t>() :
    (bits_per_sample_ == 32) ? data_ne.ntol<uint32_t>() :
    (bits_per_sample_ == 16) ? data_ne.ntol<uint16_t>() :
    data_ne;

  return file.save(data_le);
}

//----------------------------------------------------------------
// WavFileReader::load_frame
//
int
WavFileReader::load_frame(yae::Data & data, int num_samples)
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

//----------------------------------------------------------------
// WavFileReader::rewind
//
void
WavFileReader::rewind()
{
  bs_.seek_to_byte_pos(data_start_byte_pos_);
}

//----------------------------------------------------------------
// WavFileReader::seek_to
//
void
WavFileReader::seek_to(int64_t pts)
{
  std::size_t pos = data_start_byte_pos_ + pts * bytes_per_block_;
  bs_.seek_to_byte_pos(pos);
}

//----------------------------------------------------------------
// WavFileReader::get_pts
//
int64_t
WavFileReader::get_pts() const
{
  std::size_t pos = std::max(bs_.byte_pos(), data_start_byte_pos_);
  int64_t pts = (pos - data_start_byte_pos_) / bytes_per_block_;
  return pts;
}

//----------------------------------------------------------------
// WavFileReader::get_dur
//
int64_t
WavFileReader::get_dur() const
{
  std::size_t data_size = data_ne_.size() - data_start_byte_pos_;
  int64_t dur = data_size / bytes_per_block_;
  return dur;
}
