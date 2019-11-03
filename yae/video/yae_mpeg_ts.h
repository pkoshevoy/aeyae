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
    // SystemHeader
    //
    struct YAE_API SystemHeader
    {
      SystemHeader();

      void load(IBitstream & bin);

      uint32_t system_header_start_code_; // 0x000001BB
      uint16_t header_length_;

      // 24:
      uint64_t marker1_ : 1;
      uint64_t rate_bound_ : 22;
      uint64_t marker2_ : 1;

      // 8:
      uint64_t audio_bound_ : 6;
      uint64_t fixed_flag_ : 1;
      uint64_t csps_flag_ : 1;

      // 8:
      uint64_t system_audio_lock_flag_ : 1;
      uint64_t system_video_lock_flag_ : 1;
      uint64_t marker3_ : 1;
      uint64_t video_bound_ : 5;

      // 8:
      uint64_t packet_rate_restriction_flag_ : 1;
      uint64_t reserved_ : 7;

      struct YAE_API Ext
      {
        Ext();

        void load(IBitstream & bin);

        uint8_t stream_id_;
        uint16_t const1_11_ : 2;
        uint16_t const_0000000_ : 7;
        uint16_t stream_id_extension_ : 7;
        uint8_t const_10110110_;
        uint16_t const_11_ : 2;
        uint16_t pstd_buffer_bound_scale_ : 1;
        uint16_t pstd_buffer_size_bound_ : 13;
      };

      std::list<Ext> ext_;
    };


    //----------------------------------------------------------------
    // PackHeader
    //
    struct YAE_API PackHeader
    {
      PackHeader();

      void load(IBitstream & bin);

      // 32:
      uint32_t pack_start_code_; // 0x000001BA

      // 48:
      uint64_t pack_const_01_ : 2;
      uint64_t system_clock_reference_base_32_30_ : 3;
      uint64_t system_clock_reference_marker1_ : 1;
      uint64_t system_clock_reference_base_29_15_ : 15;
      uint64_t system_clock_reference_marker2_ : 1;
      uint64_t system_clock_reference_base_14_00_ : 15;
      uint64_t system_clock_reference_marker3_ : 1;
      uint64_t system_clock_reference_extension_ : 9;
      uint64_t system_clock_reference_marker4_ : 1;

      // 24:
      uint64_t program_mux_rate_ : 22;
      uint64_t marker1_ : 1;
      uint64_t marker2_ : 1;

      // 8:
      uint64_t reserved_ : 5;
      uint64_t pack_stuffing_length_ : 3;

      // ptr:
      TBufferPtr stuffing_;

      // ptr:
      yae::optional<SystemHeader> system_header_;
    };

    enum StreamId
    {
      // 101111xx
      STREAM_ID_PROGRAM_STREAM_MAP = 0xBC,
      STREAM_ID_PRIVATE_STREAM_1 = 0xBD,
      STREAM_ID_PADDING_STREAM = 0xBE,
      STREAM_ID_PRIVATE_STREAM_2 = 0xBF,

      // 110xxxxx
      STREAM_ID_AUDIO_STREAM_NUMBER_XXXXX = 0xC0,

      // 1110xxxx
      STREAM_ID_VIDEO_STREAM_NUMBER_XXXX = 0xE0,

      // 1111xxxx
      STREAM_ID_ECM = 0xF0,
      STREAM_ID_EMM = 0xF1,
      STREAM_ID_ISO13818_1A_DSMCC = 0xF2,
      STREAM_ID_ISO13522 = 0xF3,
      STREAM_ID_ITUT_H222_1A = 0xF4,
      STREAM_ID_ITUT_H222_1B = 0xF5,
      STREAM_ID_ITUT_H222_1C = 0xF6,
      STREAM_ID_ITUT_H222_1D = 0xF7,
      STREAM_ID_ITUT_H222_1E = 0xF8,
      STREAM_ID_ANCILLARY_STREAM = 0xF9,
      STREAM_ID_ISO_14496_1_SL = 0xFA,
      STREAM_ID_ISO_14496_1_FLEXMUX = 0xFB,
      STREAM_ID_METADATA_STREAM = 0xFC,
      STREAM_ID_EXTENDED_STREAM_ID = 0xFD,
      STREAM_ID_RESERVED_DATA_STREAM = 0xFE,
      STREAM_ID_PROGRAM_STREAM_DIRECTORY = 0xFF
    };

    enum TrickMode
    {
      TRICK_MODE_FAST_FORWARD = 0,
      TRICK_MODE_SLOW_MOTION = 1,
      TRICK_MODE_FREEZE_FRAME = 2,
      TRICK_MODE_FAST_REVERSE = 3,
      TRICK_MODE_SLOW_REVERSE = 4
    };

    //----------------------------------------------------------------
    // PESPacket
    //
    struct YAE_API PESPacket
    {
      PESPacket();

      void load(IBitstream & bin);

      uint32_t packet_start_code_prefix_ : 24;
      uint32_t stream_id_ : 8;
      uint16_t pes_packet_length_;

      struct PES
      {
        PES();

        void load(IBitstream & bin);

        // 8:
        uint64_t pes_const_10_ : 2;
        uint64_t pes_scrambling_control_ : 2;
        uint64_t pes_priority_ : 1;
        uint64_t data_alignment_indicator_ : 1;
        uint64_t copyright_ : 1;
        uint64_t original_or_copy_ : 1;

        // 8:
        uint64_t pts_dts_flags_ : 2;
        uint64_t escr_flag_ : 1;
        uint64_t es_rate_flag_ : 1;
        uint64_t dsm_trick_mode_flag_ : 1;
        uint64_t additional_copy_info_flag_ : 1;
        uint64_t pes_crc_flag_ : 1;
        uint64_t pes_extension_flag_ : 1;

        // 8:
        uint64_t pes_header_data_length_ : 8;

        // 40:
        uint64_t pts_prefix_ : 4;
        uint64_t pts_32_30_ : 3;
        uint64_t pts_marker1_ : 1;
        uint64_t pts_29_15_ : 15;
        uint64_t pts_marker2_ : 1;
        uint64_t pts_14_00_ : 15;
        uint64_t pts_marker3_ : 1;

        // 40:
        uint64_t dts_prefix_ : 4;
        uint64_t dts_32_30_ : 3;
        uint64_t dts_marker1_ : 1;
        uint64_t dts_29_15_ : 15;
        uint64_t dts_marker2_ : 1;
        uint64_t dts_14_00_ : 15;
        uint64_t dts_marker3_ : 1;

        // 48:
        uint64_t escr_reserved_ : 2;
        uint64_t escr_base_32_30_ : 3;
        uint64_t escr_marker1_ : 1;
        uint64_t escr_base_29_15_ : 15;
        uint64_t escr_marker2_ : 1;
        uint64_t escr_base_14_00_ : 15;
        uint64_t escr_marker3_ : 1;
        uint64_t escr_extension_ : 9;
        uint64_t escr_marker4_ : 1;

        // 24:
        uint64_t es_rate_marker1_ : 1;
        uint64_t es_rate_ : 22;
        uint64_t es_rate_marker2_ : 1;

        // 8:
        union
        {
          uint8_t trick_mode_;

          struct
          {
            uint8_t trick_mode_control_ : 3;
            uint8_t field_id_ : 2;
            uint8_t intra_slice_refresh_ : 1;
            uint8_t frequency_truncation_ : 2;
          } fast_;

          struct
          {
            uint8_t trick_mode_control_ : 3;
            uint8_t rep_cntrl_ : 5;
          } slow_;

          struct
          {
            uint8_t trick_mode_control_ : 3;
            uint8_t field_id_ : 2;
            uint8_t reserved_ : 3;
          } freeze_;

          struct
          {
            uint8_t trick_mode_control_ : 3;
            uint8_t reserved_ : 5;
          } mode_;
        };

        // 8:
        uint8_t additional_copy_marker_ : 1;
        uint8_t additional_copy_info_ : 7;

        // 16:
        uint16_t previous_pes_packet_crc_;

        struct YAE_API Extension
        {
          Extension();

          void load(IBitstream & bin);

          // 8:
          uint8_t pes_private_data_flag_ : 1;
          uint8_t pack_header_field_flag_ : 1;
          uint8_t program_packet_sequence_counter_flag_ : 1;
          uint8_t pstd_buffer_flag_ : 1;
          uint8_t reserved_ : 3;
          uint8_t pes_extension_flag_2_ : 1;

          // ptr:
          TBufferPtr pes_private_data_;

          // 8:
          uint8_t pack_field_length_;

          // ptr:
          yae::optional<PackHeader> pack_header_;

          // 16:
          uint16_t program_packet_sequence_counter_marker_ : 1;
          uint16_t program_packet_sequence_counter_ : 7;
          uint16_t mpeg1_mpeg2_identifier_marker_ : 1;
          uint16_t mpeg1_mpeg2_identifier_ : 1;
          uint16_t original_stuff_length_ : 6;

          // 16:
          uint16_t pstd_const_01_ : 2;
          uint16_t pstd_buffer_scale_ : 1;
          uint16_t pstd_buffer_size_ : 13;

          struct YAE_API Ext2
          {
            Ext2();

            void load(IBitstream & bin);

            // 8:
            uint8_t marker_ : 1;
            uint8_t pes_extension_field_length_ : 7;

            // 8:
            union
            {
              struct
              {
                uint8_t extension_flag_ : 1;
                uint8_t extension_ : 7;
              } stream_id_;

              struct
              {
                uint8_t stream_id_extension_flag_ : 1;
                uint8_t stream_id_extension_reserved_ : 6;
                uint8_t extension_flag_ : 1;
              } tref_;
            };

            // 40:
            uint64_t tref_reserved_ : 4;
            uint64_t tref_32_30_ : 3;
            uint64_t tref_marker1_ : 1;
            uint64_t tref_29_15_ : 15;
            uint64_t tref_marker2_ : 1;
            uint64_t tref_14_00_ : 15;
            uint64_t tref_marker3_ : 1;

            // ptr:
            TBufferPtr reserved_;
          };

          // ptr:
          yae::optional<Ext2> ext2_;
        };

        // ptr:
        yae::optional<Extension> extension_;

        // ptr:
        TBufferPtr stuffing_;
      };

      // ptr:
      yae::optional<PES> pes_;

      // ptr:
      TBufferPtr data_;

      // ptr:
      TBufferPtr padding_;
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
