// -*- Mode: c++; tab-width: 8; c-basic-offset: 2; indent-tabs-mode: nil -*-
// NOTE: the first line of this file sets up source code indentation rules
// for Emacs; it is also a hint to anyone modifying this file.

// Created   : Fri Nov  1 22:32:55 MDT 2019
// Copyright : Pavel Koshevoy
// License   : MIT -- http://www.opensource.org/licenses/mit-license.php

#ifndef YAE_MPEG_TS_H_
#define YAE_MPEG_TS_H_

// system includes:
#include <bitset>
#include <inttypes.h>
#include <list>
#include <map>
#include <string>

// boost:
#ifndef Q_MOC_RUN
#include <boost/lexical_cast.hpp>
#include <boost/thread.hpp>
#endif

// jsoncpp:
#include "json/json.h"

// yae includes:
#include "yae/api/yae_api.h"
#include "yae/utils/yae_data.h"
#include "yae/utils/yae_json.h"
#include "yae/utils/yae_time.h"
#include "yae/utils/yae_timesheet.h"


namespace yae
{

  namespace mpeg_ts
  {

    // forward declarations:
    struct Context;

    //----------------------------------------------------------------
    // AdaptationField
    //
    struct YAE_API AdaptationField
    {
      AdaptationField();

      void load(IBitstream & bin);
      bool is_duplicate_of(const AdaptationField & af) const;

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
        bool is_duplicate_of(const Extension & ext) const;

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

      void load(IBitstream & bin);
      bool is_duplicate_of(const TSPacket & pkt) const;

      inline bool is_null_packet() const
      { return pid_ == 0x1FFF; }

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
    // TLangText
    //
    typedef std::map<std::string, std::string> TLangText;


    //----------------------------------------------------------------
    // MultipleStringStructure
    //
    struct YAE_API MultipleStringStructure
    {
      MultipleStringStructure();

      void load(IBitstream & bin);
      std::string to_str() const;

      // map text by language:
      void get(TLangText & lang_text) const;

      uint8_t number_strings_;

      struct YAE_API Message
      {
        Message();

        void load(IBitstream & bin);
        std::string to_str() const;

        // map text by language code:
        void get(TLangText & lang_text) const;

        uint8_t iso_639_language_code_[3];
        uint8_t number_segments_;

        struct YAE_API Segment
        {
          Segment();

          void load(IBitstream & bin);
          bool to_str(std::string & text) const;

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
    // to_str
    //
    YAE_API std::string
    to_str(const MultipleStringStructure & mss);


    //----------------------------------------------------------------
    // get_text
    //
    YAE_API std::string
    get_text(const TLangText & lang_text, const std::string & lang = "eng");


    //----------------------------------------------------------------
    // Descriptor
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

      virtual void dump(std::ostream & oss) const;

      uint8_t descriptor_tag_;
      uint8_t descriptor_length_;

      // a copy of the bytestream corresponding to the loaded descriptor:
      // yae::Data bin_;
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
    // AudioStreamDescriptor
    //
    struct YAE_API AudioStreamDescriptor : Descriptor
    {
      AudioStreamDescriptor();

      void load_body(IBitstream & bin);

      uint8_t free_format_flag_ : 1;
      uint8_t id_ : 1;
      uint8_t layer_ : 2;
      uint8_t variable_rate_audio_indicator_ : 1;
      uint8_t reserved_ : 3;
    };


    //----------------------------------------------------------------
    // HierarchyDescriptor
    //
    struct YAE_API HierarchyDescriptor : Descriptor
    {
      HierarchyDescriptor();

      void load_body(IBitstream & bin);

      uint32_t reserved1_ : 1;
      uint32_t temporal_scalability_flag_ : 1;
      uint32_t spatial_scalability_flag_ : 1;
      uint32_t quality_scalability_flag_ : 1;
      uint32_t hierarchy_type_ : 4;
      uint32_t reserved2_ : 2;
      uint32_t hierarchy_layer_index_ : 6;
      uint32_t tref_present_flag_ : 1;
      uint32_t reserved3_ : 1;
      uint32_t hierarchy_embedded_layer_index_ : 6;
      uint32_t reserved4_ : 2;
      uint32_t hierarchy_channel_ : 6;
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
    // TargetBackgroundGridDescriptor
    //
    struct YAE_API TargetBackgroundGridDescriptor : Descriptor
    {
      TargetBackgroundGridDescriptor();

      void load_body(IBitstream & bin);

