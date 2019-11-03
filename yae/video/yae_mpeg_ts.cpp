// -*- Mode: c++; tab-width: 8; c-basic-offset: 2; indent-tabs-mode: nil -*-
// NOTE: the first line of this file sets up source code indentation rules
// for Emacs; it is also a hint to anyone modifying this file.

// Created   : Sat Nov  2 12:04:37 MDT 2019
// Copyright : Pavel Koshevoy
// License   : MIT -- http://www.opensource.org/licenses/mit-license.php

// yae includes:
#include "yae/api/yae_log.h"
#include "yae/utils/yae_utils.h"
#include "yae/video/yae_mpeg_ts.h"


namespace yae
{

  namespace mpeg_ts
  {

    //----------------------------------------------------------------
    // AdaptationField::Extension::Extension
    //
    AdaptationField::Extension::Extension():
      adaptation_field_extension_length_(0),
      ltw_flag_(0),
      piecewise_rate_flag_(0),
      seamless_splice_flag_(0),
      reserved1_(0),
      ltw_valid_flag_(0),
      ltw_offset_(0),
      reserved2_(0),
      piecewise_rate_(0),
      splice_type_(0),
      dts_next_au_32_30_(0),
      marker1_(0),
      dts_next_au_29_15_(0),
      marker2_(0),
      dts_next_au_14_00_(0),
      marker3_(0)
    {}

    //----------------------------------------------------------------
    // AdaptationField::Extension::load
    //
    void
    AdaptationField::Extension::load(IBitstream & bin)
    {
      adaptation_field_extension_length_ = bin.read(8);
      std::size_t start_pos = bin.position();

      ltw_flag_ = bin.read(1);
      piecewise_rate_flag_ = bin.read(1);
      seamless_splice_flag_ = bin.read(1);
      reserved1_ = bin.read(5);

      if (ltw_flag_)
      {
        ltw_valid_flag_ = bin.read(1);
        ltw_offset_ = bin.read(15);
      }

      if (piecewise_rate_flag_)
      {
        reserved2_ = bin.read(2);
        piecewise_rate_ = bin.read(22);
      }

      if (seamless_splice_flag_)
      {
        splice_type_ = bin.read(4);
        dts_next_au_32_30_ = bin.read(3);
        marker1_ = bin.read(1);
        dts_next_au_29_15_ = bin.read(15);
        marker2_ = bin.read(1);
        dts_next_au_14_00_ = bin.read(15);
        marker3_ = bin.read(1);
      }

      std::size_t end_pos = bin.position();
      std::size_t consumed = end_pos - start_pos;
      YAE_THROW_IF((consumed & 0x7) != 0);

      std::size_t consumed_bytes = consumed >> 3;
      reserved_ = bin.read_bytes(adaptation_field_extension_length_ -
                                 consumed_bytes);
    }


    //----------------------------------------------------------------
    // AdaptationField::AdaptationField
    //
    AdaptationField::AdaptationField():
      adaptation_field_length_(0),
      discontinuity_indicator_(0),
      random_access_indicator_(0),
      elementary_stream_priority_indicator_(0),
      pcr_flag_(0),
      opcr_flag_(0),
      splicing_point_flag_(0),
      transport_private_data_flag_(0),
      adaptation_field_extension_flag_(0),
      program_clock_reference_base_(0),
      program_clock_reference_reserved_(0),
      program_clock_reference_extension_(0),
      original_program_clock_reference_base_(0),
      original_program_clock_reference_reserved_(0),
      original_program_clock_reference_extension_(0),
      splice_countdown_(0),
      transport_private_data_length_(0)
    {}

