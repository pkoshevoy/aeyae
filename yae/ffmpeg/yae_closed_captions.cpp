// -*- Mode: c++; tab-width: 8; c-basic-offset: 2; indent-tabs-mode: nil -*-
// NOTE: the first line of this file sets up source code indentation rules
// for Emacs; it is also a hint to anyone modifying this file.

// Created   : Sun Feb  5 18:14:17 MST 2017
// Copyright : Pavel Koshevoy
// License   : MIT -- http://www.opensource.org/licenses/mit-license.php

// standard libraries:
#include <vector>

// boost library:
#include <boost/algorithm/string.hpp>

// yae includes:
#include "yae/ffmpeg/yae_closed_captions.h"
#include "yae/utils/yae_utils.h"

// namespace shortcuts:
namespace al = boost::algorithm;


namespace yae
{

  //----------------------------------------------------------------
  // kAvTimeBase
  //
  static const Rational kAvTimeBase(1, AV_TIME_BASE);

  //----------------------------------------------------------------
  // cc_data_channel
  //
  // figure out CEA-608 captions data channel based on the first
  // byte of the control code byte pair:
  //
  static inline int
  cc_data_channel(unsigned char b0)
  {
    // Data Channel 1, C1
    // Data Channel 2, C2
    // Data Channel 3, XDS
    return
      (b0 == 0x10 || b0 == 0x11 || b0 == 0x12 || b0 == 0x13 ||
       b0 == 0x14 || b0 == 0x15 || b0 == 0x16 || b0 == 0x17) ?
      1 :
      (b0 == 0x18 || b0 == 0x19 || b0 == 0x1A || b0 == 0x1B ||
       b0 == 0x1C || b0 == 0x1D || b0 == 0x1E || b0 == 0x1F) ?
      2 : (b0 < 0x10) ?
      3 : 0;
  }

  //----------------------------------------------------------------
  // byte_pair_in_range
  //
  static inline bool
  byte_pair_in_range(unsigned short v, unsigned short low, unsigned short high)
  {
    unsigned char a0 = 0xFF & (low >> 8);
    unsigned char a1 = 0xFF & low;
    unsigned char z0 = 0xFF & (high >> 8);
    unsigned char z1 = 0xFF & high;
    unsigned char v0 = 0xFF & (v >> 8);
    unsigned char v1 = 0xFF & v;
    return (v0 <= z0 && v0 >= a0 && v1 <= z1 && v1 >= a1);
  }

  //----------------------------------------------------------------
  // byte_pair
  //
  static inline unsigned short
  byte_pair(unsigned char b0, unsigned char b1)
  {
    return (((unsigned short)b0) << 8) | (unsigned short)b1;
  }

  //----------------------------------------------------------------
  // set_odd_parity
  //
  static inline unsigned char
  set_odd_parity(unsigned char b)
  {
    return parity_lut[b] ? b : (b ^ 0x80);
  }

  //----------------------------------------------------------------
  // clear_odd_parity
  //
  static inline unsigned char
  clear_odd_parity(unsigned char b)
  {
    return (b & 0x80) ? (b ^ 0x80) : b;
  }

  //----------------------------------------------------------------
  // parse_qt_atom
  //
  bool
  parse_qt_atom(const uint8_t * data,
                const std::size_t size,
                qt_atom_t & atom)
  {
    if (size < 8)
    {
      return false;
    }

    uint64_t sz_hdr = 8;
    uint64_t sz =
      ((uint64_t)(data[0])) << 24 |
      ((uint64_t)(data[1])) << 16 |
      ((uint64_t)(data[2])) << 8 |
      ((uint64_t)(data[3]));

    if (sz == 0)
    {
      if (size < 16)
      {
        return false;
      }

      sz_hdr = 16;
      sz =
        ((uint64_t)(data[0])) << 56 |
        ((uint64_t)(data[1])) << 48 |
        ((uint64_t)(data[2])) << 40 |
        ((uint64_t)(data[3])) << 32 |
        ((uint64_t)(data[4])) << 24 |
        ((uint64_t)(data[5])) << 16 |
        ((uint64_t)(data[6])) << 8 |
        ((uint64_t)(data[7]));
    }

    if (sz > size)
    {
      YAE_ASSERT(false);
      return false;
    }

    atom.fourcc_ = data + (sz_hdr - 4);
    atom.data_ = data + sz_hdr;
    atom.size_ = sz - sz_hdr;

    return true;
  }

