// -*- Mode: c++; tab-width: 8; c-basic-offset: 2; indent-tabs-mode: nil -*-
// NOTE: the first line of this file sets up source code indentation rules
// for Emacs; it is also a hint to anyone modifying this file.

// Created   : Thu Apr 22 21:44:28 MDT 2026
// Copyright : Pavel Koshevoy
// License   : MIT -- http://www.opensource.org/licenses/mit-license.php

// aeyae:
#include "yae/ffmpeg/yae_ffmpeg_rdft.h"
#include "yae/utils/yae_utils.h"
#include "yae/video/yae_audio_utils.h"

// ffmpeg:
extern "C" {
#include <libavutil/samplefmt.h>
}

// standard:
#include <cmath>


//----------------------------------------------------------------
// two_pi
//
static const double two_pi = M_PI * 2.0;


//----------------------------------------------------------------
// yae::generate_stereo_f32
//
yae::WavFileReader
yae::generate_stereo_f32(std::size_t num_samples,
                         int sample_rate,
                         float ch0_tone_hz,
                         float ch1_tone_hz,
                         float ch0_modulate_hz,
                         float ch1_modulate_hz)
{
  yae::WavFileReader wav;
  wav.audio_format_ = yae::WavFileReader::kIEEE754_float;
  wav.num_channels_ = 2;
  wav.sample_rate_ = sample_rate;
  wav.bits_per_sample_ = 32;
  wav.bytes_per_block_ = (wav.bits_per_sample_ >> 3) * wav.num_channels_;
  wav.bytes_per_sec_ = wav.bytes_per_block_ * wav.sample_rate_;
  wav.sample_data_size_ = wav.bytes_per_block_ * num_samples;
  wav.data_start_byte_pos_ = 0;
  wav.wav_.alloc(wav.sample_data_size_);

  float * ch0 = wav.wav_.get<float>();
  float * ch1 = ch0 + 1;

  for (uint64_t i = 0; i < num_samples; ++i, ch0 += 2, ch1 += 2)
  {
    float t = float(i) / float(sample_rate); // time in seconds

    float s0 =
      (ch0_modulate_hz <= 0.f) ? 1.f :
      ::sinf(two_pi * t * ch0_modulate_hz);

    float s1 =
      (ch1_modulate_hz <= 0.f) ? 1.f :
      ::sinf(two_pi * t * ch1_modulate_hz);

    *ch0 = ::sinf(two_pi * t * ch0_tone_hz) * s0;
    *ch1 = ::sinf(two_pi * t * ch1_tone_hz) * s1;
  }

  return wav;
}

//----------------------------------------------------------------
// to_s16
//
yae::TAudioFrame
yae::to_s16(const yae::TAudioFrame & src)
{
  if (src.get_format() == AV_SAMPLE_FMT_S16)
  {
    return src;
  }

  yae::TAudioFrame out = src;
  if (src.get_format() == AV_SAMPLE_FMT_FLT)
  {
    yae::Data s16 = yae::f32_to_s16(src.get_data(), 32767.f);
    out.traits_.sample_format_ = AV_SAMPLE_FMT_S16;
    out.set_data(s16);
  }

  YAE_THROW_IF(out.get_format() != AV_SAMPLE_FMT_S16);
  return out;
}

//----------------------------------------------------------------
// yae::s16_to_max_amp
//
yae::Data
yae::s16_to_max_amp(const yae::TAudioFrame & frame)
{
  YAE_THROW_IF(frame.get_format() != AV_SAMPLE_FMT_S16);
  return yae::s16_to_max_amp(frame.data_->get<int16_t>(),
                             frame.data_->end<int16_t>(),
                             frame.num_channels());
}

//----------------------------------------------------------------
// yae::s16_to_max_amp
//
yae::Data
yae::s16_to_max_amp(const yae::Data & src, int num_channels)
{
  return yae::s16_to_max_amp(src.get<int16_t>(),
                             src.end<int16_t>(),
                             num_channels);
}