    //----------------------------------------------------------------
    // AdaptationField::load
    //
    void
    AdaptationField::load(IBitstream & bin)
    {
      adaptation_field_length_ = bin.read(8);
      std::size_t start_pos = bin.position();

      if (adaptation_field_length_)
      {
        discontinuity_indicator_ = bin.read(1);
        random_access_indicator_ = bin.read(1);
        elementary_stream_priority_indicator_ = bin.read(1);
        pcr_flag_ = bin.read(1);
        opcr_flag_ = bin.read(1);
        splicing_point_flag_ = bin.read(1);
        transport_private_data_flag_ = bin.read(1);
        adaptation_field_extension_flag_ = bin.read(1);

        if (pcr_flag_)
        {
          program_clock_reference_base_ = bin.read(33);
          program_clock_reference_reserved_ = bin.read(6);
          program_clock_reference_extension_ = bin.read(9);
        }

        if (opcr_flag_)
        {
          original_program_clock_reference_base_ = bin.read(33);
          original_program_clock_reference_reserved_ = bin.read(6);
          original_program_clock_reference_extension_ = bin.read(9);
        }

        if (splicing_point_flag_)
        {
          splice_countdown_ = bin.read(8);
        }

        if (transport_private_data_flag_)
        {
          transport_private_data_length_ = bin.read(8);
          transport_private_data_ =
            bin.read_bytes(transport_private_data_length_);
        }

        if (adaptation_field_extension_flag_)
        {
          extension_ = Extension();
          extension_->load(bin);
        }

        std::size_t end_pos = bin.position();
        std::size_t consumed = end_pos - start_pos;
        YAE_THROW_IF((consumed & 0x7) != 0);

        std::size_t consumed_bytes = consumed >> 3;
        stuffing_ = bin.read_bytes(adaptation_field_length_ -
                                   consumed_bytes);
      }
    }


    //----------------------------------------------------------------
    // TSPacket::TSPacket
    //
    TSPacket::TSPacket():
      sync_byte_(0x47),
      transport_error_indicator_(0),
      payload_unit_start_indicator_(0),
      transport_priority_(0),
      pid_(0x1FFF),
      transport_scrambling_control_(0),
      adaptation_field_control_(0),
      continuity_counter_(0)
    {}

    //----------------------------------------------------------------
    // TSPacket::load
    //
    void
    TSPacket::load(IBitstream & bin, Context & ctx)
    {
      sync_byte_ = bin.read(8);
      YAE_THROW_IF(sync_byte_ != 0x47);

      transport_error_indicator_ = bin.read(1);
      payload_unit_start_indicator_ = bin.read(1);
      transport_priority_ = bin.read(1);

      pid_ = bin.read(13);

      transport_scrambling_control_ = bin.read(2);
      adaptation_field_control_ = bin.read(2);
      continuity_counter_ = bin.read(4);

      if ((adaptation_field_control_ & 2) == 2)
      {
        adaptation_field_ = AdaptationField();
        adaptation_field_->load(bin);
      }

      if ((adaptation_field_control_ & 1) == 1)
      {
        payload_ = bin.read_remaining_bytes();
      }
    }


    //----------------------------------------------------------------
    // SystemHeader::SystemHeader
    //
    SystemHeader::SystemHeader():
      system_header_start_code_(0),
      header_length_(0),
      marker1_(0),
      rate_bound_(0),
      marker2_(0),
      audio_bound_(0),
      fixed_flag_(0),
      csps_flag_(0),
      system_audio_lock_flag_(0),
      system_video_lock_flag_(0),
      marker3_(0),
      video_bound_(0),
      packet_rate_restriction_flag_(0),
      reserved_(0)
    {}

    //----------------------------------------------------------------
    // SystemHeader::load
    //
    void
    SystemHeader::load(IBitstream & bin)
    {
      system_header_start_code_ = bin.read(32);
      YAE_THROW_IF(system_header_start_code_ != 0x000001BB);

      header_length_ = bin.read(16);

      marker1_ = bin.read(1);
      rate_bound_ = bin.read(22);
      marker2_ = bin.read(1);

      audio_bound_ = bin.read(6);
      fixed_flag_ = bin.read(1);
      csps_flag_ = bin.read(1);

      system_audio_lock_flag_ = bin.read(1);
      system_video_lock_flag_ = bin.read(1);
      marker3_ = bin.read(1);
      video_bound_ = bin.read(5);

      packet_rate_restriction_flag_ = bin.read(1);
      reserved_ = bin.read(7);

      while (bin.next_bits(1, 1))
      {
        ext_.push_back(Ext());
        Ext & ext = ext_.back();
        ext.load(bin);
      }
    }