      uint32_t horizontal_size_ : 14;
      uint32_t vertical_size_ : 14;
      uint32_t aspect_ratio_information_ : 4;
    };


    //----------------------------------------------------------------
    // VideoWindowDescriptor
    //
    struct YAE_API VideoWindowDescriptor : Descriptor
    {
      VideoWindowDescriptor();

      void load_body(IBitstream & bin);

      uint32_t horizontal_offset_ : 14;
      uint32_t vertical_offset_ : 14;
      uint32_t window_priority_ : 4;
    };


    //----------------------------------------------------------------
    // CADescriptor
    //
    struct YAE_API CADescriptor : Descriptor
    {
      CADescriptor();

      void load_body(IBitstream & bin);

      uint16_t ca_system_id_;
      uint16_t reserved_ : 3;
      uint16_t ca_pid_ : 13;
      TBufferPtr private_data_;
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
    // SystemClockDescriptor
    //
    struct YAE_API SystemClockDescriptor : Descriptor
    {
      SystemClockDescriptor();

      void load_body(IBitstream & bin);

      uint16_t external_clock_reference_indicator_ : 1;
      uint16_t reserved1_ : 1;
      uint16_t clock_accuracy_integer_ : 6;
      uint16_t clock_accuracy_exponent_ : 3;
      uint16_t reserved2_ : 5;
    };


    //----------------------------------------------------------------
    // MultiplexBufferUtilizationDescriptor
    //
    struct YAE_API MultiplexBufferUtilizationDescriptor : Descriptor
    {
      MultiplexBufferUtilizationDescriptor();

      void load_body(IBitstream & bin);

      uint16_t bound_valid_flag_ : 1;
      uint16_t ltw_offset_lower_bound_ : 15;
      uint16_t reserved_ : 1;
      uint16_t ltw_offset_upper_bound_ : 15;
    };


    //----------------------------------------------------------------
    // CopyrightDescriptor
    //
    struct YAE_API CopyrightDescriptor : Descriptor
    {
      CopyrightDescriptor();

      void load_body(IBitstream & bin);

      uint32_t copyright_identifier_;
      TBufferPtr additional_copyright_info_;
    };


    //----------------------------------------------------------------
    // MaximumBitrateDescriptor
    //
    struct YAE_API MaximumBitrateDescriptor : Descriptor
    {
      MaximumBitrateDescriptor();

      void load_body(IBitstream & bin);

      uint32_t reserved_ : 2;
      uint32_t maximum_bitrate_ : 22;
    };


    //----------------------------------------------------------------
    // PrivateDataIndicatorDescriptor
    //
    struct YAE_API PrivateDataIndicatorDescriptor : Descriptor
    {
      PrivateDataIndicatorDescriptor();

      void load_body(IBitstream & bin);

      uint32_t private_data_indicator_;
    };


    //----------------------------------------------------------------
    // SmoothingBufferDescriptor
    //
    struct YAE_API SmoothingBufferDescriptor : Descriptor
    {
      SmoothingBufferDescriptor();

      void load_body(IBitstream & bin);

      uint32_t reserved1_ : 2;
      uint32_t sb_leak_rate_ : 22;
      uint32_t reserved2_ : 2;
      uint32_t sb_size_ : 22;
    };


    //----------------------------------------------------------------
    // STDDescriptor
    //
    struct YAE_API STDDescriptor : Descriptor
    {
      STDDescriptor();

      void load_body(IBitstream & bin);

      uint8_t reserved_ : 7;
      uint8_t leak_valid_flag_ : 1;
    };


    //----------------------------------------------------------------
    // IBPDescriptor
    //
    struct YAE_API IBPDescriptor : Descriptor
    {
      IBPDescriptor();

      void load_body(IBitstream & bin);

      uint16_t closed_gop_flag_ : 1;
      uint16_t identical_gop_flag_ : 1;
      uint16_t max_gop_length_ : 14;
    };


    //----------------------------------------------------------------
    // MPEG4VideoDescriptor
    //
    struct YAE_API MPEG4VideoDescriptor : Descriptor
    {
      MPEG4VideoDescriptor();

      void load_body(IBitstream & bin);

      uint8_t mpeg4_visual_profile_and_level_;
    };


    //----------------------------------------------------------------
    // MPEG4AudioDescriptor
    //
    struct YAE_API MPEG4AudioDescriptor : Descriptor
    {
      MPEG4AudioDescriptor();

      void load_body(IBitstream & bin);

      uint8_t mpeg4_audio_profile_and_level_;
    };