  //----------------------------------------------------------------
  // convert_quicktime_c608
  //
  // wrap CEA-608 in CEA-708 cc_data_pkt wrappers,
  // it's what the ffmpeg closed captions decoder expects:
  //
  bool
  convert_quicktime_c608(AVPacket & pkt)
  {
    std::vector<cc_data_pkt_t> cc;
    const uint8_t * data = pkt.data;
    const uint8_t * end = pkt.data + pkt.size;

    while (data < end)
    {
      qt_atom_t atom;
      if (!parse_qt_atom(data, end - data, atom))
      {
        return false;
      }

      unsigned char field =
        memcmp(atom.fourcc_, "cdat", 4) == 0 ? 1 :
        memcmp(atom.fourcc_, "cdt2", 4) == 0 ? 2 :
        0;

      if (!field)
      {
        return false;
      }

      if (atom.size_ & 0x1)
      {
        YAE_ASSERT(false);
        return false;
      }

      const uint8_t * head = atom.data_;
      const uint8_t * tail = atom.data_ + atom.size_;
      for (; head < tail; head += 2)
      {
        cc_data_pkt_t pkt;
        pkt.cc = 0xFC | (field == 1 ? 0 : 1);

        uint8_t b0 = clear_odd_parity(head[0]);
        uint8_t b1 = clear_odd_parity(head[1]);
        uint8_t b01 = byte_pair(b0, b1);

        if (field == 1 && byte_pair_in_range(b01, 0x1520, 0x152f))
        {
          // for field 1 these control codes should start with 0x14
          // according to CEA-608 spec:
          b0 = 0x14;
        }

        if (field == 2 && byte_pair_in_range(b01, 0x1420, 0x142f))
        {
          // for field 1 these control codes should start with 0x14
          // according to CEA-608 spec:
          b0 = 0x15;
        }

        pkt.b0 = set_odd_parity(b0);
        pkt.b1 = set_odd_parity(b1);
        cc.push_back(pkt);
      }

      data = tail;
    }

    const std::size_t nbytes = cc.size() * sizeof(cc_data_pkt_t);
    if (nbytes)
    {
      YAE_ASSERT(nbytes % sizeof(cc_data_pkt_t) == 0);
      const cc_data_pkt_t * p = &(cc[0]);

      if (((std::size_t)(pkt.size)) < nbytes)
      {
        av_grow_packet(&pkt, nbytes - pkt.size);
      }
      else
      {
        av_shrink_packet(&pkt, nbytes);
      }

      memcpy(pkt.data, p, nbytes);
    }

    return true;
  }

  //----------------------------------------------------------------
  // convert_quicktime_c708
  //
  //
  bool
  convert_quicktime_c708(AVPacket & pkt)
  {
    std::vector<cc_data_pkt_t> cc;
    const uint8_t * data = pkt.data;
    const uint8_t * end = pkt.data + pkt.size;

    while (data < end)
    {
      qt_atom_t atom;
      if (!parse_qt_atom(data, end - data, atom))
      {
        return false;
      }

      if (memcmp(atom.fourcc_, "ccdp", 4) != 0)
      {
        return false;
      }

      const std::size_t num_cc_pkts = (atom.data_[8] & 0x1f);
      const std::size_t cc_pkt_bytes = num_cc_pkts * 3;;

      if (atom.size_ < cc_pkt_bytes + 9)
      {
        YAE_ASSERT(false);
        return false;
      }

      const cc_data_pkt_t * cc_data_pkt =
        (const cc_data_pkt_t *)(atom.data_ + 9);

      const cc_data_pkt_t * cc_data_end =
        (const cc_data_pkt_t *)(atom.data_ + 9 + cc_pkt_bytes);

      for (; cc_data_pkt < cc_data_end; ++cc_data_pkt)
      {
        cc.push_back(*cc_data_pkt);
      }

      data = atom.data_ + atom.size_;
    }

    const std::size_t nbytes = cc.size() * sizeof(cc_data_pkt_t);
    if (nbytes)
    {
      YAE_ASSERT(nbytes % sizeof(cc_data_pkt_t) == 0);
      const cc_data_pkt_t * p = &(cc[0]);

      if (((std::size_t)(pkt.size)) < nbytes)
      {
        av_grow_packet(&pkt, nbytes - pkt.size);
      }
      else
      {
        av_shrink_packet(&pkt, nbytes);
      }

      memcpy(pkt.data, p, nbytes);
    }

    return true;
  }