    //----------------------------------------------------------------
    // SystemHeader::Ext::Ext
    //
    SystemHeader::Ext::Ext():
      stream_id_(0),
      const1_11_(0),
      const_0000000_(0),
      stream_id_extension_(0),
      const_10110110_(0),
      const_11_(0),
      pstd_buffer_bound_scale_(0),
      pstd_buffer_size_bound_(0)
    {}

    //----------------------------------------------------------------
    // SystemHeader::Ext::load
    //
    void
    SystemHeader::Ext::load(IBitstream & bin)
    {
      stream_id_ = bin.read(8);
      if (stream_id_ == 0xB7)
      {
        const1_11_ = bin.read(2);
        YAE_THROW_IF(const1_11_ != 3);

        const_0000000_  = bin.read(7);
        YAE_THROW_IF(const_0000000_ != 0);

        stream_id_extension_ = bin.read(7);

        const_10110110_ = bin.read(8);
        YAE_THROW_IF(const_10110110_ != 0xB6);
      }

      const_11_ = bin.read(2);
      YAE_THROW_IF(const_11_ != 3);

      pstd_buffer_bound_scale_ = bin.read(1);
      pstd_buffer_size_bound_ = bin.read(13);
    }


    //----------------------------------------------------------------
    // PackHeader::PackHeader
    //
    PackHeader::PackHeader():
      pack_start_code_(0),
      pack_const_01_(0),
      system_clock_reference_base_32_30_(0),
      system_clock_reference_marker1_(0),
      system_clock_reference_base_29_15_(0),
      system_clock_reference_marker2_(0),
      system_clock_reference_base_14_00_(0),
      system_clock_reference_marker3_(0),
      system_clock_reference_extension_(0),
      system_clock_reference_marker4_(0),
      program_mux_rate_(0),
      marker1_(0),
      marker2_(0),
      reserved_(0),
      pack_stuffing_length_(0)
    {}

    //----------------------------------------------------------------
    // PackHeader::load
    //
    void PackHeader::load(IBitstream & bin)
    {
      pack_start_code_ = bin.read(32);
      YAE_THROW_IF(pack_start_code_ != 0x000001BB);

      pack_const_01_ = bin.read(2);
      YAE_THROW_IF(pack_const_01_ != 1);

      system_clock_reference_base_32_30_ = bin.read(3);
      system_clock_reference_marker1_ = bin.read(1);
      system_clock_reference_base_29_15_ = bin.read(15);
      system_clock_reference_marker2_ = bin.read(1);
      system_clock_reference_base_14_00_ = bin.read(15);
      system_clock_reference_marker3_ = bin.read(1);
      system_clock_reference_extension_ = bin.read(9);
      system_clock_reference_marker4_ = bin.read(1);

      program_mux_rate_ = bin.read(22);
      marker1_ = bin.read(1);
      marker2_ = bin.read(1);

      reserved_ = bin.read(5);
      pack_stuffing_length_ = bin.read(3);
      stuffing_ = bin.read_bytes(pack_stuffing_length_);

      if (bin.next_bits(32, 0x000001BB))
      {
        system_header_ = SystemHeader();
        SystemHeader & system_header = *system_header_;
        system_header.load(bin);
      }
    }


    //----------------------------------------------------------------
    // PESPacket::PESPacket
    //
    PESPacket::PESPacket():
      packet_start_code_prefix_(0),
      stream_id_(0),
      pes_packet_length_(0)
    {}