    //----------------------------------------------------------------
    // IODDescriptor
    //
    struct YAE_API IODDescriptor : Descriptor
    {
      IODDescriptor();

      void load_body(IBitstream & bin);

      uint16_t scope_of_iod_label_ : 8;
      uint16_t iod_label_ : 8;

      // IOD is defined in 8.6.3.1 of ISO/IEC 14496-1:
      TBufferPtr initial_object_descriptor_;
    };


    //----------------------------------------------------------------
    // SLDescriptor
    //
    struct YAE_API SLDescriptor : Descriptor
    {
      SLDescriptor();

      void load_body(IBitstream & bin);

      uint16_t es_id_;
    };


    //----------------------------------------------------------------
    // FMCDescriptor
    //
    struct YAE_API FMCDescriptor : Descriptor
    {
      void load_body(IBitstream & bin);

      struct YAE_API FlexMux
      {
        FlexMux();

        void load(IBitstream & bin);

        uint16_t es_id_;
        uint8_t flex_mux_channel_;
      };

      std::vector<FlexMux> flex_mux_;
    };


    //----------------------------------------------------------------
    // ExternalESIDDescriptor
    //
    struct YAE_API ExternalESIDDescriptor : Descriptor
    {
      ExternalESIDDescriptor();

      void load_body(IBitstream & bin);

      uint16_t external_es_id_;
    };


    //----------------------------------------------------------------
    // MuxcodeDescriptor
    //
    struct YAE_API MuxcodeDescriptor : Descriptor
    {
      void load_body(IBitstream & bin);

      // defined in 11.2.4.3 of ISO/IEC 14496-1
      TBufferPtr mux_code_table_entries_;
    };


    //----------------------------------------------------------------
    // MultiplexBufferDescriptor
    //
    struct YAE_API MultiplexBufferDescriptor : Descriptor
    {
      MultiplexBufferDescriptor();

      void load_body(IBitstream & bin);

      uint64_t mb_buffer_size_ : 24;
      uint64_t tb_leak_rate_ : 24;
    };


    //----------------------------------------------------------------
    // FlexMuxTimingDescriptor
    //
    struct YAE_API FlexMuxTimingDescriptor : Descriptor
    {
      FlexMuxTimingDescriptor();

      void load_body(IBitstream & bin);

      uint64_t fcr_es_id_ : 16;
      uint64_t fcr_resolution_ : 32;
      uint64_t fcr_length_ : 8;
      uint64_t fmx_rate_length_ : 8;
    };


    //----------------------------------------------------------------
    // MPEG2StereoscopicVideoFormatDescriptor
    //
    struct YAE_API MPEG2StereoscopicVideoFormatDescriptor : Descriptor
    {
      MPEG2StereoscopicVideoFormatDescriptor();

      void load_body(IBitstream & bin);

      uint8_t stereoscopic_video_arrangement_type_present_ : 1;
      uint8_t stereoscopic_video_arrangement_type_ : 7;
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

      // virtual:
      void load_body(IBitstream & bin);
      void dump(std::ostream & oss) const;

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

      // virtual:
      void load_body(IBitstream & bin);
      void dump(std::ostream & oss) const;

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
    // TContentAdvisoryDescriptorPtr
    //
    typedef yae::shared_ptr<ContentAdvisoryDescriptor, Descriptor>
    TContentAdvisoryDescriptorPtr;


    //----------------------------------------------------------------
    // ExtendedChannelNameDescriptor
    //
    struct YAE_API ExtendedChannelNameDescriptor : Descriptor
    {
      void load_body(IBitstream & bin);
      void dump(std::ostream & oss) const;

      MultipleStringStructure long_channel_name_text_;
    };


    //----------------------------------------------------------------
    // ServiceLocationDescriptor
    //
    struct YAE_API ServiceLocationDescriptor : Descriptor
    {
      ServiceLocationDescriptor();

      void load_body(IBitstream & bin);
      void dump(std::ostream & oss) const;

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
        uint8_t iso_639_language_code_[3];
      };

      std::vector<Element> element_;
    };

    //----------------------------------------------------------------
    // ServiceLocationDescriptorPtr
    //
    typedef yae::shared_ptr<ServiceLocationDescriptor, Descriptor>
    TServiceLocationDescriptorPtr;


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
      void dump(std::ostream & oss) const;

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
      void dump(std::ostream & oss) const;

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
      virtual void load_header(IBitstream & bin);
      virtual void load_body(IBitstream & bin, std::size_t n_bytes) = 0;

