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
    // MultipleStringStructure
    //
    struct YAE_API MultipleStringStructure
    {
      MultipleStringStructure();

      void load(IBitstream & bin);

      uint8_t number_strings_;

      struct YAE_API Message
      {
        Message();

        void load(IBitstream & bin);

        uint8_t iso_639_language_code_[3];
        uint8_t number_segments_;

        struct YAE_API Segment
        {
          Segment();

          void load(IBitstream & bin);

          enum CompresionType
          {
            NO_COMPRESSION = 0x00,
            COMPRESSION_ANNEXC_C4_C5 = 0x01,
            COMPRESSION_ANNEXC_C6_C7 = 0x02,
          };

          uint8_t compression_type_;
          uint8_t mode_;
          uint8_t number_bytes_;
          TBufferPtr compressed_string_;
        };

        std::vector<Segment> segment_;
      };

      std::vector<Message> strings_;
    };


    //----------------------------------------------------------------
    // IDescriptor
    //
    struct YAE_API Descriptor
    {
      Descriptor();
      virtual ~Descriptor();

    protected:
      void load_header(IBitstream & bin);
      virtual void load_body(IBitstream & bin);

    public:
      void load(IBitstream & bin);

      uint8_t descriptor_tag_;
      uint8_t descriptor_length_;
    };

    //----------------------------------------------------------------
    // TDescriptorPtr
    //
    typedef yae::shared_ptr<Descriptor> TDescriptorPtr;


    //----------------------------------------------------------------
    // RawDescriptor
    //
    struct YAE_API RawDescriptor : Descriptor
    {
      void load_body(IBitstream & bin);

      TBufferPtr payload_;
    };


    //----------------------------------------------------------------
    // VideoStreamDescriptor
    //
    struct YAE_API VideoStreamDescriptor : Descriptor
    {
      VideoStreamDescriptor();

      void load_body(IBitstream & bin);

      uint8_t multiple_frame_rate_flag_ : 1;
      uint8_t frame_rate_code_ : 4;
      uint8_t mpeg1_only_flag_ : 1;
      uint8_t constrained_parameter_flag_ : 1;
      uint8_t still_picture_flag_ : 1;
      uint8_t profile_and_level_indication_;
      uint8_t chroma_format_ : 2;
      uint8_t frame_rate_extension_flag_ : 1;
      uint8_t reserved_ : 5;
    };


    //----------------------------------------------------------------
    // RegistrationDescriptor
    //
    struct YAE_API RegistrationDescriptor : Descriptor
    {
      RegistrationDescriptor();

      void load_body(IBitstream & bin);

      uint32_t format_identifier_;
      TBufferPtr additional_identification_info_;
    };


    //----------------------------------------------------------------
    // DataStreamAlignmentDescriptor
    //
    struct YAE_API DataStreamAlignmentDescriptor : Descriptor
    {
      DataStreamAlignmentDescriptor();

      void load_body(IBitstream & bin);

      uint8_t alignment_type_;
    };


    //----------------------------------------------------------------
    // ISO639LanguageDescriptor
    //
    struct YAE_API ISO639LanguageDescriptor : Descriptor
    {
      void load_body(IBitstream & bin);

      struct YAE_API Lang
      {
        Lang();

        void load(IBitstream & bin);

        uint8_t iso_639_language_code_[3];
        uint8_t audio_type_;
      };

      std::vector<Lang> lang_;
    };


    //----------------------------------------------------------------
    // AC3AudioDescriptor
    //
    struct YAE_API AC3AudioDescriptor : Descriptor
    {
      AC3AudioDescriptor();

      void load_body(IBitstream & bin);

      uint8_t sample_rate_code_ : 3;
      uint8_t bsid_ : 5;
      uint8_t bit_rate_code_ : 6;
      uint8_t surround_mode_ : 2;
      uint8_t bsmod_ : 3;
      uint8_t num_channels_ : 4;
      uint8_t full_svc_ : 1;

      uint8_t langcod_;
      uint8_t langcod2_;

      union
      {
        uint8_t asvcflags_;

        struct
        {
          uint8_t mainid_ : 3;
          uint8_t priority_ : 2;
          uint8_t reserved_ : 3;
        };
      };

      uint8_t textlen_ : 7;
      uint8_t text_code_ : 1;
      TBufferPtr text_;

      uint8_t language_flag_ : 1;
      uint8_t language2_flag_ : 1;
      uint8_t reserved2_ : 6;
      uint8_t language_[3];
      uint8_t language2_[3];
      TBufferPtr additional_info_;
    };


    //----------------------------------------------------------------
    // CaptionServiceDescriptor
    //
    struct YAE_API CaptionServiceDescriptor : Descriptor
    {
      CaptionServiceDescriptor();

      virtual void load_body(IBitstream & bin);

      uint8_t reserved_ : 3;
      uint8_t number_of_services_ : 5;

      struct YAE_API Service
      {
        Service();

        void load(IBitstream & bin);

        uint8_t language_[3];
        uint8_t digital_cc_ : 1;
        uint8_t reserved1_ : 1;
        union
        {
          uint8_t caption_service_number_ : 6;

          struct
          {
            uint8_t reserved2_ : 5;
            uint8_t line21_field_ : 1;
          };
        };

        uint16_t easy_reader_ : 1;
        uint16_t wide_aspect_ratio_ : 1;
        uint16_t reserved3_ : 14;
      };

      std::vector<Service> service_;
    };


    //----------------------------------------------------------------
    // ContentAdvisoryDescriptor
    //
    struct YAE_API ContentAdvisoryDescriptor : Descriptor
    {
      ContentAdvisoryDescriptor();

      void load_body(IBitstream & bin);

      uint8_t reserved_ : 2;
      uint8_t rating_region_count_ : 6;

      struct YAE_API Region
      {
        Region();

        void load(IBitstream & bin);

        uint8_t rating_region_;
        uint8_t rated_dimensions_;

        struct YAE_API Dimension
        {
          Dimension();

          void load(IBitstream & bin);

          uint8_t rating_dimension_;
          uint8_t reserved_ : 4;
          uint8_t rating_value_ : 4;
        };

        std::vector<Dimension> dimension_;
        uint8_t rating_description_length_;
        MultipleStringStructure rating_description_text_;
      };

      std::vector<Region> region_;
    };


    //----------------------------------------------------------------
    // ExtendedChannelNameDescriptor
    //
    struct YAE_API ExtendedChannelNameDescriptor : Descriptor
    {
      void load_body(IBitstream & bin);

      MultipleStringStructure long_channel_name_text_;
    };


    //----------------------------------------------------------------
    // ServiceLocationDescriptor
    //
    struct YAE_API ServiceLocationDescriptor : Descriptor
    {
      ServiceLocationDescriptor();

      void load_body(IBitstream & bin);

      uint16_t reserved_ : 3;
      uint16_t pcr_pid_ : 13;
      uint8_t number_elements_;

      struct YAE_API Element
      {
        Element();

        void load(IBitstream & bin);

        uint8_t stream_type_;
        uint16_t reserved_ : 3;
        uint16_t elementary_pid_ : 13;
        uint8_t iso_639_languace_code_[3];
      };

      std::vector<Element> element_;
    };


    //----------------------------------------------------------------
    // TimeShiftedServiceDescriptor
    //
    struct YAE_API TimeShiftedServiceDescriptor : Descriptor
    {
      TimeShiftedServiceDescriptor();

      void load_body(IBitstream & bin);

      uint8_t reserved_ : 3;
      uint8_t number_of_services_ : 5;

      struct YAE_API Service
      {
        Service();

        void load(IBitstream & bin);

        uint16_t reserved1_ : 6;
        uint16_t time_shift_ : 10;
        uint32_t reserved2_ : 4;
        uint32_t major_channel_number_ : 10;
        uint32_t minor_channel_number_ : 10;
      };

      std::vector<Service> service_;
    };


    //----------------------------------------------------------------
    // ComponentNameDescriptor
    //
    struct YAE_API ComponentNameDescriptor : Descriptor
    {
      void load_body(IBitstream & bin);

      MultipleStringStructure component_name_string_;
    };


    //----------------------------------------------------------------
    // DCCRequestDescriptor
    //
    // dcc_departing_request_descriptor and
    // dcc_arriving_request_descriptor use the same structure
    //
    struct YAE_API DCCRequestDescriptor : Descriptor
    {
      DCCRequestDescriptor();

      void load_body(IBitstream & bin);

      uint8_t dcc_request_type_;
      uint8_t dcc_request_text_length_;
      MultipleStringStructure dcc_request_text_;
    };


    //----------------------------------------------------------------
    // RedistributionControlDescriptor
    //
    struct YAE_API RedistributionControlDescriptor : Descriptor
    {
      void load_body(IBitstream & bin);

      TBufferPtr rc_information_;
    };


    //----------------------------------------------------------------
    // GenreDescriptor
    //
    struct YAE_API GenreDescriptor : Descriptor
    {
      GenreDescriptor();

      void load_body(IBitstream & bin);

      uint8_t reserved_ : 3;
      uint8_t attribute_count_ : 5;
      TBufferPtr attribute_;
    };


    //----------------------------------------------------------------
    // EAC3AudioStreamDescriptor
    //
    struct YAE_API EAC3AudioStreamDescriptor : Descriptor
    {
      EAC3AudioStreamDescriptor();

      void load_body(IBitstream & bin);

      uint16_t reserved1_ : 1;
      uint16_t bsid_flag_ : 1;
      uint16_t mainid_flag_ : 1;
      uint16_t asvc_flag_ : 1;
      uint16_t mixinfoexists_ : 1;
      uint16_t substream1_flag_ : 1;
      uint16_t substream2_flag_ : 1;
      uint16_t substream3_flag_ : 1;
      uint16_t reserved2_ : 1;
      uint16_t full_service_flag_ : 1;
      uint16_t audio_service_type_ : 3;
      uint16_t number_of_channels_ : 3;

      uint8_t language_flag_ : 1;
      uint8_t language2_flag_ : 1;
      uint8_t reserved3_ : 1;
      uint8_t bsid_ : 5;

      uint8_t reserved4_ : 3;
      uint8_t priority_ : 2;
      uint8_t mainid_ : 3;

      uint8_t asvc_;
      uint8_t substream1_;
      uint8_t substream2_;
      uint8_t substream3_;

      uint8_t language_[3];
      uint8_t language2_[3];

      uint8_t substream1_lang_[3];
      uint8_t substream2_lang_[3];
      uint8_t substream3_lang_[3];

      TBufferPtr additional_info_;
    };


    //----------------------------------------------------------------
    // load_descriptor
    //
    YAE_API TDescriptorPtr
    load_descriptor(IBitstream & bin);


    //----------------------------------------------------------------
    // Section
    //
    struct YAE_API Section
    {
      Section();
      virtual ~Section() {}

    protected:
      void load_header(IBitstream & bin);
      virtual void load_body(IBitstream & bin, std::size_t n_bytes) = 0;

    public:
      void load(IBitstream & bin);

      uint8_t pointer_field_;
      uint8_t table_id_;
      uint16_t section_syntax_indicator_ : 1;
      uint16_t private_indicator_ : 1;
      uint16_t reserved1_ : 2;
      uint16_t section_length_ : 12;

      union
      {
        uint16_t program_number_;
        uint16_t table_id_extension_;
        uint16_t transport_stream_id_;
        uint16_t source_id_;
        uint16_t ett_table_id_extension_;

        struct
        {
          uint16_t reserved_ : 8;
          uint16_t rating_region_ : 8;
        };
      };

      uint64_t reserved2_ : 2;
      uint64_t version_number_ : 5;
      uint64_t current_next_indicator_ : 1;

      uint64_t section_number_ : 8;
      uint64_t last_section_number_ : 8;

      uint32_t crc32_;
    };

    //----------------------------------------------------------------
    // TSectionPtr
    //
    typedef yae::shared_ptr<Section> TSectionPtr;


    //----------------------------------------------------------------
    // ProgramAssociationTable
    //
    struct YAE_API ProgramAssociationTable : Section
    {
      void load_body(IBitstream & bin, std::size_t n_bytes);

      struct YAE_API Program
      {
        Program();

        void load(IBitstream & bin);

        uint32_t program_number_ : 16;
        uint32_t reserved_ : 3;
        uint32_t pid_ : 13;
      };

      std::vector<Program> program_;
    };

    //----------------------------------------------------------------
    // PATSectionPtr
    //
    typedef yae::shared_ptr<ProgramAssociationTable, Section> PATSectionPtr;


    //----------------------------------------------------------------
    // ConditionalAccessTable
    //
    struct YAE_API ConditionalAccessTable : Section
    {
      void load_body(IBitstream & bin, std::size_t n_bytes);

      std::vector<TDescriptorPtr> descriptor_;
    };

    //----------------------------------------------------------------
    // CATSectionPtr
    //
    typedef yae::shared_ptr<ConditionalAccessTable, Section> CATSectionPtr;


    //----------------------------------------------------------------
    // ProgramMapTable
    //
    struct YAE_API ProgramMapTable : Section
    {
      ProgramMapTable();

      void load_body(IBitstream & bin, std::size_t n_bytes);

      uint16_t reserved1_ : 3;
      uint16_t pcr_pid_ : 13;
      uint16_t reserved2_ : 4;
      uint16_t program_info_length_ : 12;
      std::vector<TDescriptorPtr> descriptor_;

      struct YAE_API ElementaryStream
      {
        ElementaryStream();

        void load(IBitstream & bin);

        uint8_t stream_type_;
        uint16_t reserved1_ : 3;
        uint16_t elementary_pid_ : 13;
        uint16_t reserved2_ : 4;
        uint16_t es_info_length_ : 12;
        std::vector<TDescriptorPtr> descriptor_;
      };

      std::vector<ElementaryStream> es_;
    };

    //----------------------------------------------------------------
    // PMTSectionPtr
    //
    typedef yae::shared_ptr<ProgramMapTable, Section> PMTSectionPtr;


    //----------------------------------------------------------------
    // SystemTimeTable
    //
    struct YAE_API SystemTimeTable : Section
    {
      SystemTimeTable();

      void load_body(IBitstream & bin, std::size_t n_bytes);

      uint8_t protocol_version_;
      uint32_t system_time_;
      uint8_t gps_utc_offset_;
      uint16_t daylight_saving_;

      std::vector<TDescriptorPtr> descriptor_;
    };

    //----------------------------------------------------------------
    // STTSectionPtr
    //
    typedef yae::shared_ptr<SystemTimeTable, Section> STTSectionPtr;


    //----------------------------------------------------------------
    // MasterGuideTable
    //
    struct YAE_API MasterGuideTable : Section
    {
      MasterGuideTable();

      void load_body(IBitstream & bin, std::size_t n_bytes);

      uint8_t protocol_version_;
      uint16_t tables_defined_;

      struct YAE_API Table
      {
        Table();

        void load(IBitstream & bin);

        uint16_t table_type_;
        uint16_t reserved1_ : 3;
        uint16_t table_type_pid_ : 13;
        uint8_t reserved2_ : 3;
        uint8_t table_type_version_number_ : 5;
        uint32_t number_bytes_;
        uint16_t reserved3_ : 4;
        uint16_t table_type_descriptors_length_ : 12;

        std::vector<TDescriptorPtr> descriptor_;
      };

      std::vector<Table> table_;

      uint16_t reserved_ : 4;
      uint16_t descriptors_length_ : 12;
      std::vector<TDescriptorPtr> descriptor_;
    };

    //----------------------------------------------------------------
    // MGTSectionPtr
    //
    typedef yae::shared_ptr<MasterGuideTable, Section> MGTSectionPtr;


    //----------------------------------------------------------------
    // VirtualChannelTable
    //
    struct YAE_API VirtualChannelTable : Section
    {
      VirtualChannelTable();

      void load_body(IBitstream & bin, std::size_t n_bytes);

      uint8_t protocol_version_;
      uint8_t num_channels_in_section_;

      struct YAE_API Channel
      {
        Channel();

        void load(IBitstream & bin);

        uint16_t short_name_[7]; // UTF-16
        uint32_t reserved1_ : 4;
        uint32_t major_channel_number_ : 10;
        uint32_t minor_channel_number_ : 10;
        uint32_t modulation_mode_ : 8;
        uint32_t carrier_frequency_;
        uint16_t channel_tsid_;
        uint16_t program_number_;
        uint16_t etm_location_ : 2;
        uint16_t access_controlled_ : 1;
        uint16_t hidden_ : 1;
        uint16_t path_selected_ : 1;
        uint16_t out_of_band_ : 1;
        uint16_t hide_guide_ : 1;
        uint16_t reserved3_ : 3;
        uint16_t service_type_ : 6;
        uint16_t source_id_;
        uint16_t reserved4_ : 6;
        uint16_t descriptors_length_ : 10;

        std::vector<TDescriptorPtr> descriptor_;
      };

      std::vector<Channel> channel_;

      uint16_t reserved_ : 6;
      uint16_t additional_descriptors_length_ : 10;

      std::vector<TDescriptorPtr> additional_descriptor_;
    };

    //----------------------------------------------------------------
    // VCTSectionPtr
    //
    typedef yae::shared_ptr<VirtualChannelTable, Section> VCTSectionPtr;


    //----------------------------------------------------------------
    // RatingRegionTable
    //
    struct YAE_API RatingRegionTable : Section
    {
      RatingRegionTable();

      void load_body(IBitstream & bin, std::size_t n_bytes);

      uint8_t protocol_version_;
      uint8_t rating_region_name_length_;
      MultipleStringStructure rating_region_name_text_;
      uint8_t dimensions_defined_;

      struct YAE_API Dimension
      {
        Dimension();

        void load(IBitstream & bin);

        uint8_t dimension_name_length_;
        MultipleStringStructure dimension_name_text_;
        uint8_t reserved_ : 3;
        uint8_t graduated_scale_ : 1;
        uint8_t values_defined_ : 4;

        struct YAE_API Rating
        {
          Rating();

          void load(IBitstream & bin);

          uint8_t abbrev_rating_value_length_;
          MultipleStringStructure abbrev_rating_value_text_;
          uint8_t rating_value_length_;
          MultipleStringStructure rating_value_text_;
        };

        std::vector<Rating> rating_;
      };

      std::vector<Dimension> dimension_;

      uint16_t reserved_ : 6;
      uint16_t descriptors_length_ : 10;
      std::vector<TDescriptorPtr> descriptor_;
    };

    //----------------------------------------------------------------
    // RRTSectionPtr
    //
    typedef yae::shared_ptr<RatingRegionTable, Section> RRTSectionPtr;


    //----------------------------------------------------------------
    // EventInformationTable
    //
    struct YAE_API EventInformationTable : Section
    {
      EventInformationTable();

      void load_body(IBitstream & bin, std::size_t n_bytes);

      uint8_t protocol_version_;
      uint8_t num_events_in_section_;

      struct YAE_API Event
      {
        Event();

        void load(IBitstream & bin);

        uint16_t reserved1_ : 2;
        uint16_t event_id_ : 14;
        uint32_t start_time_;
        uint32_t reserved2_ : 2;
        uint32_t etm_location_ : 2;
        uint32_t length_in_seconds_ : 20;
        uint32_t title_length_ : 8;

        MultipleStringStructure title_text_;

        uint16_t reserved3_ : 4;
        uint16_t descriptors_length_ : 12;

        std::vector<TDescriptorPtr> descriptor_;
      };

      std::vector<Event> event_;
    };

    //----------------------------------------------------------------
    // EITSectionPtr
    //
    typedef yae::shared_ptr<EventInformationTable, Section> EITSectionPtr;


    //----------------------------------------------------------------
    // ExtendedTextTable
    //
    struct YAE_API ExtendedTextTable : Section
    {
      ExtendedTextTable();

      void load_body(IBitstream & bin, std::size_t n_bytes);

      uint8_t protocol_version_;
      uint32_t etm_id_;
      MultipleStringStructure extended_text_message_;
    };

    //----------------------------------------------------------------
    // ETTSectionPtr
    //
    typedef yae::shared_ptr<ExtendedTextTable, Section> ETTSectionPtr;


    //----------------------------------------------------------------
    // load_section
    //
    YAE_API TSectionPtr
    load_section(IBitstream & bin);


    //----------------------------------------------------------------
    // assemble_payload
    //
    YAE_API yae::Data
    assemble_payload(std::list<TSPacket> & packets);


    //----------------------------------------------------------------
    // Context
    //
    struct YAE_API Context
    {
      void consume(uint16_t pid,
                   std::list<TSPacket> & packets,
                   bool parse = true);

      void load(IBitstream & bin, TSPacket & pkt);

      // packets, indexed by pid:
      std::map<uint16_t, std::list<TSPacket> > pes_;

      // program numbers, indexed by program map pid:
      std::map<uint16_t, uint16_t> pid_pmt_;
      std::map<uint16_t, uint16_t> pid_es_;

      // pid sets for various PSIP sections:
      std::set<uint16_t> pid_tvct_curr_;
      std::set<uint16_t> pid_tvct_next_;
      std::set<uint16_t> pid_cvct_curr_;
      std::set<uint16_t> pid_cvct_next_;
      std::set<uint16_t> pid_channel_ett_;
      std::set<uint16_t> pid_dccsct_;
      std::map<uint16_t, uint8_t> pid_eit_;
      std::map<uint16_t, uint8_t> pid_event_ett_;
      std::map<uint16_t, uint8_t> pid_rrt_;
      std::map<uint16_t, uint8_t> pid_dcct_;
    };

  }
}


#endif // YAE_MPEG_TS_H_