    //----------------------------------------------------------------
    // PESPacket::load
    //
    void
    PESPacket::load(IBitstream & bin)
    {
      packet_start_code_prefix_ = bin.read(24);
      YAE_THROW_IF(packet_start_code_prefix_ != 0x000001);

      stream_id_ = bin.read(8);
      pes_packet_length_ = bin.read(16);
      std::size_t start_pos = bin.position();

      if (stream_id_ != STREAM_ID_PROGRAM_STREAM_MAP &&
          stream_id_ != STREAM_ID_PADDING_STREAM  &&
          stream_id_ != STREAM_ID_PRIVATE_STREAM_2 &&
          stream_id_ != STREAM_ID_ECM &&
          stream_id_ != STREAM_ID_EMM &&
          stream_id_ != STREAM_ID_PROGRAM_STREAM_DIRECTORY &&
          stream_id_ != STREAM_ID_ISO13818_1A_DSMCC &&
          stream_id_ != STREAM_ID_ITUT_H222_1E)
      {
        pes_ = PES();

        PES & pes = *pes_;
        pes.load(bin);

        // load data:
        std::size_t end_pos = bin.position();
        std::size_t consumed = end_pos - start_pos;
        YAE_THROW_IF((consumed & 0x7) != 0);

        std::size_t consumed_bytes = consumed >> 3;
        data_ = bin.read_bytes(pes_packet_length_ - consumed_bytes);
      }
      else if (stream_id_ == STREAM_ID_PROGRAM_STREAM_MAP ||
               stream_id_ == STREAM_ID_PRIVATE_STREAM_2 ||
               stream_id_ == STREAM_ID_ECM ||
               stream_id_ == STREAM_ID_EMM ||
               stream_id_ == STREAM_ID_PROGRAM_STREAM_DIRECTORY ||
               stream_id_ == STREAM_ID_ISO13818_1A_DSMCC ||
               stream_id_ == STREAM_ID_ITUT_H222_1E)
      {
        data_ = bin.read_bytes(pes_packet_length_);
      }
      else if (stream_id_ == STREAM_ID_PADDING_STREAM)
      {
        padding_ = bin.read_bytes(pes_packet_length_);
      }
    }


    //----------------------------------------------------------------
    // PESPacket::PES::PES
    //
    PESPacket::PES::PES():
      pes_const_10_(0),
      pes_scrambling_control_(0),
      pes_priority_(0),
      data_alignment_indicator_(0),
      copyright_(0),
      original_or_copy_(0),

      pts_dts_flags_(0),
      escr_flag_(0),
      es_rate_flag_(0),
      dsm_trick_mode_flag_(0),
      additional_copy_info_flag_(0),
      pes_crc_flag_(0),
      pes_extension_flag_(0),

      pes_header_data_length_(0),

      pts_prefix_(0),
      pts_32_30_(0),
      pts_marker1_(0),
      pts_29_15_(0),
      pts_marker2_(0),
      pts_14_00_(0),
      pts_marker3_(0),

      dts_prefix_(0),
      dts_32_30_(0),
      dts_marker1_(0),
      dts_29_15_(0),
      dts_marker2_(0),
      dts_14_00_(0),
      dts_marker3_(0),

      escr_reserved_(0),
      escr_base_32_30_(0),
      escr_marker1_(0),
      escr_base_29_15_(0),
      escr_marker2_(0),
      escr_base_14_00_(0),
      escr_marker3_(0),
      escr_extension_(0),
      escr_marker4_(0),

      es_rate_marker1_(0),
      es_rate_(0),
      es_rate_marker2_(0),

      trick_mode_(0),

      additional_copy_marker_(0),
      additional_copy_info_(0),

      previous_pes_packet_crc_(0)
    {}

