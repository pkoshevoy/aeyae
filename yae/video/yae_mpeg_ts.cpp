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
#if 0
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