    public:
      virtual void load(IBitstream & bin);

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
    // PrivateSection
    //
    struct YAE_API PrivateSection : Section
    {
    protected:
      virtual void load_header(IBitstream & bin) {}
      virtual void load_body(IBitstream & bin, std::size_t n_bytes) {}

    public:
      virtual void load(IBitstream & bin);

      TBufferPtr private_data_;
    };

    //----------------------------------------------------------------
    // TPrivateSectionPtr
    //
    typedef yae::shared_ptr<PrivateSection, Section> TPrivateSectionPtr;


    //----------------------------------------------------------------
    // TSDescriptionSection
    //
    struct YAE_API TSDescriptionSection : Section
    {
      void load_body(IBitstream & bin, std::size_t n_bytes);

      std::vector<TDescriptorPtr> descriptor_;
    };

    //----------------------------------------------------------------
    // TSDescSectionPtr
    //
    typedef yae::shared_ptr<TSDescriptionSection, Section> TSDescSectionPtr;


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

      uint8_t protocol_version_ : 8;

      // elapsed GPS seconds since UTC 00:00:00 January 6th 1980
      uint32_t system_time_ : 32;

      // to convert to UTC time subtract gps_utc_offset from GPS seconds
      uint8_t gps_utc_offset_ : 8;

      uint16_t daylight_saving_status_ : 1;
      uint16_t daylight_saving_reserved_ : 2;

      // local day of the month on which the transition into or out of
      // daylight saving time is to occur (1 - 31):
      uint16_t daylight_saving_day_of_month_ : 5;

      // local hour at which the transition into or out of
      // daylight saving time to is to occur (0 - 18).
      // This usually occurs at 2am in the US:
      uint16_t daylight_saving_hour_ : 8;

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
      uint32_t etm_id_source_id_ : 16;
      uint32_t etm_id_event_id_ : 15;
      uint32_t etm_id_event_flag_ : 1;
      MultipleStringStructure extended_text_message_;
    };

    //----------------------------------------------------------------
    // ETTSectionPtr
    //
    typedef yae::shared_ptr<ExtendedTextTable, Section> ETTSectionPtr;


    //----------------------------------------------------------------
    // SpliceInfoSection
    //
    struct YAE_API SpliceInfoSection : Section
    {
      SpliceInfoSection();

    protected:
      // vrtual:
      void load_header(IBitstream & bin) {}
      void load_body(IBitstream & bin, std::size_t n_bytes) {}

    public:
      // virtual:
      void load(IBitstream & bin);

      uint64_t protocol_version_ : 8;
      uint64_t encrypted_packet_ : 1;
      uint64_t encryption_algorithm_ : 6;
      uint64_t pts_adjustment_ : 33;
      uint64_t cw_index_ : 8;
      uint32_t tier_ : 12;
      uint32_t splice_command_length_ : 12;
      uint32_t splice_command_type_ : 8;

      //----------------------------------------------------------------
      // BreakDuration
      //
      struct YAE_API BreakDuration
      {
        BreakDuration();

        void load(IBitstream & bin);

        uint64_t auto_return_ : 1;
        uint64_t reserved_ : 6;
        uint64_t duration_ : 33;
      };

      //----------------------------------------------------------------
      // SpliceTime
      //
      struct YAE_API SpliceTime
      {
        SpliceTime();

        void load(IBitstream & bin);

        uint64_t time_specified_flag_ : 1;
        uint64_t reserved_ : 6;
        uint64_t pts_time_ : 33;
      };

      //----------------------------------------------------------------
      // Splice
      //
      struct YAE_API Splice
      {
        Splice();

        void load(IBitstream & bin);

        uint32_t splice_event_id_;
        uint8_t splice_event_cancel_indicator_ : 1;
        uint8_t reserved1_ : 7;

        uint8_t out_of_network_indicator_ : 1;
        uint8_t program_splice_flag_ : 1;
        uint8_t duration_flag_ : 1;
        uint8_t reserved2_ : 5;

        uint32_t utc_splice_time_;

        uint8_t component_count_;

        struct YAE_API Component
        {
          uint8_t component_tag_;
          uint32_t utc_splice_time_;
        };

        std::vector<Component> component_;

        yae::optional<BreakDuration> break_duration_;
        uint32_t unique_program_id_ : 16;
        uint32_t avail_num_ : 8;
        uint32_t avails_expected_ : 8;
      };