    //----------------------------------------------------------------
    // PESPacket::PES::load
    //
    void
    PESPacket::PES::load(IBitstream & bin)
    {
      pes_const_10_ = bin.read(2);
      YAE_THROW_IF(pes_const_10_ != 3);

      pes_scrambling_control_ = bin.read(2);
      pes_priority_ = bin.read(1);
      data_alignment_indicator_ = bin.read(1);
      copyright_ = bin.read(1);
      original_or_copy_ = bin.read(1);

      pts_dts_flags_ = bin.read(2);
      escr_flag_ = bin.read(1);
      es_rate_flag_ = bin.read(1);
      dsm_trick_mode_flag_ = bin.read(1);
      additional_copy_info_flag_ = bin.read(1);
      pes_crc_flag_ = bin.read(1);
      pes_extension_flag_ = bin.read(1);
      pes_header_data_length_ = bin.read(8);
      std::size_t start_pos = bin.position();

      if ((pts_dts_flags_ & 0x2) == 0x2)
      {
        pts_prefix_ = bin.read(4);
        YAE_THROW_IF(pts_prefix_ != pts_dts_flags_);

        pts_32_30_ = bin.read(3);
        pts_marker1_ = bin.read(1);
        pts_29_15_ = bin.read(15);
        pts_marker2_ = bin.read(1);
        pts_14_00_ = bin.read(15);
        pts_marker3_ = bin.read(1);
      }

      if (pts_dts_flags_ == 0x3)
      {
        dts_prefix_ = bin.read(4);
        YAE_THROW_IF(dts_prefix_ != 1);

        dts_32_30_ = bin.read(3);
        dts_marker1_ = bin.read(1);
        dts_29_15_ = bin.read(15);
        dts_marker2_ = bin.read(1);
        dts_14_00_ = bin.read(15);
        dts_marker3_ = bin.read(1);
      }

      if (escr_flag_)
      {
        escr_reserved_ = bin.read(2);
        escr_base_32_30_ = bin.read(3);
        escr_marker1_ = bin.read(1);
        escr_base_29_15_ = bin.read(15);
        escr_marker2_ = bin.read(1);
        escr_base_14_00_ = bin.read(15);
        escr_marker3_ = bin.read(1);
        escr_extension_ = bin.read(9);
        escr_marker4_ = bin.read(1);
      }

      if (es_rate_flag_)
      {
        es_rate_marker1_ = bin.read(1);
        es_rate_ = bin.read(22);
        es_rate_marker2_ = bin.read(1);
      }

      if (dsm_trick_mode_flag_)
      {
        mode_.trick_mode_control_ = bin.read(3);
        if (mode_.trick_mode_control_ == TRICK_MODE_FAST_FORWARD ||
            mode_.trick_mode_control_ == TRICK_MODE_FAST_REVERSE)
        {
          fast_.field_id_ = bin.read(2);
          fast_.intra_slice_refresh_ = bin.read(1);
          fast_.frequency_truncation_ = bin.read(2);
        }
        else if (mode_.trick_mode_control_ == TRICK_MODE_SLOW_MOTION ||
                 mode_.trick_mode_control_ == TRICK_MODE_SLOW_REVERSE)
        {
          slow_.rep_cntrl_ = bin.read(5);
        }
        else if (mode_.trick_mode_control_ == TRICK_MODE_FREEZE_FRAME)
        {
          freeze_.field_id_ = bin.read(2);
          freeze_.reserved_ = bin.read(3);
        }
        else
        {
          mode_.reserved_ = bin.read(5);
        }
      }

      if (additional_copy_info_flag_)
      {
        additional_copy_marker_ = bin.read(1);
        additional_copy_info_ = bin.read(7);
      }

      if (pes_crc_flag_)
      {
        previous_pes_packet_crc_ = bin.read(16);
      }

      if (pes_extension_flag_)
      {
        extension_ = Extension();

        Extension & ext = *extension_;
        ext.load(bin);
      }

      // load stuffing:
      std::size_t end_pos = bin.position();
      std::size_t consumed = end_pos - start_pos;
      YAE_THROW_IF((consumed & 0x7) != 0);

      std::size_t consumed_bytes = consumed >> 3;
      stuffing_ = bin.read_bytes(pes_header_data_length_ - consumed_bytes);
    }