//----------------------------------------------------------------
// yae::s16_to_max_amp
//
yae::Data
yae::s16_to_max_amp(const int16_t * src,
                    const int16_t * end,
                    int num_channels)
{
  int num_samples = (end - src) / num_channels;

  yae::Data output;
  int16_t * dst = output.resize<int16_t>(num_samples);
  output.memset(0);

  while (src < end)
  {
    // find max amplitude across all channels:
    int amp = abs(src[0]);

    for (int i = 1; i < num_channels; ++i)
    {
      amp = std::max<int>(amp, src[i]);
    }

    *dst = std::min<int>(amp, std::numeric_limits<int16_t>::max());
    dst += 1;
    src += num_channels;
  }

  return output;
}

//----------------------------------------------------------------
// yae::s16_to_mono
//
yae::Data
yae::s16_to_mono(const yae::TAudioFrame & frame)
{
  YAE_THROW_IF(frame.get_format() != AV_SAMPLE_FMT_S16);
  return yae::s16_to_mono(frame.data_->get<int16_t>(),
                          frame.data_->end<int16_t>(),
                          frame.num_channels());
}

//----------------------------------------------------------------
// yae::s16_to_mono
//
yae::Data
yae::s16_to_mono(const yae::Data & src, int num_channels)
{
  return yae::s16_to_mono(src.get<int16_t>(),
                          src.end<int16_t>(),
                          num_channels);
}

//----------------------------------------------------------------
// yae::s16_to_mono
//
yae::Data
yae::s16_to_mono(const int16_t * src,
                 const int16_t * end,
                 int num_channels)
{
  int num_samples = (end - src) / num_channels;

  yae::Data output;
  int16_t * dst = output.resize<int16_t>(num_samples);
  output.memset(0);

  while (src < end)
  {
    int sum = 0;

    for (int i = 0; i < num_channels; ++i)
    {
      sum += src[i];
    }

    *dst = int16_t(sum / num_channels);
    dst += 1;
    src += num_channels;
  }

  return output;
}

//----------------------------------------------------------------
// s16_mono_downsample
//
yae::Data
yae::s16_mono_downsample(const yae::Data & input)
{
  const int16_t * src = input.get<int16_t>();
  int num_samples = input.num<int16_t>();
  int out_samples = num_samples / 2;

  yae::Data output;
  int16_t * dst = output.resize<int16_t>(out_samples);
  int16_t * end = output.end<int16_t>();
  output.memset(0);

  while (dst < end)
  {
    int sum = (src[0] + src[1]);
    *dst = int16_t(sum >> 1);
    dst += 1;
    src += 2;
  }

  return output;
}

//----------------------------------------------------------------
// yae::s16_to_f32
//
yae::Data
yae::s16_to_f32(const yae::Data & input, int out_samples)
{
  int num_samples = input.num<int16_t>();

  if (out_samples <= 0)
  {
    out_samples = num_samples;
  }

  num_samples = std::min<int>(num_samples, out_samples);

  yae::Data output;
  float * dst = output.resize<float>(out_samples);
  output.memset(0);

  const int16_t * src = input.get<int16_t>();
  const int16_t * end = input.end<int16_t>();
  end = std::min(end, src + num_samples);

  for (; src < end; ++src, ++dst)
  {
    // transform into [-1, 1] range:
    *dst = (float(int(*src) - std::numeric_limits<int16_t>::min()) /
            float(std::numeric_limits<uint16_t>::max()) * 2.f - 1.f);
  }

  return output;
}