      //----------------------------------------------------------------
      // Command
      //
      struct YAE_API Command
      {
        virtual ~Command() {}
        virtual void load(IBitstream & bin, std::size_t nbytes) {}
     };

      //----------------------------------------------------------------
      // SpliceNull
      //
      struct YAE_API SpliceNull : Command {};

      //----------------------------------------------------------------
      // SpliceSchedule
      //
      struct YAE_API SpliceSchedule : Command
      {
        SpliceSchedule();

        void load(IBitstream & bin, std::size_t nbytes);

        uint8_t splice_count_;
        std::vector<Splice> splice_;
      };

      //----------------------------------------------------------------
      // SpliceInsert
      //
      struct YAE_API SpliceInsert : Command
      {
        SpliceInsert();

        void load(IBitstream & bin, std::size_t nbytes);

        uint32_t splice_event_id_;
        uint8_t splice_event_cancel_indicator_ : 1;
        uint8_t reserved1_ : 7;

        uint8_t out_of_network_indicator_ : 1;
        uint8_t program_splice_flag_ : 1;
        uint8_t duration_flag_ : 1;
        uint8_t splice_immediate_flag_ : 1;
        uint8_t reserved2_ : 4;

        yae::optional<SpliceTime> splice_time_;

        uint8_t component_count_;

        struct YAE_API Component
        {
          uint8_t component_tag_;
          yae::optional<SpliceTime> splice_time_;
        };

        std::vector<Component> component_;

        yae::optional<BreakDuration> break_duration_;
        uint32_t unique_program_id_ : 16;
        uint32_t avail_num_ : 8;
        uint32_t avails_expected_ : 8;
      };

      //----------------------------------------------------------------
      // TimeSignal
      //
      struct YAE_API TimeSignal : Command
      {
        void load(IBitstream & bin, std::size_t nbytes);

        SpliceTime splice_time_;
      };

      //----------------------------------------------------------------
      // BandwidthReservation
      //
      struct YAE_API BandwidthReservation : Command {};

      //----------------------------------------------------------------
      // PrivateCommand
      //
      struct YAE_API PrivateCommand : Command
      {
        PrivateCommand();

        void load(IBitstream & bin, std::size_t nbytes);

        uint32_t identifier_;
        TBufferPtr private_;
      };

      yae::shared_ptr<Command> command_;

      uint16_t descriptor_loop_length_;

      //----------------------------------------------------------------
      // SpliceDescriptor
      //
      struct YAE_API SpliceDescriptor
      {
        SpliceDescriptor();

        void load(IBitstream & bin);

        uint16_t splice_descriptor_tag_ : 8;
        uint16_t descriptor_length_ : 8;
        uint32_t identified_;
        TBufferPtr private_;
      };

      std::vector<SpliceDescriptor> descriptor_;

      TBufferPtr alignment_stuffing_;

      uint32_t ecrc32_;
    };

    //----------------------------------------------------------------
    // SpliceInfoSectionPtr
    //
    typedef yae::shared_ptr<SpliceInfoSection, Section> SpliceInfoSectionPtr;


    //----------------------------------------------------------------
    // DSMCCSection
    //
    struct YAE_API DSMCCSection : Section
    {
      void load_body(IBitstream & bin, std::size_t n_bytes);

      TBufferPtr body_;
    };


    //----------------------------------------------------------------
    // DSMCCSectionPtr
    //
    typedef yae::shared_ptr<DSMCCSection, Section> DSMCCSectionPtr;


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
    // channel_number
    //
    inline uint32_t
    channel_number(uint16_t major, uint16_t minor)
    {
      return (uint32_t(major) << 16) | uint32_t(minor);
    }

    //----------------------------------------------------------------
    // channel_major
    //
    inline uint16_t
    channel_major(uint32_t channel_number)
    {
      return uint16_t(channel_number >> 16);
    }

    //----------------------------------------------------------------
    // channel_minor
    //
    inline uint16_t
    channel_minor(uint32_t channel_number)
    {
      return uint16_t(channel_number & 0xFFFF);
    }


    //----------------------------------------------------------------
    // RatingValue
    //
    struct YAE_API RatingValue
    {
      TLangText abbrev_;
      TLangText rating_;
    };

    //----------------------------------------------------------------
    // RatingDimension
    //
    struct YAE_API RatingDimension
    {
      TLangText name_;
      std::vector<RatingValue> values_;
    };

    //----------------------------------------------------------------
    // RatingRegion
    //
    struct YAE_API RatingRegion
    {
      TLangText name_;
      std::vector<RatingDimension> dimensions_;
    };