    //----------------------------------------------------------------
    // PESPacket::PES::Extension::Extension
    //
    PESPacket::PES::Extension::Extension():
      pes_private_data_flag_(0),
      pack_header_field_flag_(0),
      program_packet_sequence_counter_flag_(0),
      pstd_buffer_flag_(0),
      reserved_(0),
      pes_extension_flag_2_(0),
      pack_field_length_(0),
      program_packet_sequence_counter_marker_(0),
      program_packet_sequence_counter_(0),
      mpeg1_mpeg2_identifier_marker_(0),
      mpeg1_mpeg2_identifier_(0),
      original_stuff_length_(0),
      pstd_const_01_(0),
      pstd_buffer_scale_(0),
      pstd_buffer_size_(0)
    {}

    //----------------------------------------------------------------
    // PESPacket::PES::Extension::load
    //
    void
    PESPacket::PES::Extension::load(IBitstream & bin)
    {
      pes_private_data_flag_ = bin.read(1);
      pack_header_field_flag_ = bin.read(1);
      program_packet_sequence_counter_flag_ = bin.read(1);
      pstd_buffer_flag_ = bin.read(1);
      reserved_ = bin.read(3);
      pes_extension_flag_2_ = bin.read(1);

      if (pes_private_data_flag_)
      {
        pes_private_data_ = bin.read_bytes(16);
      }

      if (pack_header_field_flag_)
      {
        pack_field_length_ = bin.read(8);

        pack_header_ = PackHeader();
        PackHeader & pack_header = *pack_header_;
        pack_header.load(bin);
      }

      if (program_packet_sequence_counter_flag_)
      {
        program_packet_sequence_counter_marker_ = bin.read(1);
        program_packet_sequence_counter_ = bin.read(7);
        mpeg1_mpeg2_identifier_marker_ = bin.read(1);
        mpeg1_mpeg2_identifier_ = bin.read(1);
        original_stuff_length_ = bin.read(6);
      }

      if (pstd_buffer_flag_)
      {
        pstd_const_01_ = bin.read(2);
        pstd_buffer_scale_ = bin.read(1);
        pstd_buffer_size_ = bin.read(13);
      }

      if (pes_extension_flag_2_)
      {
        ext2_ = Ext2();
        Ext2 & ext2 = *ext2_;
        ext2.load(bin);
      }
    }


    //----------------------------------------------------------------
    // PESPacket::PES::Extension::Ext2::Ext2
    //
    PESPacket::PES::Extension::Ext2::Ext2():
      marker_(0),
      pes_extension_field_length_(0),
      tref_reserved_(0),
      tref_32_30_(0),
      tref_marker1_(0),
      tref_29_15_(0),
      tref_marker2_(0),
      tref_14_00_(0),
      tref_marker3_(0)
    {
      stream_id_.extension_flag_ = 0;
      stream_id_.extension_ = 0;
    }

    //----------------------------------------------------------------
    // PESPacket::PES::Extension::Ext2::load
    //
    void
    PESPacket::PES::Extension::Ext2::load(IBitstream & bin)
    {
      marker_ = bin.read(1);
      pes_extension_field_length_ = bin.read(7);

      std::size_t start_pos = bin.position();
      stream_id_.extension_flag_ = bin.read(1);

      if (stream_id_.extension_flag_ == 0)
      {
        stream_id_.extension_ = bin.read(7);
      }
      else
      {
        tref_.stream_id_extension_reserved_ = bin.read(6);
        tref_.extension_flag_ = bin.read(1);

        if (tref_.extension_flag_)
        {
          tref_reserved_ = bin.read(4);
          tref_32_30_ = bin.read(3);
          tref_marker1_ = bin.read(1);
          tref_29_15_ = bin.read(15);
          tref_marker2_ = bin.read(1);
          tref_14_00_ = bin.read(15);
          tref_marker3_ = bin.read(1);
        }
      }

      std::size_t end_pos = bin.position();
      std::size_t consumed = end_pos - start_pos;
      YAE_THROW_IF((consumed & 0x7) != 0);

      std::size_t consumed_bytes = consumed >> 3;
      reserved_ = bin.read_bytes(pes_extension_field_length_ -
                                 consumed_bytes);
    }