//----------------------------------------------------------------
// yae::f32_to_s16
//
yae::Data
yae::f32_to_s16(const yae::Data & input, float scale, int out_samples)
{
  int num_samples = input.num<float>();

  if (out_samples <= 0)
  {
    out_samples = num_samples;
  }

  num_samples = std::min<int>(num_samples, out_samples);

  yae::Data output;
  int16_t * dst = output.resize<int16_t>(out_samples);
  output.memset(0);

  const float * src = input.get<float>();
  const float * end = input.end<float>();
  end = std::min(end, src + num_samples);

  for (; src < end; ++src, ++dst)
  {
    float f = std::min(32767.f, std::max(-32768.f, *src * scale));
    *dst = int16_t(f);
  }

  return output;
}

//----------------------------------------------------------------
// yae::draw_wav_s16_mono
//
yae::AvFrm
yae::draw_wav_s16_mono(const yae::Data & s16_mono)
{
  int num_samples = s16_mono.num<int16_t>();

  yae::AvFrm frame;
  frame.alloc_video_buffers(AV_PIX_FMT_GRAY8, 256, num_samples);

  AVFrame & frm = frame.get();
  memset(frm.data[0], 0x00, frm.height * frm.linesize[0]);

  for (int i = 0; i < num_samples; ++i)
  {
    uint8_t * row = frm.data[0] + i * frm.linesize[0];
    int16_t v = s16_mono.get<int16_t>(i);
    int16_t w = v >> 8;

    if (w < 0)
    {
      memset(row + 128 + w, 0xFF, -w);
    }
    else
    {
      memset(row + 128, 0xFF, w);
    }
  }

  return frame;
}

//----------------------------------------------------------------
// yae::draw_wav_amp
//
yae::AvFrm
yae::draw_wav_amp(const yae::TAudioFrame & input)
{
  YAE_THROW_IF(input.get_format() != AV_SAMPLE_FMT_S16);
  return yae::draw_wav_amp(input.get_data(), input.num_channels());
}

//----------------------------------------------------------------
// yae::draw_wav_amp
//
yae::AvFrm
yae::draw_wav_amp(yae::Data s16_samples, int num_channels)
{
  yae::Data s16_mono = yae::s16_to_max_amp(s16_samples, num_channels);
  std::size_t num_samples = s16_mono.num<int16_t>();

  while (num_samples > 512)
  {
    s16_mono = yae::s16_mono_downsample(s16_mono);
    num_samples /= 2;
  }

  yae::AvFrm frame;
  frame.alloc_video_buffers(AV_PIX_FMT_GRAY8, 256, num_samples);

  AVFrame & frm = frame.get();
  memset(frm.data[0], 0x00, frm.height * frm.linesize[0]);

  for (std::size_t i = 0; i < num_samples; ++i)
  {
    uint8_t * row = frm.data[0] + i * frm.linesize[0];
    int16_t v = s16_mono.get<int16_t>(i);
    int16_t w = v >> 8;
    memset(row + 128 - w, 0xFF, w * 2);
  }

  return frame;
}

//----------------------------------------------------------------
// rgb24_add
//
inline static void
rgb24_add(uint8_t * row, uint8_t * end, uint32_t rgb)
{
  uint8_t r = (rgb >> 16) & 0xFF;
  uint8_t g = (rgb >> 8) & 0xFF;
  uint8_t b = rgb & 0xFF;

  for (uint8_t * out = row; out < end; out += 3)
  {
    out[0] += r;
    out[1] += g;
    out[2] += b;
  }
}

//----------------------------------------------------------------
// rgb24_add
//
inline static void
rgb24_add(uint8_t * row, std::size_t count, uint32_t rgb)
{
  uint8_t * end = row + count * 3;
  rgb24_add(row, end, rgb);
}

//----------------------------------------------------------------
// yae::draw_wav_overlap
//
yae::AvFrm
yae::draw_wav_overlap(const yae::TAudioFrame & frame_a,
                      const yae::TAudioFrame & frame_b,
                      int offset)
{
  using yae::to_s16;
  using yae::s16_to_max_amp;
  yae::Data s16_mono_a = s16_to_max_amp(to_s16(frame_a));
  yae::Data s16_mono_b = s16_to_max_amp(to_s16(frame_b));
  return yae::draw_wav_overlap(s16_mono_a, s16_mono_b, offset);
}