  //----------------------------------------------------------------
  // split_cc_packets_by_channel
  //
  // split data into separate packets based on field and data channel,
  // and convert CC2, CC3, CC4 into CC1 (because that's the only one
  // supported by the ffmpeg captions decoder).
  //
  static bool
  split_cc_packets_by_channel(int64_t pts,
                              const cc_data_pkt_t * cc_data_pkt,
                              const cc_data_pkt_t * cc_data_end,
                              unsigned char prior[2][2],
                              unsigned char dataChannel[2],
                              std::map<unsigned char, AvPkt> & pkt)
  {
    std::vector<cc_data_pkt_t> cc[4];
    for (; cc_data_pkt < cc_data_end; ++cc_data_pkt)
    {
      // https://en.wikipedia.org/wiki/CEA-708
      cc_data_pkt_t pkt = *cc_data_pkt;

      bool valid = !!(pkt.cc & 4);
      if (!valid)
      {
        continue;
      }

      cc_data_pkt_type_t cc_type = (cc_data_pkt_type_t)(pkt.cc & 3);
      if (cc_type != NTSC_CC_FIELD_1 &&
          cc_type != NTSC_CC_FIELD_2)
      {
        continue;
      }

      // shortcuts:
      unsigned char field_number = 1 + (unsigned char)cc_type;
      unsigned char & prior_c0 = prior[cc_type][0];
      unsigned char & prior_c1 = prior[cc_type][1];

      const bool odd_parity_b0 = parity_lut[pkt.b0];
      const bool odd_parity_b1 = parity_lut[pkt.b1];

      if (!odd_parity_b0 && !odd_parity_b1)
      {
        // uncorrectable parity error:
        continue;
      }

      unsigned char b0 = clear_odd_parity(pkt.b0);
      unsigned char b1 = clear_odd_parity(pkt.b1);
      unsigned short b01 = byte_pair(b0, b1);

      if (byte_pair_in_range(b01, 0x1020, 0x1f7f))
      {
        if (!odd_parity_b1)
        {
          // if the second byte of the control code fails parity
          // then ignore the control code:
          continue;
        }

        if (prior_c0 == pkt.b0 && prior_c1 == pkt.b1)
        {
          // ignore the redundant control code:
          continue;
        }

        if (!odd_parity_b0 && prior_c1 == pkt.b1)
        {
          // ignore failed redundant control code:
          continue;
        }
      }

      if (!odd_parity_b0)
      {
        b0 = 0x7f;
      }

      if (!odd_parity_b1)
      {
        b1 = 0x7f;
      }

      b01 = byte_pair(b0, b1);

      if (b01)
      {
        // update prior control codes:
        prior_c0 = pkt.b0;
        prior_c1 = pkt.b1;
      }

      unsigned int data_channel = cc_data_channel(b0);
      if (!data_channel)
      {
        data_channel = dataChannel[field_number - 1];
      }
      else if (data_channel < 3)
      {
        dataChannel[field_number - 1] = data_channel;
      }

      if (data_channel == 2)
      {
        // convert data channel 2 into data channel 1:
        b0 -= 8;
        b01 = byte_pair(b0, b1);
      }

      if (data_channel == 3)
      {
        // XDS, ignore until normal captioning resumes:
        continue;
      }

      if (field_number == 2)
      {
        // convert from field number 2 to field number 1:
        if (byte_pair_in_range(b01, 0x1520, 0x152f))
        {
          b0 = 0x14;
          b01 = byte_pair(b0, b1);
          (void)b01;
        }

        pkt.cc ^= (unsigned char)NTSC_CC_FIELD_2;
      }

      unsigned int n = 2 * (field_number - 1) + (data_channel - 1);
      pkt.b0 = set_odd_parity(b0);
      pkt.b1 = set_odd_parity(b1);
      cc[n].push_back(pkt);
    }

    for (unsigned char i = 0; i < 4; i++)
    {
      const std::size_t n = cc[i].size();
      if (!n)
      {
        continue;
      }

      const cc_data_pkt_t * p = &(cc[i][0]);
      const std::size_t nbytes = n * sizeof(*p);

      AvPkt tmp;
      AVPacket & packet = tmp.get();
      av_new_packet(&packet, nbytes);
      memcpy(packet.data, p, nbytes);
      packet.pts = pts;
      pkt[i] = tmp;
    }

    return !pkt.empty();
  }

