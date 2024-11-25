// -*- Mode: c++; tab-width: 8; c-basic-offset: 2; indent-tabs-mode: nil -*-
// NOTE: the first line of this file sets up source code indentation rules
// for Emacs; it is also a hint to anyone modifying this file.

// Created   : Sun Feb  5 18:14:17 MST 2017
// Copyright : Pavel Koshevoy
// License   : MIT -- http://www.opensource.org/licenses/mit-license.php

#ifndef YAE_CLOSED_CAPTIONS_H_
#define YAE_CLOSED_CAPTIONS_H_

// aeyae:
#include "yae/api/yae_api.h"
#include "yae/ffmpeg/yae_track.h"
#include "yae/ffmpeg/yae_subtitles_track.h"

// standard:
#include <map>
#include <string>

// ffmpeg:
extern "C"
{
#include <libavcodec/avcodec.h>
}


namespace yae
{

  //----------------------------------------------------------------
  // cc_data_pkt_type_t
  //
  enum cc_data_pkt_type_t
  {
    NTSC_CC_FIELD_1 = 0,
    NTSC_CC_FIELD_2 = 1,
    DTVCC_PACKET_DATA = 2,
    DTVCC_PACKET_START = 3
  };

  //----------------------------------------------------------------
  // cc_data_pkt_t
  //
  struct YAE_API cc_data_pkt_t
  {
    unsigned char cc;
    unsigned char b0;
    unsigned char b1;

    inline cc_data_pkt_type_t cc_type() const
    { return (cc_data_pkt_type_t)(cc & 3); }

    inline bool is_valid() const
    {
      return (// '1' bit
              (cc & 0x80) == 0x80 &&
              // followed by '1111' reserved
              // followed by cc_valid bit
              (cc & 0x04) == 0x04);
    }

    inline bool is_cea608() const
    {
      cc_data_pkt_type_t cc_type = (cc_data_pkt_type_t)(cc & 3);
      return (cc_type == NTSC_CC_FIELD_1 ||
              cc_type == NTSC_CC_FIELD_2);
    }

    inline bool is_dtvcc() const
    {
      cc_data_pkt_type_t cc_type = (cc_data_pkt_type_t)(cc & 3);
      return (cc_type == DTVCC_PACKET_DATA ||
              cc_type == DTVCC_PACKET_START);
    }
  };

  //----------------------------------------------------------------
  // qt_atom_t
  //
  struct YAE_API qt_atom_t
  {
    const uint8_t * fourcc_;
    const uint8_t * data_;
    std::size_t size_;
  };

  //----------------------------------------------------------------
  // parse_qt_atom
  //
  YAE_API bool
  parse_qt_atom(const uint8_t * data,
                const std::size_t size,
                qt_atom_t & atom);

  //----------------------------------------------------------------
  // convert_quicktime_c608
  //
  // wrap CEA-608 in CEA-708 cc_data_pkt wrappers,
  // it's what the ffmpeg closed captions decoder expects:
  //
  YAE_API bool
  convert_quicktime_c608(AVPacket & pkt);

  //----------------------------------------------------------------
  // convert_quicktime_c708
  //
  YAE_API bool
  convert_quicktime_c708(AVPacket & pkt);

  //----------------------------------------------------------------
  // CaptionsDecoder
  //
  struct YAE_API CaptionsDecoder
  {
    CaptionsDecoder();

    void reset();

    // 0 - disabled
    // 1 - CC1
    // 2 - CC2
    // 3 - CC3
    // 4 - CC4
    void enableClosedCaptions(unsigned int cc);

    // helpers:
    void decode(const AVRational & timeBase,
                const AVFrame & frame,
                QueueWaitMgr * terminator);

    void decode(const AVRational & timeBase,
                const AVPacket & packet,
                QueueWaitMgr * terminator);

    void decode(int64_t pts,
                const AVRational & timeBase,
                std::map<unsigned char, AvPkt> & cc,
                QueueWaitMgr * terminator);

    // is decoding enabled for any of the channels:
    inline unsigned int enabled() const
    { return decode_ < 5 ? decode_ : 0; }

    // accessor to the selected captions channel:
    inline SubtitlesTrack * captions()
    { return enabled() ? &(captions_[decode_ - 1]) : NULL; }

  protected:
    // which channel to decode:
    unsigned int decode_;

    // we change the Style of the captions a little:
    std::string adjusted_subtitle_header_[4];

    // decoded captions will go here:
    SubtitlesTrack captions_[4];

    // CEA-608 closed captions decoders, one per channel:
    AvCodecContextPtr cc_[4];

    // for keeping track of previous/current CEA-608 data channel:
    unsigned char dataChannel_[2];
  };

}


#endif // YAE_CLOSED_CAPTIONS_H_