//----------------------------------------------------------------
// yae::draw_wav_overlap
//
yae::AvFrm
yae::draw_wav_overlap(yae::Data s16_mono_a,
                      yae::Data s16_mono_b,
                      int offset)
{
  int num_samples = s16_mono_a.num<int16_t>();

  while (num_samples > 512)
  {
    s16_mono_a = yae::s16_mono_downsample(s16_mono_a);
    s16_mono_b = yae::s16_mono_downsample(s16_mono_b);
    num_samples /= 2;
    offset /= 2;
  }

  yae::AvFrm frame;
  frame.alloc_video_buffers(AV_PIX_FMT_RGB24, 256, num_samples);

  AVFrame & frm = frame.get();
  memset(frm.data[0], 0x00, frm.height * frm.linesize[0]);

  for (int i = 0; i < num_samples; ++i)
  {
    uint8_t * row = frm.data[0] + i * frm.linesize[0];
    int16_t v = s16_mono_a.get<int16_t>(i);
    int16_t w = v >> 8;
    rgb24_add(row + (128 - w) * 3, w * 2, 0xFF7F00);

    int j = i + offset;
    if (j < 0 || num_samples <= j)
    {
      continue;
    }

    v = s16_mono_b.get<int16_t>(j);
    w = v >> 8;
    rgb24_add(row + (128 - w) * 3, w * 2, 0x0080FF);
  }

  return frame;
}

//----------------------------------------------------------------
// SlidingAverage
//
template <typename TData, std::size_t Size>
struct SlidingAverage
{
  enum { kSize = Size };
  std::size_t size_;
  std::size_t tail_;
  TData data_[Size];
  TData sum_;

  SlidingAverage():
    size_(0),
    tail_(0),
    sum_(0)
  {}

  void push(TData v)
  {
    sum_ += v;

    if (size_ < Size)
    {
      data_[tail_] = v;
      size_ += 1;
    }
    else
    {
      sum_ -= data_[tail_];
      data_[tail_] = v;
    }

    tail_ = (tail_ + 1) % Size;
  }

  inline double avg() const
  { return double(sum_) / double(size_); }
};