  //----------------------------------------------------------------
  // split_cc_packets_by_channel
  //
  // split data into separate packets based on field and data channel,
  // and convert CC2, CC3, CC4 into CC1 (because that's the only one
  // supported by the ffmpeg captions decoder).
  //
  static bool
  split_cc_packets_by_channel(const AVPacket & src,
                              unsigned char prior[2][2],
                              unsigned char dataChannel[2],
                              std::map<unsigned char, AvPkt> & cc)
  {
    YAE_ASSERT(src.size % sizeof(cc_data_pkt_t) == 0);
    const cc_data_pkt_t * cc_data_pkt = (const cc_data_pkt_t *)(src.data);
    const cc_data_pkt_t * cc_data_end = (const cc_data_pkt_t *)(src.data +
                                                                src.size);

    if (!split_cc_packets_by_channel(src.pts,
                                     cc_data_pkt,
                                     cc_data_end,
                                     prior,
                                     dataChannel,
                                     cc))
    {
      return false;
    }

    for (std::map<unsigned char, AvPkt>::iterator
           i = cc.begin(), end = cc.end(); i != end; ++i)
    {
      AVPacket & dst = i->second.get();
      av_packet_copy_props(&dst, &src);
    }

    return true;
  }

  //----------------------------------------------------------------
  // adjust_ass_header
  //
  static std::string
  adjust_ass_header(const std::string & header)
  {
    /*
[Script Info]
; Script generated by FFmpeg/Lavc57.75.100
ScriptType: v4.00+
PlayResX: 384
PlayResY: 288

[V4+ Styles]
Format: Name, Fontname, Fontsize, PrimaryColour, SecondaryColour, OutlineColour, BackColour, Bold, Italic, Underline, StrikeOut, ScaleX, ScaleY, Spacing, Angle, BorderStyle, Outline, Shadow, Alignment, MarginL, MarginR, MarginV, Encoding
Style: Default,Monospace,16,&Hffffff,&Hffffff,&H0,&H0,0,0,0,0,100,100,0,0,3,1,0,2,10,10,10,0

[Events]
Format: Layer, Start, End, Style, Name, MarginL, MarginR, MarginV, Effect, Text
     */
    typedef std::string::size_type TStrPos;
    TStrPos c0 = header.find("[V4+ Styles]");

    TStrPos c1 = header.find("Format: ", c0 + 12);
    TStrPos c2 = header.find("\r\n", c1 + 8);

    TStrPos c3 = header.find("Style: ", c2 + 1);
    TStrPos c4 = header.find("\r\n", c3 + 7);

    std::string format = header.substr(c1 + 8, c2 - c1 - 8);
    std::string style = header.substr(c3 + 7, c4 - c3 - 7);

    std::vector<std::string> field;
    boost::algorithm::split(field, format,
                            boost::is_any_of(", "),
                            boost::token_compress_on);

    std::vector<std::string> value;
    boost::algorithm::split(value, style,
                            boost::is_any_of(", "),
                            boost::token_compress_on);

    std::size_t n = field.size();
    YAE_ASSERT(n == value.size());

    if (n == value.size() && n > 0)
    {
      for (std::size_t i = 0; i < n; i++)
      {
        const std::string & k = field[i];
        std::string & v = value[i];

        if (k == "Fontname")
        {
#ifdef _WIN32
          v = "Lucida Console";
#endif
        }
        else if (k == "Fontsize")
        {
#ifdef _WIN32
          v = "13";
#endif
        }
        else if (k == "PrimaryColour")
        {
          // &hAABBGGRR:
          v = "&Hff00ffff";
          // v = "&Hffffffff";
        }
        else if (k == "SecondaryColour")
        {
          v = "&Hffffffff";
          // v = "&Hff00ffff";
        }
        else if (k == "OutlineColour")
        {
          // outline color, AABBGGRR:
          // v = "&H3f808080";
          v = "&H7f000000";
        }
        else if (k == "BackColour")
        {
          // shadow color:
          v = "&H3f000000";
        }
        else if (k == "BorderStyle")
        {
          // 1: outline + drop shadow
          // 3: opaque box
          v = "3";
        }
        else if (k == "Outline")
        {
          v = "2";
        }
        else if (k == "Shadow")
        {
          v = "1";
        }
        else if (k == "Alignment")
        {
          // 1: left
          // 2: center
          // 3: right
          //
          // 1,2,3 -- sub-title
          // 4,5,6 -- mid-title
          // 7,8,9 -- top-title

          v = "1";
        }
      }

      std::ostringstream oss;
      oss << header.substr(0, c3 + 7) << value[0];

      for (std::size_t i = 1; i < n; i++)
      {
        oss << ", " << value[i];
      }

      oss << header.substr(c4);

      std::string result(oss.str().c_str());
      return result;
    }

    return header;
  }


