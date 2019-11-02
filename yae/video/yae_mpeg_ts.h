// -*- Mode: c++; tab-width: 8; c-basic-offset: 2; indent-tabs-mode: nil -*-
// NOTE: the first line of this file sets up source code indentation rules
// for Emacs; it is also a hint to anyone modifying this file.

// Created   : Fri Nov  1 22:32:55 MDT 2019
// Copyright : Pavel Koshevoy
// License   : MIT -- http://www.opensource.org/licenses/mit-license.php

#ifndef YAE_MPEG_TS_H_
#define YAE_MPEG_TS_H_

// system includes:
#include <inttypes.h>
#include <list>
#include <map>

// yae includes:
#include "yae/api/yae_api.h"
#include "yae/utils/yae_data.h"


namespace yae
{

  namespace mpeg_ts
  {

    // forward declarations:
    struct YAE_API Context;

    //----------------------------------------------------------------
    // AdaptationField
    //
    struct YAE_API AdaptationField
    {
      AdaptationField();

      void load(IBitstream & bin);

      // 8:
      uint64_t adaptation_field_length_ : 8;

      // 8:
      uint64_t discontinuity_indicator_ : 1;
      uint64_t random_access_indicator_ : 1;
      uint64_t elementary_stream_priority_indicator_ : 1;
      uint64_t pcr_flag_ : 1;
      uint64_t opcr_flag_ : 1;
      uint64_t splicing_point_flag_ : 1;
      uint64_t transport_private_data_flag_ : 1;
      uint64_t adaptation_field_extension_flag_ : 1;

      // 48:
      uint64_t program_clock_reference_base_ : 33;
      uint64_t program_clock_reference_reserved_ : 6;
      uint64_t program_clock_reference_extension_ : 9;

      // 48:
      uint64_t original_program_clock_reference_base_ : 33;
      uint64_t original_program_clock_reference_reserved_ : 6;
      uint64_t original_program_clock_reference_extension_ : 9;

      // 16:
      uint64_t splice_countdown_ : 8;
      uint64_t transport_private_data_length_ : 8;

      // ptr:
      TBufferPtr transport_private_data_;

      struct YAE_API Extension
      {
        Extension();

        void load(IBitstream & bin);

        // 8:
        uint8_t adaptation_field_extension_length_ : 8;

        // 8:
        uint8_t ltw_flag_ : 1;
        uint8_t piecewise_rate_flag_ : 1;
        uint8_t seamless_splice_flag_ : 1;
        uint8_t reserved1_ : 5;

        // 16:
        uint16_t ltw_valid_flag_ : 1;
        uint16_t ltw_offset_ : 15;

        // 24:
        uint64_t reserved2_ : 2;
        uint64_t piecewise_rate_ : 22;

        // 40:
        uint64_t splice_type_ : 4;
        uint64_t dts_next_au_32_30_ : 3;
        uint64_t marker1_ : 1;
        uint64_t dts_next_au_29_15_ : 15;
        uint64_t marker2_ : 1;
        uint64_t dts_next_au_14_00_ : 15;
        uint64_t marker3_ : 1;

        // ptr:
        TBufferPtr reserved_;
      };

      // ptr:
      yae::optional<Extension> extension_;

      // ptr:
      TBufferPtr stuffing_; // 0xFF
    };


    //----------------------------------------------------------------
    // TSPacket
    //
    struct YAE_API TSPacket
    {
      TSPacket();

      void load(IBitstream & bin, Context & ctx);

      uint32_t sync_byte_ : 8; // 0x47

      uint32_t transport_error_indicator_ : 1;
      uint32_t payload_unit_start_indicator_ : 1;
      uint32_t transport_priority_ : 1;

      // 0x0000  Program association table
      // 0x0001  Conditional access table
      // 0x0002  Transport stream description table
      // 0x0003  IPMP control information table
      //
      // 0x0004 - 0x000F reserved
      //
      // 0x0010 - 0x1FFE may be assigned as network_PID, Program_map_PID,
      // elementary_PID, or for other purposes
      //
      // 0x1FFF  Null packet
      uint32_t pid_ : 13;

      // 00  Not scrambled
      uint32_t transport_scrambling_control_ : 2;

      // 00  Reserved for future use by ISO/IEC
      // 01  No AdaptationField, payload only
      // 10  AdaptationField only, no payload
      // 11  AdaptationField folowed by payload
      uint32_t adaptation_field_control_ : 2;

      uint32_t continuity_counter_ : 4;

      // ptr:
      yae::optional<AdaptationField> adaptation_field_;

      // ptr:
      TBufferPtr payload_;
    };


    //----------------------------------------------------------------
    // assemble_payload
    //
    YAE_API yae::Data assemble_payload(std::list<TSPacket> & packets);

    //----------------------------------------------------------------
    // consume
    //
    YAE_API void consume(uint16_t pid, std::list<TSPacket> & packets);

    //----------------------------------------------------------------
    // Context
    //
    struct YAE_API Context
    {
      void load(IBitstream & bin, TSPacket & pkt);

      std::map<uint16_t, std::list<TSPacket> > pes_;
    };

  }
}


#endif // YAE_MPEG_TS_H_