    //----------------------------------------------------------------
    // ContentAdvisory
    //
    struct YAE_API ContentAdvisory
    {
      TLangText description_;
      std::map<uint16_t, uint8_t> values_; // dimension:value
    };


    //----------------------------------------------------------------
    // get_rating
    //
    std::string
    get_rating(const std::map<uint16_t, RatingRegion> & rrt,
               const uint8_t region,
               const ContentAdvisory & ca,
               const std::string & lang = std::string("eng"));

    //----------------------------------------------------------------
    // get_rating
    //
    YAE_API std::string
    get_rating(const std::map<uint16_t, RatingRegion> & rrt,
               const std::map<uint16_t, ContentAdvisory> & rating,
               const std::string & lang = std::string("eng"));


    //----------------------------------------------------------------
    // ChannelGuide
    //
    struct YAE_API ChannelGuide
    {
      ChannelGuide();

      //----------------------------------------------------------------
      // Item
      //
      struct YAE_API Item
      {
        Item();

        void set_rating(const ContentAdvisoryDescriptor & ca);

        inline std::string
        get_title(const std::string & lang = std::string("eng")) const
        { return get_text(title_, lang); }

        inline uint32_t t1() const
        { return t0_ + dt_; }

        // check whether t is contained in [t0, t1):
        inline bool contains(const uint32_t & t) const
        { return t0_ <= t && t < t1(); }

        inline bool contains(const Item & item) const
        { return t0_ <= item.t0_ && item.t1() <= t1(); }

        inline bool disjoint(const Item & item) const
        { return t0_ > item.t1() || item.t0_ > t1(); }

        inline bool overlaps(const Item & item) const
        { return !disjoint(item); }

        uint16_t source_id_;
        uint16_t event_id_;
        uint32_t t0_; // seconds, GPS time
        uint32_t dt_; // seconds

        // title, indexed by language:
        TLangText title_;

        // this provides a set of indecies into the RRT,
        // indexed by rating region:
        std::map<uint16_t, ContentAdvisory> rating_;
      };

      std::string
      get_description(const Item & item,
                      const std::string & lang = std::string("eng")) const;


      //----------------------------------------------------------------
      // Track
      //
      struct YAE_API Track
      {
        Track():
          stream_type_(0)
        {}

        std::string lang_;
        uint8_t stream_type_;
      };

      std::string name_;
      uint16_t source_id_;
      uint16_t program_number_;
      bool access_controlled_;
      bool hidden_;
      bool hide_guide_;
      uint16_t pcr_pid_;
      std::map<uint16_t, Track> es_;
      std::list<Item> items_;

      // per-event per-language event descriptions:
      std::map<uint16_t, TLangText> event_etm_;

      // per-language channel description:
      TLangText channel_etm_;
    };


    //----------------------------------------------------------------
    // Sections
    //
    struct YAE_API SectionSet
    {
      void set_last_section_number(uint8_t i);
      void set_observed_section(uint8_t i);
      bool is_complete() const;

      std::bitset<0x100> expected_;
      std::bitset<0x100> observed_;
    };


    //----------------------------------------------------------------
    // TableSet
    //
    struct YAE_API TableSet
    {
      void reset();

      void set_expected_table(uint8_t i);
      void set_observed_table(uint8_t i,
                              uint8_t section,
                              uint8_t last_section_number);
      bool is_complete() const;

      std::bitset<0x100> expected_;
      std::map<uint16_t, SectionSet> observed_;
    };


    //----------------------------------------------------------------
    // Bucket
    //
    struct YAE_API Bucket
    {
      Bucket();

      TTime elapsed_time_since_mgt() const;

      //----------------------------------------------------------------
      // VerifyEventDesc
      //
      enum VerifyEventDesc
      {
        kEventDescOptional = 0,
        kEventDescRequired = 1,
      };

      // check whether we have all the expected sections
      // and the extent of the EIT events overlaps
      // given GPS time:
      bool has_epg_for(uint32_t gps_time,
                       VerifyEventDesc verify_etm) const;

      // map major.minor channel number to ChannelGuide:
      std::map<uint32_t, ChannelGuide> guide_;

      // map source_id to major.minor channel number:
      std::map<uint16_t, uint32_t> source_id_to_ch_num_;

      // region rating table, indexed by ration region id:
      std::map<uint16_t, RatingRegion> rrt_;

      // map PID to major.minor channel number:
      std::map<uint16_t, uint32_t> pid_to_ch_num_;

