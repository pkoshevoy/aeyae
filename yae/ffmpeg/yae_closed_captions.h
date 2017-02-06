// -*- Mode: c++; tab-width: 8; c-basic-offset: 2; indent-tabs-mode: nil -*-
// NOTE: the first line of this file sets up source code indentation rules
// for Emacs; it is also a hint to anyone modifying this file.

// Created   : Sun Feb  5 18:14:17 MST 2017
// Copyright : Pavel Koshevoy
// License   : MIT -- http://www.opensource.org/licenses/mit-license.php

#ifndef YAE_CLOSED_CAPTIONS_H_
#define YAE_CLOSED_CAPTIONS_H_

// standard libraries:
#include <map>
#include <string>

// yae includes:
#include "../api/yae_api.h"
#include "../ffmpeg/yae_track.h"


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
  };

  //----------------------------------------------------------------
  // split_cc_packets_by_channel
  //
  // split data into separate packets based on field and data channel,
  // and convert CC2, CC3, CC4 into CC1 (because that's the only one
  // supported by the ffmpeg captions decoder).
  //
  YAE_API bool
  split_cc_packets_by_channel(int64_t pts,
                              const cc_data_pkt_t * cc_data_pkt,
                              const cc_data_pkt_t * cc_data_end,
                              unsigned char dataChannel[2],
                              std::map<unsigned char, AvPkt> & pkt);

  //----------------------------------------------------------------
  // adjust_ass_header
  //
  YAE_API std::string
  adjust_ass_header(const std::string & header);

}


#endif // YAE_CLOSED_CAPTIONS_H_