  //----------------------------------------------------------------
  // openClosedCaptionsDecoder
  //
  static AvCodecContextPtr
  openClosedCaptionsDecoder(const AVRational & timeBase)
  {
    AvCodecContextPtr ccDec;

    const AVCodec * codec = avcodec_find_decoder(AV_CODEC_ID_EIA_608);

    if (codec)
    {
      AVDictionary * opts = NULL;
      av_dict_set_int(&opts, "real_time", 1, 0);

      ccDec = tryToOpen(codec, NULL, opts);
      AVCodecContext * cc = ccDec.get();

      if (cc)
      {
        cc->pkt_timebase = timeBase;
        cc->sub_text_format = FF_SUB_TEXT_FMT_ASS_WITH_TIMINGS;
      }
    }

    return ccDec;
  }


  //----------------------------------------------------------------
  // CaptionsDecoder::CaptionsDecoder
  //
  CaptionsDecoder::CaptionsDecoder():
    decode_(0)
  {
    reset();
  }

  //----------------------------------------------------------------
  // CaptionsDecoder::reset
  //
  void
  CaptionsDecoder::reset()
  {
    // for each channel:
    for (unsigned char i = 0; i < 4; i++)
    {
      cc_[i].reset();
      captions_[i].clear();
    }

    // for each field:
    for (unsigned char i = 0; i < 2; i++)
    {
      prior_[i][0] = 0;
      prior_[i][1] = 0;
      dataChannel_[i] = 1;
    }
  }

  //----------------------------------------------------------------
  // CaptionsDecoder::enableClosedCaptions
  //
  void
  CaptionsDecoder::enableClosedCaptions(unsigned int cc)
  {
    if (cc < 5)
    {
      decode_ = cc;
    }
  }