//----------------------------------------------------------------
// yae::find_alignment_offset
//
int
yae::find_alignment_offset(const yae::TAudioFrame & a,
                           const yae::TAudioFrame & b,
                           double & best_err)
{
  typedef yae::rDFT::re_t re_t;
  typedef yae::rDFT::cx_t cx_t;

  using yae::to_s16;
  using yae::s16_to_mono;
  using yae::s16_to_f32;
  using yae::f32_to_s16;
  using yae::draw_wav_s16_mono;
  using yae::save_as_png;

  int num_samples = a.num_samples();
  int window = yae::get_po2_size(num_samples);
  int half_window = window / 2;
  int window_x2 = window * 2;

  yae::rDFT rdft;
  rdft.init(window_x2);
  // rdft.init(window);

  // convert frame data to AV_SAMPLE_FMT_FLT, mono:
  yae::rDFT::Frame frag_a;
  frag_a.init(rdft, s16_to_f32(s16_to_mono(to_s16(a)), rdft.po2_size()));

  yae::rDFT::Frame frag_b;
  frag_b.init(rdft, s16_to_f32(s16_to_mono(to_s16(b)), rdft.po2_size()));

  // shortcuts:
  // YAE_ASSERT(rdft.re_buffer().num<re_t>() == window * 2);
  // YAE_ASSERT(rdft.re_buffer().num<re_t>() == window);
  re_t * correlation = rdft.re_buffer().get<re_t>();

  // calculate cross correlation in frequency domain:
  {
    const cx_t * xa = frag_a.cx_.get<cx_t>();
    const cx_t * xb = frag_b.cx_.get<cx_t>();

    rdft.cx_buffer().memset(0);
    // YAE_ASSERT(rdft.cx_buffer().num<cx_t>() == window + 1);
    // YAE_ASSERT(rdft.cx_buffer().num<cx_t>() == half_window + 1);
    cx_t * xc = rdft.cx_buffer().get<cx_t>();

    // for (uint32_t i = 0; i <= window; i++, xa++, xb++, xc++)
    for (uint32_t i = 0; i <= half_window; i++, xa++, xb++, xc++)
    {
      xc->re = (xa->re * xb->re + xa->im * xb->im);
      xc->im = (xa->im * xb->re - xa->re * xb->im);
    }

    // apply inverse rDFT transform:
    xc = rdft.cx_buffer().get<cx_t>();
    rdft.c2r(xc, correlation);
  }

  // rescale the data:
  re_t peak = 0.f;
  {
    re_t * xc = correlation;
    for (int i = 0; i < window; ++i, ++xc)
    {
      int overlap = window - i;
#if 0
      re_t s = 1.f;
#elif 1
      re_t s = re_t(window) / re_t(overlap);
#elif 0
      re_t s = re_t(2 * window - overlap) / re_t(window + overlap);
#else
      re_t s = (i + 1) * (window_x2 - i);
#endif
      re_t & metric = *xc;
      metric *= s;
      peak = std::max(peak, ::fabsf(metric));
    }
  }

  // run a box filter over thresholded data:
  yae::Data average;
  {
    re_t * avg = average.resize<re_t>(window);
    re_t * xc = correlation;

    typedef SlidingAverage<re_t, 3> TSlidingWindow;
    std::size_t n = TSlidingWindow::kSize;
    std::size_t n2 = n / 2;
    TSlidingWindow box;
    for (std::size_t i = 0; i < n; ++i)
    {
      std::size_t j = (i <  n2) ? (n2 - i) : (i - n2);
      box.push(xc[j]);
    }

    std::size_t n2_1 = n2 + 1;
    for (std::size_t i = 0; i < window; ++i, ++avg)
    {
      *avg = box.avg();
      std::size_t j = (i + n2_1 < window) ? (i + n2_1) : (window + n2 - i);
      box.push(xc[j]);
    }
  }

  // subtract the box filtered data to isolate the peaks:
  yae::Data diff;
  {
    re_t * xc = correlation;
    re_t * avg = average.get<re_t>();
    re_t * out = diff.resize<re_t>(window);
    re_t * end = diff.end<re_t>();
    re_t threshold = peak * 0.95;
    re_t max = 0.f;

    for (; out < end; ++xc, ++avg, ++out)
    {
      re_t amp = ::fabsf(*xc);
      *out = (amp < threshold) ? 0.f : (*xc - *avg);
      max = std::max(max, ::fabsf(*out));
    }
#if 0
    yae::Data s16_mono = f32_to_s16(diff, 32767.f / max);
    yae::AvFrm frm = draw_wav_s16_mono(s16_mono);
    save_as_png(frm, "/tmp/peaks-", a.duration());
#endif
  }

  // find offset evaluation candidates:
  std::list<int> candidates;
  {
    re_t * src = diff.get<re_t>();
    double num = 0.0;
    double den = 0.0;
    for (std::size_t i = 0; i < window; ++i, ++src)
    {
      re_t v = *src;
      if (den && !v)
      {
        double ix = num / den;
        candidates.push_back(int(ix + 0.5));
        num = 0;
        den = 0;
      }

      num += i * v;
      den += v;
    }

    if (den)
    {
      double ix = num / den;
      candidates.push_back(int(ix + 0.5));
    }
  }

  // find the best offset:
  best_err = std::numeric_limits<double>::max();
  int best_offset = 0;
  {
    re_t * xc = correlation;
    for (std::list<int>::const_iterator iter = candidates.begin();
         iter != candidates.end() && best_err > 0; ++iter)
    {
      int i = *iter;
      re_t metric = xc[i];
      int offset = (metric < 0) ? -i : i;

      re_t * src_a = frag_a.re_.get<re_t>();
      re_t * src_b = frag_b.re_.get<re_t>();

      re_t * end_a = src_a + window;
      re_t * end_b = src_b + window;
#if 0
      yae::AvFrm frm_ab = yae::draw_wav_overlap(b, a, offset);
      save_as_png(frm_ab, "/tmp/overlap-", a.duration());
#endif
      if (offset < 0)
      {
        src_b -= offset;
      }
      else
      {
        src_a += offset;
      }

      re_t err_sum = 0.0;
      std::size_t err_num = 0;
      for (; src_a < end_a && src_b < end_b; ++src_a, ++src_b)
      {
        err_sum += ::fabsf(*src_a - *src_b);
        err_num += 1;
      }

      re_t err = err_sum / re_t(err_num);
      if (err < best_err)
      {
        best_err = err;
        best_offset = offset;
      }
    }
  }

#if 0
  // debugging only:
  {
    yae::AvFrm frm_a = yae::draw_wav_amp(to_s16(a));
    save_as_png(frm_a,
                yae::strfmt("/tmp/amp-a-%08" PRIi64 "-", a.time_.time_),
                a.duration());

    yae::AvFrm frm_b = yae::draw_wav_amp(to_s16(b));
    save_as_png(frm_b,
                yae::strfmt("/tmp/amp-b-%08" PRIi64 "-", b.time_.time_),
                b.duration());

    yae::Data s16_mono = f32_to_s16(rdft.re_buffer(), 32767.f / peak , window);
    yae::AvFrm frm = draw_wav_s16_mono(s16_mono);
    save_as_png(frm,
                yae::strfmt("/tmp/xcor-%08" PRIi64 "-offset-%i-",
                            a.time_.time_,
                            best_offset),
                a.duration());
  }
#elif 0
    yae::AvFrm frm_a = yae::draw_wav_amp(to_s16(a));
    save_as_png(frm_a, "/tmp/amp-a-", a.duration());

    yae::AvFrm frm_b = yae::draw_wav_amp(to_s16(b));
    save_as_png(frm_b, "/tmp/amp-b-", b.duration());

    yae::Data s16_mono = f32_to_s16(rdft.re_buffer(), 32767.f / peak , window);
    yae::AvFrm frm = draw_wav_s16_mono(s16_mono);
    save_as_png(frm, "/tmp/xcor-", a.duration());

    yae::AvFrm frm_ab = yae::draw_wav_overlap(b, a, best_offset);
    save_as_png(frm_ab, "/tmp/overlap-", a.duration());
#endif

  return best_offset;
}