      TTime timestamp_mgt_;
      TableSet vct_table_set_;
      TableSet eit_table_set_;
      TableSet ett_table_set_;
      TableSet rrt_table_set_;
    };


    YAE_API void save(Json::Value & json, const RatingValue & rv);
    YAE_API void load(const Json::Value & json, RatingValue & rv);

    YAE_API void save(Json::Value & json, const RatingDimension & rd);
    YAE_API void load(const Json::Value & json, RatingDimension & rd);

    YAE_API void save(Json::Value & json, const RatingRegion & rr);
    YAE_API void load(const Json::Value & json, RatingRegion & rr);

    YAE_API void save(Json::Value & json, const ContentAdvisory & ca);
    YAE_API void load(const Json::Value & json, ContentAdvisory & ca);

    YAE_API void save(Json::Value & json, const ChannelGuide::Item & item);
    YAE_API void load(const Json::Value & json, ChannelGuide::Item & item);

    YAE_API void save(Json::Value & json, const ChannelGuide::Track & trk);
    YAE_API void load(const Json::Value & json, ChannelGuide::Track & trk);

    YAE_API void save(Json::Value & json, const ChannelGuide & guide);
    YAE_API void load(const Json::Value & json, ChannelGuide & guide);

    YAE_API void save(Json::Value & json, const ChannelGuide & guide);
    YAE_API void load(const Json::Value & json, ChannelGuide & guide);

    YAE_API void save(Json::Value & json, const SectionSet & section_set);
    YAE_API void load(const Json::Value & json, SectionSet & section_set);

    YAE_API void save(Json::Value & json, const TableSet & table_set);
    YAE_API void load(const Json::Value & json, TableSet & table_set);

    YAE_API void save(Json::Value & json, const Bucket & bucket);
    YAE_API void load(const Json::Value & json, Bucket & bucket);


    //----------------------------------------------------------------
    // IPacketHandler
    //
    struct YAE_API IPacketHandler
    {
      //----------------------------------------------------------------
      // Packet
      //
      struct YAE_API Packet
      {
        Packet(uint16_t pid = 0x1FFF, const TBufferPtr & data = TBufferPtr()):
          pid_(pid),
          data_(data)
        {}

        uint16_t pid_;
        TBufferPtr data_;
      };

      virtual ~IPacketHandler() {}

      // NOTE: Context::mutex_ will be locked when this is called:
      virtual void handle(const IPacketHandler::Packet & packet,
                          const Bucket & bucket,
                          uint32_t gps_time) = 0;
    };


    //----------------------------------------------------------------
    // EPG
    //
    struct YAE_API EPG
    {

      //----------------------------------------------------------------
      // Program
      //
      struct YAE_API Program
      {
        bool operator == (const Program & other) const;

        std::string title_;
        std::string description_;
        std::string rating_;
        uint32_t gps_time_;
        uint32_t duration_;

        // local time:
        struct tm tm_;
      };

      //----------------------------------------------------------------
      // Channel
      //
      struct YAE_API Channel
      {
        Channel();

        bool operator == (const Channel & other) const;

        // helper:
        const EPG::Program * find(uint32_t gps_time) const;

        void dump(std::ostream & oss) const;

        uint16_t major_;
        uint16_t minor_;
        std::string name_;
        std::string description_;
        std::list<EPG::Program> programs_;
      };

      // returns true if both channel and program were found:
      bool find(uint32_t ch_num,
                uint32_t gps_time,
                const Channel *& channel,
                const Program *& program) const;

      // calculate [t0, t1) GPS time bounding box over all channels/programs:
      void gps_timespan(uint32_t & gps_t0, uint32_t & gps_t1) const;

      void dump(std::ostream & oss) const;

      // channels, indexed by major.minor channel number:
      std::map<uint32_t, Channel> channels_;
    };

    YAE_API void save(Json::Value & json, const EPG::Program & program);
    YAE_API void load(const Json::Value & json, EPG::Program & program);

    YAE_API void save(Json::Value & json, const EPG::Channel & channel);
    YAE_API void load(const Json::Value & json, EPG::Channel & channel);

    YAE_API void save(Json::Value & json, const EPG & epg);
    YAE_API void load(const Json::Value & json, EPG & epg);


    //----------------------------------------------------------------
    // Context
    //
    struct YAE_API Context
    {
      Context(const std::string & id);

      void push(const TSPacket & pkt);

      void handle(const IPacketHandler::Packet & packet,
                  IPacketHandler & handler) const;