  //----------------------------------------------------------------
  // makeCcPkt
  //
  static bool
  makeCcPkt(const AVFrame & frame,
            unsigned char prior[2][2],
            unsigned char dataChannel[2],
            std::map<unsigned char, AvPkt> & pkt)
  {
    int found = 0;

    for (int i = 0; i < frame.nb_side_data; i++)
    {
      const AVFrameSideData * s = frame.side_data[i];
      if (!s || s->type != AV_FRAME_DATA_A53_CC)
      {
        continue;
      }

      found++;

      if (found > 1)
      {
        YAE_ASSERT(false);
        continue;
      }

      // s->data consists of CEA-708 cc_data_pkt's
      const cc_data_pkt_t * cc_data_pkt = (const cc_data_pkt_t *)(s->data);
      const cc_data_pkt_t * cc_data_end = (const cc_data_pkt_t *)(s->data +
                                                                  s->size);
      split_cc_packets_by_channel(frame.pts,
                                  cc_data_pkt,
                                  cc_data_end,
                                  prior,
                                  dataChannel,
                                  pkt);
    }

    YAE_ASSERT(found <= 1);
    return !pkt.empty();
  }

  //----------------------------------------------------------------
  // CaptionsDecoder::decode
  //
  void
  CaptionsDecoder::decode(const AVRational & timeBase,
                          const AVFrame & frame,
                          QueueWaitMgr * terminator)
  {
    std::map<unsigned char, AvPkt> cc;
    makeCcPkt(frame, prior_, dataChannel_, cc);
    decode(frame.pts, timeBase, cc, terminator);
  }

  //----------------------------------------------------------------
  // parse_timespan
  //
  static bool
  parse_timespan(const AVSubtitle & sub,
                 TTime & tmin,
                 TTime & tmax)
  {
    // std::vector<std::string> columns;

    tmin = TTime::max_flicks();
    tmax = TTime::min_flicks();

    for (int r = 0; r < sub.num_rects; r++)
    {
      const AVSubtitleRect * rect = sub.rects[r];
      if (rect)
      {
        std::vector<std::string> values;
        yae::split(values, ",", rect->ass);

        // NOTE: the values are assumed to match the [Events] Format:
        // values[1] == Start and values[2] == End
        if (values.size() < 3)
        {
          continue;
        }

        // shortcuts:
        std::string & start = values[1];
        std::string & end = values[2];

        yae::strip_ws(start);
        yae::strip_ws(end);

        TTime t0;
        if (yae::parse_time(t0, start.c_str(), ":", "."))
        {
          tmin = std::min(tmin, t0);
        }

        TTime t1;
        if (yae::parse_time(t1, end.c_str(), ":", "."))
        {
          tmax = std::max(tmax, t1);
        }
      }
    }

    if (tmax <= tmin &&
        sub.pts &&
        sub.end_display_time != std::numeric_limits<uint32_t>::max())
    {
#if 0
      tmax = tmin + TTime(sub.end_display_time - sub.start_display_time, 1000);
      tmax = tmax.rebased(100);
#else
      tmin = tmax - TTime(sub.end_display_time - sub.start_display_time, 1000);
      tmin = tmin.rebased(100);
#endif
    }

    return tmin <= tmax;
  }

  //----------------------------------------------------------------
  // CaptionsDecoder::decode
  //
  void
  CaptionsDecoder::decode(const AVRational & timeBase,
                          const AVPacket & packet,
                          QueueWaitMgr * terminator)
  {
    std::map<unsigned char, AvPkt> cc;
    split_cc_packets_by_channel(packet, prior_, dataChannel_, cc);
    decode(packet.pts, timeBase, cc, terminator);
  }