    //----------------------------------------------------------------
    // assemble_payload
    //
    yae::Data
    assemble_payload(std::list<TSPacket> & packets)
    {
      std::size_t payload_bytes = 0;
      for (std::list<TSPacket>::const_iterator i = packets.begin();
           i != packets.end(); ++i)
      {
        const TSPacket & pkt = *i;
        if (pkt.payload_)
        {
          payload_bytes += pkt.payload_->size();
        }
      }

      Data payload(payload_bytes);
      unsigned char * dst = payload.get();

      for (std::list<TSPacket>::const_iterator i = packets.begin();
           i != packets.end(); ++i)
      {
        const TSPacket & pkt = *i;
        if (pkt.payload_)
        {
          const unsigned char * src = pkt.payload_->get();
          const std::size_t src_bytes = pkt.payload_->size();
          memcpy(dst, src, src_bytes);
          dst += src_bytes;
        }
      }

      return payload;
    }

    //----------------------------------------------------------------
    // consume
    //
    void
    consume(uint16_t pid, std::list<TSPacket> & packets)
    {
      yae::Data payload = assemble_payload(packets);
      std::string tmp = yae::to_hex(payload.get(),
                                    std::min<std::size_t>(payload.size(), 32),
                                    4);
      yae_dlog("%5i pid   %8i pkts   %8i bytes   %s ...",
               int(pid),
               int(packets.size()),
               int(payload.size()),
               tmp.c_str());

      try
      {
        yae::Bitstream bin(payload);
        PESPacket pes_pkt;
        pes_pkt.load(bin);
      }
      catch (const std::exception & e)
      {
        yae_elog("failed to load PESPacket: %s", e.what());
      }
      catch (...)
      {
        yae_elog("failed to load PESPacket: unexpected exception");
      }
    }

    //----------------------------------------------------------------
    // Context::load
    //
    void
    Context::load(IBitstream & bin, TSPacket & pkt)
    {
      std::size_t start_pos = bin.position();
      pkt.load(bin, *this);
      std::size_t end_pos = bin.position();
      YAE_THROW_IF(((end_pos - start_pos) >> 3) != 188);

      if (pkt.payload_unit_start_indicator_)
      {
        // The payload_unit_start_indicator has normative meaning for
        // transport stream packets that carry PES packets or transport
        // stream section data.
        //
        // When the payload of the transport stream packet contains
        // PES packet data, the payload_unit_start_indicator has the
        // following significance:
        //
        // - a '1' indicates that the payload of this transport stream packet
        //   will commence with the first byte of a PES packet
        //
        // - a '0' indicates no PES packet shall start in this transport
        //   stream packet.
        //
        // If the payload_unit_start_indicator is set to '1', then one
        // and only one PES packet starts in this transport stream packet.
        // This also applies to private streams of stream_type 6.
        //
        // When the payload of the transport stream packet contains transport
        // stream section data, the payload_unit_start_indicator has the
        // following significance:
        //
        // - if the transport stream packet carries the first byte of a section,
        //   the payload_unit_start_indicator value shall be '1', indicating
        //   that the first byte of the payload of this transport stream
        //   packet carries the pointer_field.
        // - if the transport stream packet does not carry the first byte of
        //   a section, the payload_unit_start_indicator value shall be '0',
        //   indicating that there is no pointer_field in the payload.
        // This also applies to private streams of stream_type 5.
        //
        // For null packets the payload_unit_start_indicator shall be
        // set to '0'.
        //
        // The meaning of this bit for transport stream packets carrying
        // only private data is not defined.

        std::list<TSPacket> & pes = pes_[pkt.pid_];
#if 1
        if (!pes.empty())
        {
          // do something with the previous packets:
          consume(pkt.pid_, pes);
          pes.clear();
        }
#endif
        pes.push_back(pkt);
      }
      else if (yae::has(pes_, uint16_t(pkt.pid_)))
      {
        std::list<TSPacket> & pes = pes_[pkt.pid_];
        if (!pes.empty())
        {
          pes.push_back(pkt);
        }
      }
    }
  }
}