//----------------------------------------------------------------
// yae::get_traits
//
void
yae::get_traits(const yae::WavFileReader & wav,
                yae::AudioTraits & traits)
{
  traits.ch_layout_.set_default_layout(wav.num_channels_);
  traits.sample_rate_ = wav.sample_rate_;
  traits.sample_format_ =
    (wav.audio_format_ == yae::WavFileReader::kPCM_integer) ?
    ((wav.bits_per_sample_ == 8) ? AV_SAMPLE_FMT_U8 :
     (wav.bits_per_sample_ == 16) ? AV_SAMPLE_FMT_S16 :
     (wav.bits_per_sample_ == 32) ? AV_SAMPLE_FMT_S32 :
     AV_SAMPLE_FMT_NONE) :
    (wav.audio_format_ == yae::WavFileReader::kIEEE754_float) ?
    ((wav.bits_per_sample_ == 32) ? AV_SAMPLE_FMT_FLT :
     (wav.bits_per_sample_ == 64) ? AV_SAMPLE_FMT_DBL :
     AV_SAMPLE_FMT_NONE) :
    AV_SAMPLE_FMT_NONE;
}

//----------------------------------------------------------------
// yae::load
//
int
yae::load(yae::WavFileReader & wav, yae::TAudioFrame & frame, int max_samples)
{
  yae::get_traits(wav, frame.traits_);
  int64_t pts = wav.get_pts();

  yae::Data data;
  int num_samples = wav.load_frame(data, max_samples);

  yae::TPlanarBufferPtr buffer_ptr(new yae::TPlanarBuffer(1));
  frame.data_ = buffer_ptr;

  yae::TPlanarBuffer & buffer = *buffer_ptr;
  buffer.resize(data.size());
  memcpy(buffer.data(0), data.get(), data.size());
  frame.time_.reset(pts, wav.sample_rate_);

  return num_samples;
}