      void get_epg_nolock(const Bucket & bucket,
                          yae::mpeg_ts::EPG & epg,
                          const std::string & lang = std::string("eng"),
                          uint32_t min_gps_time = 0) const;

      void get_epg_now(yae::mpeg_ts::EPG & epg,
                       const std::string & lang = std::string("eng"),
                       uint32_t prev_hours = 24) const;

      // channels, indexed by major.minor:
      void get_channels(std::map<uint32_t, EPG::Channel> & channels,
                        const std::string & lang = std::string("eng")) const;

      bool channel_guide_overlaps(int64_t t) const;

      void save(Json::Value & json) const;
      void load(const Json::Value & json);

      void dump(const std::string & lang = std::string("eng")) const;

      // NOTE: this will lock the mutex while accessing the current Bucket:
      TTime elapsed_time_since_mgt() const;

      enum
      {
#if 1
        kBucketDuration = 60 * 60 * 3, // 3 hours
#else
        kBucketDuration = 60 * 5, // 5 minutes
#endif
      };

      inline std::size_t bucket_index_at(uint32_t gps_time) const
      { return (gps_time / kBucketDuration) % bucket_.size(); }

      inline uint32_t get_current_bucket_index() const
      {
        uint32_t gps_time = this->gps_time_now();
        return this->bucket_index_at(gps_time);
      }

      inline Bucket & get_current_bucket()
      {
        std::size_t ix = this->get_current_bucket_index();
        return bucket_[ix];
      }

      inline const Bucket & get_current_bucket() const
      {
        std::size_t ix = this->get_current_bucket_index();
        return bucket_[ix];
      }

      inline void clear_buffers()
      {
        prev_.clear();
        pes_.clear();
      }

      inline uint16_t lookup_pid_in_pmt(uint16_t pid) const
      { return yae::get<uint16_t, uint16_t>(pid_pmt_, pid, 0); }

      inline uint16_t lookup_pid_in_es(uint16_t pid) const
      { return yae::get<uint16_t, uint16_t>(pid_es_, pid, 0); }

      inline uint16_t lookup_program_id(uint16_t pid) const
      {
        uint16_t program_id = lookup_pid_in_es(pid);
        return program_id ? program_id : lookup_pid_in_pmt(pid);
      }

      // for more human-friendly logging:
      std::string log_prefix_;

      // for profiling:
      mutable yae::Timesheet timesheet_;

    protected:
      // helpers:
      const Bucket & get_epg_bucket_nolock(uint32_t gps_time) const;

      void consume(uint16_t pid,
                   std::list<TSPacket> & packets,
                   bool parse = true);

      // protect against concurrent access:
      mutable boost::mutex mutex_;

      // helpers:
      uint32_t gps_time_now() const;

      void consume_stt(const STTSectionPtr & stt_section);
      void consume_mgt(const MGTSectionPtr & mgt_section);
      void consume_vct(const VirtualChannelTable & vct, uint16_t pid);
      void consume_rrt(const RRTSectionPtr & rrt_section, uint16_t pid);
      void consume_eit(const EventInformationTable & eit, uint16_t pid);
      void consume_ett(const ExtendedTextTable & ett, uint16_t pid);

      void dump(const std::vector<TDescriptorPtr> & descs,
                std::ostream & oss) const;

      std::string id_;

      // 1 days worth of channel guide data in 3 hour long chunks,
      // indexed by an index derived from STT system time:
      //
      std::vector<Bucket> bucket_; // 1 + (24 / 3)

      // unused:
      uint16_t network_pid_;

      // in-stream clock:
      TTime stt_walltime_;
      STTSectionPtr stt_;
      int64_t stt_error_;

      // keep track of previous packet per PID
      // so we can properly handle duplicate packets
      // and detect continuity counter discontinuities:
      std::map<uint16_t, TSPacket> prev_;

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
      std::map<uint16_t, uint8_t> pid_vct_;
      std::map<uint16_t, uint8_t> pid_eit_;
      std::map<uint16_t, uint8_t> pid_event_ett_;
      std::map<uint16_t, uint8_t> pid_rrt_;
      std::map<uint16_t, uint8_t> pid_dcct_;

      // table versions, indexed by pid;
      // VCT and RRT version are stored separately
      // because they can share PID with MGT table:
      std::map<uint16_t, uint16_t> version_;
      std::map<uint16_t, uint16_t> version_vct_;
      std::map<uint16_t, uint16_t> version_rrt_;
    };

  }
}


#endif // YAE_MPEG_TS_H_