  //----------------------------------------------------------------
  // CaptionsDecoder::decode
  //
  void
  CaptionsDecoder::decode(int64_t pts,
                          const AVRational & timeBase,
                          std::map<unsigned char, AvPkt> & cc,
                          QueueWaitMgr * terminator)
  {
    for (std::map<unsigned char, AvPkt>::iterator
           i = cc.begin(), end = cc.end(); i != end; ++i)
    {
      // shortcuts:
      const unsigned char n = i->first;
      AvPkt & pkt = i->second;
      AVPacket & packet = pkt.get();

      // this shouldn't be necessary -- it's fine to decode all caption
      // channels all the time, because it makes switching between
      // them more seamless.  However, I have no sources to test with
      // that contain anything besides CC1, so I'll limit it to CC1
      // for now:
      if (((unsigned int)(n)) + 1 != decode_)
      {
        continue;
      }

      // instantiate the CC decoder on-demand:
      cc_[n] || (cc_[n] = openClosedCaptionsDecoder(timeBase));

      // prevent captions decoder from being destroyed while it is used:
      AvCodecContextPtr keepAlive(cc_[n]);
      AVCodecContext * ccDec = keepAlive.get();
      if (!ccDec)
      {
        continue;
      }

      AVSubtitle sub;
      int gotSub = 0;
      int err = avcodec_decode_subtitle2(ccDec, &sub, &gotSub, &packet);
      if (err < 0 || !gotSub)
      {
        continue;
      }

      // just parse the time stamp from the Dialogue, it's less broken
      TSubsFrame sf;
      sf.traits_ = kSubsCEA608;
      sf.render_ = true;
      sf.rewriteTimings_ = true;

      if (!parse_timespan(sub, sf.time_, sf.tEnd_) &&
          packet.pts != AV_NOPTS_VALUE)
      {
        sf.time_.base_ = AV_TIME_BASE;
        sf.tEnd_.base_ = AV_TIME_BASE;
        sf.tEnd_.time_ = std::numeric_limits<int64>::max();

        int64_t ptsPkt = av_rescale_q(packet.pts,
                                      timeBase,
                                      kAvTimeBase);
        sf.time_.time_ = ptsPkt;

        int64_t endPkt = av_rescale_q(packet.pts + packet.duration,
                                      timeBase,
                                      kAvTimeBase);
        sf.tEnd_.time_ = endPkt;
      }

      sf.time_ = sf.time_.rebased(30000);
      sf.tEnd_ = std::max(sf.tEnd_, sf.time_ + TTime(1001, 30000));

      std::string header((const char *)(ccDec->subtitle_header),
                         (const char *)(ccDec->subtitle_header +
                                        ccDec->subtitle_header_size));
      header = adjust_ass_header(header);

      const unsigned char * hdr = (const unsigned char *)(&header[0]);
      std::size_t sz = header.size();
      sf.private_ = TSubsPrivatePtr(new TSubsPrivate(sub, hdr, sz),
                                    &TSubsPrivate::deallocator);
      if (sub.num_rects)
      {
        SubtitlesTrack & captions = captions_[n];
        TSubsFrame & last = captions.last_;

        // try to avoid introducing flicker with roll-up captions:
        int64_t nframes = (sf.time_.get(30000) - last.tEnd_.get(30000)) / 1001;
        if (nframes < 3)
        {
          sf.time_ = last.tEnd_;
        }

        last = sf;
        captions.push(sf, terminator);
      }
    }

    // extend the duration of the most recent caption to cover current frame:
    TTime ptsNow = TTime(pts * timeBase.num, timeBase.den).rebased(30000);
    TTime ptsNext = ptsNow + TTime(1001, 30000);

    for (unsigned int i = 0; i < 4; i++)
    {
      if (!cc_[i])
      {
        continue;
      }

      SubtitlesTrack & captions = captions_[i];
      TSubsFrame & last = captions.last_;
      int64_t nframes =
        (ptsNext.get(30000) - last.tEnd_.get(30000) + 1000) / 1001;

      // avoid extending caption duration indefinitely:
      if (nframes && last.tEnd_ < last.time_ + 12.0)
      {
        TTime ptsPrev = last.tEnd_;
        last.tEnd_ += TTime(nframes * 1001, 30000);

        // avoid creating overlapping ASS events,
        // better to create short adjacent events instead:
        TSubsFrame sf(last);
        sf.time_ = ptsPrev;
        captions.push(sf, terminator);
      }
    }
  }

}