//----------------------------------------------------------------
// yae::calc_avg_abs_diff
//
double
yae::calc_avg_abs_diff(yae::WavFileReader wav_a,
                       yae::WavFileReader wav_b,
                       std::size_t frame_size,
                       int64_t offset)
{
  wav_a.rewind();
  wav_b.rewind();

  if (offset > 0)
  {
    wav_b.seek_to(offset);
  }
  else
  {
    wav_a.seek_to(-offset);
  }

  yae::TAudioFrame frame_a;
  yae::TAudioFrame frame_b;

  yae::load(wav_a, frame_a, frame_size);
  yae::load(wav_b, frame_b, frame_size);

  // sum the absolute differences
  uint64_t sum_absdiffs = 0;
  uint64_t num_absdiffs = 0;

  while (frame_a.num_samples() == frame_b.num_samples() &&
         frame_a.num_samples() > 0)
  {
    frame_a = to_s16(frame_a);
    frame_b = to_s16(frame_b);

#if 0
    yae::AvFrm wav_overlap = draw_wav_overlap(frame_a, frame_b, 0);
    save_as_png(wav_overlap,
                yae::strfmt("/tmp/overlap-%08" PRIi64 "-aligned-%i-",
                            frame_a.time_.time_,
                            offset),
                frame_a.duration());
#endif
    yae::Data src_amp = s16_to_max_amp(frame_a);
    yae::Data out_amp = s16_to_max_amp(frame_b);

    // downsample:
    while (src_amp.num<int16_t>() > 512)
    {
      src_amp = s16_mono_downsample(src_amp);
      out_amp = s16_mono_downsample(out_amp);
    }

    const int16_t * out = out_amp.get<int16_t>();
    const int16_t * src = src_amp.get<int16_t>();
    const int16_t * end = src_amp.end<int16_t>();

    for (; src < end; ++src, ++out)
    {
      int absdiff = ::abs(*src - *out);
      sum_absdiffs += absdiff;
      num_absdiffs += 1;
    }

    // load next frame:
    yae::load(wav_a, frame_a, frame_size);
    yae::load(wav_b, frame_b, frame_size);
  }

  double avg_absdiff = double(sum_absdiffs) / double(num_absdiffs);
  return avg_absdiff;
}

