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
  // cc_data_channel
  //
  // figure out CEA-608 captions data channel based on the first
  // byte of the control code byte pair:
  //
  static inline int
  cc_data_channel(unsigned char b0)
  {
    return
      (b0 == 0x10 || b0 == 0x11 || b0 == 0x12 || b0 == 0x13 ||
       b0 == 0x14 || b0 == 0x15 || b0 == 0x16 || b0 == 0x17) ?
      1 :
      (b0 == 0x18 || b0 == 0x19 || b0 == 0x1A || b0 == 0x1B ||
       b0 == 0x1C || b0 == 0x1D || b0 == 0x1E || b0 == 0x1F) ?
      2 : 0;
  }

  //----------------------------------------------------------------
  // contains
  //
  static inline bool
  contains(unsigned short low, unsigned short high, unsigned short v)
  {
    return (v <= high && v >= low);
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
  // split_cc_packets_by_channel
  //
  bool
  split_cc_packets_by_channel(int64_t pts,
                              const cc_data_pkt_t * cc_data_pkt,
                              const cc_data_pkt_t * cc_data_end,
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

      // shortcut:
      unsigned char field_number = 1 + (unsigned char)cc_type;

      // both bytes in the byte-pair are supposed to have odd parity,
      // although I've seen QuickTime c608 where that's not the case
      if (!(parity_lut[pkt.b0] &&
            parity_lut[pkt.b1]))
      {
        // YAE_ASSERT(false);
        // continue;
      }

      unsigned char b0 = (pkt.b0 & 0x80) ? (pkt.b0 ^ 0x80) : pkt.b0;
      unsigned char b1 = (pkt.b1 & 0x80) ? (pkt.b1 ^ 0x80) : pkt.b1;
      unsigned short b01 = byte_pair(b0, b1);

      unsigned int data_channel = cc_data_channel(b0);
      if (data_channel > 0 && data_channel < 2)
      {
        dataChannel[field_number - 1] = data_channel;
      }
      else
      {
        data_channel = dataChannel[field_number - 1];
      }

      if (data_channel == 2)
      {
        // convert data channel 2 into data channel 1:
        b0 -= 8;
        b01 = byte_pair(b0, b1);
      }

      if (field_number == 2)
      {
        // convert from field number 2 to field number 1:
        if (contains(0x1520, 0x152f, b01))
        {
          b0 = 0x14;
          b01 = byte_pair(b0, b1);
        }

        pkt.cc &= ~((int)NTSC_CC_FIELD_2);
      }

      unsigned int n = 2 * (field_number - 1) + (data_channel - 1);
      pkt.b0 = parity_lut[b0] ? b0 : (b0 ^ 0x80);
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
      av_new_packet(&tmp, nbytes);
      memcpy(tmp.data, p, nbytes);
      tmp.pts = pts;
      pkt[i] = tmp;
    }

    return !pkt.empty();
  }


  //----------------------------------------------------------------
  // adjust_ass_header
  //
  std::string
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
}