//----------------------------------------------------------------
// find_alignment_offset
//
// return best alignment offset between two wav files.
//
// the returned offset specifies to the number of samples that
// must be removed from the start of wav_b to align it with wav_a.
//
// NOTE: this can fail if the misalignment between waveforms is larger
// than the frame size (or even half the frame size).
// initial_offset is used to nudge the alignment closer.
//
static int64_t
find_alignment_offset(yae::WavFileReader wav_a,
                      yae::WavFileReader wav_b,
                      std::size_t frame_size_po2,
                      int64_t initial_offset)
{
  using yae::draw_wav_overlap;
  using yae::find_alignment_offset;

  frame_size_po2 = yae::get_po2_size(frame_size_po2);

  wav_a.rewind();
  wav_b.rewind();

  if (initial_offset > 0)
  {
    wav_b.seek_to(initial_offset);
  }
  else
  {
    wav_a.seek_to(-initial_offset);
  }

  yae::TAudioFrame frame_a;
  yae::TAudioFrame frame_b;

  yae::load(wav_a, frame_a, frame_size_po2);
  yae::load(wav_b, frame_b, frame_size_po2);

  // find alignment for each frame pair:
  std::vector<int> offsets;
  while (frame_a.num_samples() == frame_b.num_samples() &&
         frame_a.num_samples() > 0)
  {
    double abs_diff = std::numeric_limits<double>::max();
    int offset = find_alignment_offset(frame_b, frame_a, abs_diff);
    offsets.push_back(offset);
#if 0
    yae_dlog("alignment offset: %i", offset);
    yae::AvFrm wav_overlap = draw_wav_overlap(frame_a, frame_b, offset);
    save_as_png(wav_overlap,
                yae::strfmt("/tmp/overlap-%08" PRIi64 "-offset-%i-",
                            frame_a.time_.time_,
                            offset),
                frame_a.duration());
#endif

    // load next frame:
    yae::load(wav_a, frame_a, frame_size_po2);
    yae::load(wav_b, frame_b, frame_size_po2);
  }

  // select the median offset:
  std::sort(offsets.begin(), offsets.end());
  int offset = offsets[offsets.size() / 2];
  return initial_offset + offset;
}

//----------------------------------------------------------------
// yae::find_alignment_offset
//
int64_t
yae::find_alignment_offset(yae::WavFileReader wav_a,
                           yae::WavFileReader wav_b,
                           std::size_t frame_size_po2,
                           double & best_avg_abs_diff,
                           double avg_diff_threshold)
{
  using yae::calc_avg_abs_diff;
  using yae::draw_wav_overlap;
  using yae::save_as_png;

  frame_size_po2 = yae::get_po2_size(frame_size_po2);
  best_avg_abs_diff = std::numeric_limits<double>::max();

  int64_t best_offset = 0;
  int64_t max_dur = std::min(wav_a.get_dur(), wav_b.get_dur());

  // for (int64_t i = frame_size_po2 * 5; i < max_dur; i += frame_size_po2)
  for (int64_t i = 0; i < max_dur; i += frame_size_po2)
  {
    int64_t offset = ::find_alignment_offset(wav_a, wav_b, frame_size_po2, i);
    double avg_diff = calc_avg_abs_diff(wav_a, wav_b, frame_size_po2, offset);

    if (avg_diff < best_avg_abs_diff)
    {
      best_offset = offset;
      best_avg_abs_diff = avg_diff;
    }

    if (avg_diff <= avg_diff_threshold)
    {
      break;
    }

    offset = -::find_alignment_offset(wav_b, wav_a, frame_size_po2, i);
    avg_diff = calc_avg_abs_diff(wav_a, wav_b, frame_size_po2, offset);

    if (avg_diff < best_avg_abs_diff)
    {
      best_avg_abs_diff = avg_diff;
      best_offset = offset;
    }

    if (avg_diff <= avg_diff_threshold)
    {
      break;
    }
  }

#if 0
  yae::TAudioFrame frame_a;
  yae::TAudioFrame frame_b;

  wav_a.rewind();
  wav_b.rewind();

  if (best_offset > 0)
  {
    wav_b.seek_to(best_offset);
  }
  else
  {
    wav_a.seek_to(-best_offset);
  }

  yae::load(wav_a, frame_a, frame_size_po2);
  yae::load(wav_b, frame_b, frame_size_po2);

  while (frame_a.num_samples() == frame_b.num_samples() &&
         frame_a.num_samples() > 0)
  {
    yae::AvFrm wav_overlap = draw_wav_overlap(frame_a, frame_b, 0);
    save_as_png(wav_overlap,
                yae::strfmt("/tmp/overlap-%08" PRIi64 "-aligned-%i-",
                            frame_a.time_.time_,
                            best_offset),
                frame_a.duration());

    // load next frame:
    yae::load(wav_a, frame_a, frame_size_po2);
    yae::load(wav_b, frame_b, frame_size_po2);
  }
#endif

  return best_offset;
}
