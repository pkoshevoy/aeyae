// -*- Mode: c++; tab-width: 8; c-basic-offset: 2; indent-tabs-mode: nil -*-
// NOTE: the first line of this file sets up source code indentation rules
// for Emacs; it is also a hint to anyone modifying this file.

// Created   : Sun Jul 20 07:54:26 PM MDT 2025
// Copyright : Pavel Koshevoy
// License   : MIT -- http://www.opensource.org/licenses/mit-license.php

#ifndef YAE_ISO1446_H_
#define YAE_ISO1446_H_

// aeyae:
#include "yae/api/yae_api.h"
#include "yae/api/yae_shared_ptr.h"
#include "yae/utils/yae_data.h"
#include "yae/utils/yae_json.h"
#include "yae/utils/yae_utils.h"

// boost:
#ifndef Q_MOC_RUN
#include <boost/pointer_cast.hpp>
#include <boost/shared_ptr.hpp>
#endif

// standard:
#include <vector>


namespace yae
{

  //----------------------------------------------------------------
  // insert_emulation_prevention_0x03
  //
  // see ISO/IEC 14496-10:2004(E), 7.4.1 NAL unit semantics
  // emulation_prevention_three_byte
  //
  YAE_API yae::Data
  insert_emulation_prevention_0x03(const uint8_t * src, std::size_t src_size);

  //----------------------------------------------------------------
  // remove_emulation_prevention_0x03
  //
  // see ISO/IEC 14496-10:2004(E), 7.4.1 NAL unit semantics
  // emulation_prevention_three_byte
  //
  YAE_API yae::Data
  remove_emulation_prevention_0x03(const uint8_t * src, std::size_t src_size);


  namespace iso14496
  {

    // namespace access:
    using yae::bitstream::ByteAlignment;
    using yae::bitstream::IPayload;
    using yae::Bit;
    using yae::NBit;


    //----------------------------------------------------------------
    // DescrTag
    //
    // from ISO/IEC 14496-1:2010(E), Table 1
    //
    enum DescrTag
    {
      // 0x00 forbidden
      ObjectDescrTag = 0x01,
      InitialObjectDescrTag = 0x02,
      ES_DescrTag = 0x03,
      DecoderConfigDescrTag = 0x04,
      DecSpecificInfoTag = 0x05,
      SLConfigDescrTag = 0x06,
      ContentIdentDescrTag = 0x07,
      SupplContentIdentDescrTag = 0x08,
      IPI_DescrPointerTag = 0x09,
      IPMP_DescrPointerTag = 0x0A,
      IPMP_DescrTag = 0x0B,
      QoS_DescrTag = 0x0C,
      RegistrationDescrTag = 0x0D,
      ES_ID_IncTag = 0x0E,
      ES_ID_RefTag = 0x0F,
      MP4_IOD_Tag = 0x10,
      MP4_OD_Tag = 0x11,
      IPL_DescrPointerRefTag = 0x12,
      ExtensionProfileLevelDescrTag = 0x13,
      ProfileLevelIndicationIndexDescrTag = 0x14,
      // [0x15, 9x3F] Reserved for ISO use
      ContentClassificationDescrTag = 0x40,
      KeyWordDescrTag = 0x41,
      RatingDescrTag = 0x42,
      LanguageDescrTag = 0x43,
      ShortTextualDescrTag = 0x44,
      ExpandedTextualDescrTag = 0x45,
      ContentCreatorNameDescrTag = 0x46,
      ContentCreationDateDescrTag = 0x47,
      OCICreatorNameDescrTag = 0x48,
      OCICreationDateDescrTag = 0x49,
      SmpteCameraPositionDescrTag = 0x4A,
      SegmentDescrTag = 0x4B,
      MediaTimeDescrTag = 0x4C,
      // [0x4D, 0x5F] Reserved for ISO use (OCI extensions)
      IPMP_ToolsListDescrTag = 0x60,
      IPMP_ToolTag = 0x61,
      M4MuxTimingDescrTag = 0x62,
      M4MuxCodeTableDescrTag = 0x63,
      ExtSLConfigDescrTag = 0x64,
      M4MuxBufferSizeDescrTag = 0x65,
      M4MuxIdentDescrTag = 0x66,
      DependencyPointerTag = 0x67,
      DependencyMarkerTag = 0x68,
      M4MuxChannelDescrTag = 0x69,
      // [0x6A, 0xBF] Reserved for ISO use
      // [0xC0, 0xFE] User private
      // 0xFF forbidden
    };

    //----------------------------------------------------------------
    // CommandTag
    //
    // from ISO/IEC 14496-1:2010(E), Table 2
    //
    enum CommandTag
    {
      // 0x00 fobidden
      ObjectDescrUpdateTag = 0x01,
      ObjectDescrRemoveTag = 0x02,
      ES_DescrUpdateTag = 0x03,
      ES_DescrRemoveTag = 0x04,
      IPMP_DescrUpdateTag = 0x05,
      IPMP_DescrRemoveTag = 0x06,
      ES_DescrRemoveRefTag = 0x07,
      ObjectDescrExecuteTag = 0x08,
      // [0x09, 0xBF] Reserved for ISO (command tags)
      // [0xC0, 0xFE] User private
      // 0xFF forbidden
    };

    //----------------------------------------------------------------
    // LengthField
    //
    struct YAE_API LengthField : public IPayload
    {
      LengthField(std::size_t payload_size = 0):
        payload_size_(payload_size)
      {}

      // helper, returns number of bytes required to store length field,
      // breaks up length field into bytes and stores values to the
      // passed in b[4] in reverse order:
      std::size_t split_into_bytes(uint8_t b[4]) const;

      virtual std::size_t size() const;
      virtual void save(IBitstream & bin) const;
      virtual bool load(IBitstream & bin);

      std::size_t payload_size_;
    };

    //----------------------------------------------------------------
    // BaseDescriptor
    //
    struct YAE_API BaseDescriptor
    {
      BaseDescriptor(DescrTag tag): tag_(tag) {}
      virtual ~BaseDescriptor() {}

      // serialize desriptor tag, length field, and payload,
      // and pad to byte-align
      virtual void save(IBitstream & bin) const;
      virtual bool load(IBitstream & bin) = 0;

      // convenience accessor:
      template <typename TPayload>
      TPayload &
      payload() const
      {
        boost::shared_ptr<TPayload> p =
          boost::dynamic_pointer_cast<TPayload>(payload_);
        return *p;
      }

      // tag values are defined in ISO/IEC 14496-1, Table 1
      Bit<8> tag_;

      boost::shared_ptr<IPayload> payload_;
    };

    //----------------------------------------------------------------
    // load_descriptor
    //
    template <typename TPayload>
    bool load_descriptor(BaseDescriptor & desc, IBitstream & bin, uint16_t tag)
    {
      if (!desc.tag_.load(bin) ||
          desc.tag_.data_ != tag)
      {
        return false;
      }

      LengthField payload_bytes;
      if (!payload_bytes.load(bin))
      {
        return false;
      }

      // temporarily override bitstream end position:
      uint64_t payload_end =
        bin.position_plus_nbytes(payload_bytes.payload_size_);
      yae::SetEnd override_bin_end(bin, payload_end);

      desc.payload_.reset(new TPayload());
      if (!desc.payload_->load(bin))
      {
        return false;
      }

      bin.skip_until_byte_aligned();

      if (bin.position() < payload_end)
      {
#ifndef NDEBUG
        yae::Data unparsed = bin.read_bytes_until(payload_end);
        yae_wlog("unparsed descriptor data: %s", unparsed.to_hex().c_str());
#else
        bin.seek(payload_end);
#endif
      }

      return true;
    }

    //----------------------------------------------------------------
    // AudioObjectType
    //
    // GetAudioObjectType()
    // {
    //   audioObjectType = 5 bits, uimsbf
    //   if (audioObjectType == 31) {
    //     audioObjectTypeExt = 6 bits, uimsbf
    //     audioObjectType = 32 + audioObjectTypeExt
    //   }
    //   return audioObjectType;
    // }
    //
    struct YAE_API AudioObjectType : public IPayload
    {
      AudioObjectType(uint8_t audio_object_type = 0);

      virtual void save(IBitstream & bin) const;
      virtual bool load(IBitstream & bin);

      inline uint8_t get() const
      { return (aot_.data_ == 31) ? (32 + ext_.data_) : aot_.data_; }

      inline void set(uint8_t audio_object_type)
      {
        aot_.data_ = (audio_object_type < 31) ? audio_object_type : 31;
        ext_.data_ = (audio_object_type < 31) ? 0 : (audio_object_type - 32);
      }

      Bit<5> aot_;
      Bit<6> ext_;
    };

    //----------------------------------------------------------------
    // SamplingFrequency
    //
    struct YAE_API SamplingFrequency : public IPayload
    {
      virtual void save(IBitstream & bin) const;
      virtual bool load(IBitstream & bin);

      uint32_t get() const;
      void set(uint32_t sampling_frequency);

      Bit<4> index_;

      // if index == 0xF:
      Bit<24> value_;
    };

    //----------------------------------------------------------------
    // SyncExtensionType0x2b7
    //
    struct YAE_API SyncExtensionType0x2b7 : public IPayload
    {
      SyncExtensionType0x2b7();

      virtual void save(IBitstream & bin) const;
      virtual bool load(IBitstream & bin);

      AudioObjectType extensionAudioObjectType_;

      // if extensionAudioObjectType == 5:
      Bit<1, 1> sbrPresentFlag;

      // if sbrPresentFlag:
      SamplingFrequency extensionSamplingFrequency_;

      // if bin.has_enough_bits(12):
      Bit<11> syncExtensionType;

      // if syncExtensionType == 0x548:
      Bit<1> psPresentFlag;

      // if extensionAudioObjectType == 22:
      Bit<4> extensionChannelConfiguration;
    };

    //----------------------------------------------------------------
    // ProgramConfigElement
    //
    struct YAE_API ProgramConfigElement : public IPayload
    {
      virtual void save(IBitstream & bin) const;
      virtual bool load(IBitstream & bin);

      Bit<4> element_instance_tag;
      Bit<2> object_type;
      Bit<4> sampling_frequency_index;
      Bit<4> num_front_channel_elements;
      Bit<4> num_side_channel_elements;
      Bit<4> num_back_channel_elements;
      Bit<2> num_lfe_channel_elements;
      Bit<3> num_assoc_data_elements;
      Bit<4> num_valid_cc_elements;

      Bit<1> mono_mixdown_present;
      // if mono_mixdown_present:
      Bit<4> mono_mixdown_element_number;

      Bit<1> stereo_mixdown_present;
      // if stereo_mixdown_present:
      Bit<4> stereo_mixdown_element_number;

      Bit<1> matrix_mixdown_idx_present;
      // if matrix_mixdown_idx_present:
      Bit<2> matrix_mixdown_idx;
      Bit<1> pseudo_surround_enable;

      struct YAE_API ChannelElement : public IPayload
      {
        virtual void save(IBitstream & bin) const;
        virtual bool load(IBitstream & bin);

        Bit<1> is_cpe;
        Bit<4> tag_select;
      };

      std::vector<ChannelElement> front_element_;
      std::vector<ChannelElement> side_element_;
      std::vector<ChannelElement> back_element_;

      struct YAE_API TagSelect : public Bit<4> {};

      std::vector<TagSelect> lfe_element_;
      std::vector<TagSelect> assoc_data_element_;

      struct YAE_API CCElement : public IPayload
      {
        virtual void save(IBitstream & bin) const;
        virtual bool load(IBitstream & bin);

        Bit<1> is_ind_sw;
        Bit<4> tag_select;
      };

      std::vector<CCElement> cc_element_;

      ByteAlignment byte_alignment_;

      Bit<8> comment_field_bytes_;
      yae::Data comment_field_data_;
    };

    //----------------------------------------------------------------
    // GASpecificConfig
    //
    struct YAE_API GASpecificConfig : public IPayload
    {
      GASpecificConfig(uint8_t samplingFrequencyIndex,
                       uint8_t channelConfiguration,
                       uint8_t audioObjectType);

      virtual void save(IBitstream & bin) const;
      virtual bool load(IBitstream & bin);

      Bit<1> frameLengthFlag;
      Bit<1> dependsOnCoreCoder;

      // if dependsOnCoreCoder:
      Bit<14> codeCoderDelay;

      Bit<1> extensionFlag;

      // if !channelConfiguration:
      boost::shared_ptr<ProgramConfigElement> program_config_element;

      // if audioObjectType is 6 or 20:
      Bit<3> layerNr;

      // if extensionFlag && audioObjectType is 22:
      Bit<5> numOfSubFrame;
      Bit<11> layer_length;

      // if extensionFlag && audioObjectType is 17, 19, 20, or 23:
      Bit<1> aacSectionDataResilienceFlag;
      Bit<1> aacScalefactorDataResilienceFlag;
      Bit<1> aacSpectralDataResilienceFlag;

      // if extensionFlag:
      Bit<1> extensionFlag3;

    protected:
      uint8_t samplingFrequencyIndex_;
      uint8_t channelConfiguration_;
      uint8_t audioObjectType_;
    };

    //----------------------------------------------------------------
    // AudioSpecificConfig
    //
    // ISO/IEC 14496-3:2009(E), 1.6.2.1
    //
    struct YAE_API AudioSpecificConfig : public IPayload
    {
      AudioSpecificConfig():
        audioObjectType_(2)
      {}

      virtual void save(IBitstream & bin) const;
      virtual bool load(IBitstream & bin);

      AudioObjectType audioObjectType_;
      SamplingFrequency samplingFrequency_;
      Bit<4> channelConfiguration;

      SamplingFrequency extensionSamplingFrequency_;
      AudioObjectType extensionAudioObjectType_;
      // if extensionAudioObjectType == 22:
      Bit<4> extensionChannelConfiguration;

      // GASpecificConfig, etc...
      boost::shared_ptr<IPayload> specific_config_;

      Bit<11, 0x2b7> syncExtensionType;
      SyncExtensionType0x2b7 syncExtensionType0x2b7;
    };

    //----------------------------------------------------------------
    // DecoderSpecificInfo
    //
    template <typename TPayload>
    struct DecoderSpecificInfo : public BaseDescriptor
    {
      DecoderSpecificInfo():
        BaseDescriptor(DecSpecificInfoTag)
      {
        payload_.reset(new TPayload());
      }

      virtual bool load(IBitstream & bin)
      { return load_descriptor<TPayload>(*this, bin, 0x05); }
    };

    //----------------------------------------------------------------
    // DecoderConfigDescriptor
    //
    struct YAE_API DecoderConfigDescriptor : public BaseDescriptor
    {
      struct YAE_API Payload : public IPayload
      {
        virtual void save(IBitstream & bin) const;
        virtual bool load(IBitstream & bin);

        // NOTE: 0x40 -- Audio ISO/IEC 14496-3
        Bit<8> objectTypeIndication;

        Bit<6> streamType;
        Bit<1> upStream;
        const Bit<1, 1> reserved;

        Bit<24> bufferSizeDB;
        Bit<32> maxBitrate;
        Bit<32> avgBitrate;

        // AudioSpecificConfig() extends the abstract class
        // DecoderSpecificInfo, as defined in ISO/IEC 14496-1,
        // when DecoderConfigDescriptor.objectTypeIndication
        // refers to streams complying with ISO/IEC 14496-3.
        // In this case the existence of AudioSpecificConfig() is mandatory.
        DecoderSpecificInfo<AudioSpecificConfig> decSpecificInfo;

        // profileLevelIndicationIndexDescriptor [0...255]
      };

      DecoderConfigDescriptor();

      virtual bool load(IBitstream & bin)
      { return load_descriptor<Payload>(*this, bin, 0x04); }
    };

    //----------------------------------------------------------------
    // SLConfigDescriptor
    //
    // ISO/IEC 14496-1:2004 7.3.2.3.1
    //
    struct YAE_API SLConfigDescriptor : public BaseDescriptor
    {
      struct YAE_API Payload : public IPayload
      {
        Payload():
          predefined(0x02)
        {}

        // Predefined field value
        // Description
        // 0x00 Custom
        // 0x01 null SL packet header
        // 0x02 Reserved for use in MP4 files
        // 0x03 – 0xFF Reserved for ISO use
        Bit<8> predefined;

        struct YAE_API if_not_predefined : public IPayload
        {
          Bit<1> useAccessUnitStartFlag;
          Bit<1> useAccessUnitEndFlag;
          Bit<1> useRandomAccessPointFlag;
          Bit<1> hasRandomAccessUnitsOnlyFlag;
          Bit<1> usePaddingFlag;
          Bit<1> useTimeStampsFlag;
          Bit<1> useIdleFlag;
          Bit<1> durationFlag;
          Bit<32> timeStampResolution;
          Bit<32> OCRResolution;
          Bit<8> timeStampLength; // must be ≤ 64
          Bit<8> OCRLength; // must be ≤ 64
          Bit<8> AU_Length; // must be ≤ 32
          Bit<8> instantBitrateLength;
          Bit<4> degradationPriorityLength;
          Bit<5> AU_seqNumLength; // must be ≤ 16
          Bit<5> packetSeqNumLength; // must be ≤ 16
          Bit<2, 0x03> reserved;

          virtual void save(IBitstream & bin) const;
          virtual bool load(IBitstream & bin);
        };

        boost::shared_ptr<if_not_predefined> if_not_predefined_;

        struct YAE_API if_duration_flag : public IPayload
        {
          Bit<32> timeScale;
          Bit<16> accessUnitDuration;
          Bit<16> compositionUnitDuration;

          virtual void save(IBitstream & bin) const;
          virtual bool load(IBitstream & bin);
        };

        boost::shared_ptr<if_duration_flag> if_duration_flag_;

        struct YAE_API if_not_use_timestamps_flag : public IPayload
        {
          // timeStampLength
          NBit startDecodingTimeStamp;
          NBit startCompositionTimeStamp;

          virtual void save(IBitstream & bin) const;
          virtual bool load(IBitstream & bin);
        };

        boost::shared_ptr<if_not_use_timestamps_flag>
        if_not_use_timestamps_flag_;

        // not going to define the unused portion of the payload

        virtual void save(IBitstream & bin) const;
        virtual bool load(IBitstream & bin);
      };

      SLConfigDescriptor():
        BaseDescriptor(SLConfigDescrTag)
      {
        payload_.reset(new Payload());
      }

      virtual bool load(IBitstream & bin)
      { return load_descriptor<Payload>(*this, bin, 0x06); }
    };


    //----------------------------------------------------------------
    // ES_Descriptor
    //
    struct YAE_API ES_Descriptor : public BaseDescriptor
    {
      struct YAE_API Payload : public IPayload
      {
        Bit<16> ES_ID;
        Bit<1> streamDependenceFlag;
        Bit<1> URL_Flag;
        Bit<1> OCRstreamFlag;
        Bit<5> streamPriority;

        struct YAE_API if_stream_dependence_flag : public IPayload
        {
          virtual void save(IBitstream & bin) const
          { dependsOn_ES_ID.save(bin); }

          virtual bool load(IBitstream & bin)
          { return dependsOn_ES_ID.load(bin); }

          Bit<16> dependsOn_ES_ID;
        };

        boost::shared_ptr<if_stream_dependence_flag>
        if_stream_dependence_flag_;

        struct YAE_API if_url_flag : public IPayload
        {
          virtual void save(IBitstream & bin) const;
          virtual bool load(IBitstream & bin);

          Bit<8> URLlength;

          // URLlength entries:
          std::vector<Bit<8> > URLstring;
        };

        boost::shared_ptr<if_url_flag> if_url_flag_;

        struct YAE_API if_ocr_stream_flag : public IPayload
        {
          virtual void save(IBitstream & bin) const
          { OCR_ES_Id.save(bin); }

          virtual bool load(IBitstream & bin)
          { return OCR_ES_Id.load(bin); }

          Bit<16> OCR_ES_Id;
        };

        boost::shared_ptr<if_ocr_stream_flag> if_ocr_stream_flag_;

        DecoderConfigDescriptor decConfigDescr;

        // if (ODProfileLevelIndication == 0x01) then no SL extension.
        // otherwise SL extension is possible.
        SLConfigDescriptor slConfigDescr;

        /* not going to to worry about these until we need them:

           IPI_DescrPointer ipiPtr[0 .. 1];
           IP_IdentificationDataSet ipIDS[0 .. 255];
           IPMP_DescriptorPointer ipmpDescrPtr[0 .. 255];
           LanguageDescriptor langDescr[0 .. 255];
           QoS_Descriptor qosDescr[0 .. 1];
           RegistrationDescriptor regDescr[0 .. 1];
           ExtensionDescriptor extDescr[0 .. 255];
        */

        virtual void save(IBitstream & bin) const;
        virtual bool load(IBitstream & bin);
      };

      ES_Descriptor();

      virtual bool load(IBitstream & bin)
      { return load_descriptor<Payload>(*this, bin, 0x03); }
    };

    //----------------------------------------------------------------
    // NALU
    //
    // unsigned int(16) nalUnitLength;
    // bit(8*nalUnitLength) nalUnit;
    //
    // NOTE: load(...) automatically removes emulation prevention 0x03 bytes,
    // and save(...) automatically inserts emulation prevention 0x03 bytes.
    //
    struct YAE_API NALU : public IPayload
    {
      yae::Data nalu;

      virtual void save(IBitstream & bin) const;
      virtual bool load(IBitstream & bin);
    };

    //----------------------------------------------------------------
    // save_array
    //
    template <typename TPayload>
    void
    save_array(IBitstream & bin,
               const TPayload * payload,
               std::size_t payload_size)
    {
      for (std::size_t i = 0; i < payload_size; i++)
      {
        const TPayload & data = payload[i];
        data.save(bin);
      }
    }

    //----------------------------------------------------------------
    // load_array
    //
    template <typename TPayload>
    bool
    load_array(IBitstream & bin,
               TPayload * payload,
               std::size_t payload_size)
    {
      for (std::size_t i = 0; i < payload_size; i++)
      {
        TPayload & data = payload[i];
        if (!data.load(bin))
        {
          return false;
        }
      }

      return true;
    }

    //----------------------------------------------------------------
    // save_array
    //
    template <std::size_t array_size_bits, typename TPayload>
    void
    save_array(IBitstream & bin, const std::vector<TPayload> & container)
    {
      Bit<array_size_bits> num;
      num.data_ = container.size();
      num.save(bin);

      const TPayload * data = container.empty() ? NULL : &container[0];
      save_array<TPayload>(bin, data, num.data_);
    }

    //----------------------------------------------------------------
    // load_array
    //
    template <std::size_t array_size_bits, typename TPayload>
    bool
    load_array(IBitstream & bin, std::vector<TPayload> & container)
    {
      container.clear();

      Bit<array_size_bits> num;
      if (!num.load(bin))
      {
        return false;
      }

      container.resize(num.data_);
      TPayload * data = container.empty() ? NULL : &container[0];
      return load_array<TPayload>(bin, data, num.data_);
    }

    //----------------------------------------------------------------
    // AVCDecoderConfigurationRecord
    //
    // ISO/IEC 14496-15:2017(E)
    // 5.3.3.1.2
    // AVCDecoderConfigurationRecord
    //
    struct YAE_API AVCDecoderConfigurationRecord : public IPayload
    {
      Bit<8, 0x01> configurationVersion;
      Bit<8> AVCProfileIndication;
      Bit<8> profile_compatibility;
      Bit<8> AVCLevelIndication;
      Bit<6, 0x3F> reserved1;
      Bit<2> lengthSizeMinusOne;
      Bit<3, 0x07> reserved2;

      // Bit<5> numOfSequenceParameterSets;
      std::vector<NALU> sps_;

      // Bit<8> numOfPictureParameterSets;
      std::vector<NALU> pps_;

      //----------------------------------------------------------------
      // if_avc_profile_indication
      //
      // if (AVCProfileIndication == 100 ||
      //     AVCProfileIndication == 110 ||
      //     AVCProfileIndication == 122 ||
      //     AVCProfileIndication == 144)
      struct YAE_API if_AVCProfileIndication
      {
        Bit<6, 0x3F> reserved1;
        Bit<2> chroma_format;
        Bit<5, 0x1F> reserved2;
        Bit<3> bit_depth_luma_minus8;
        Bit<5, 0x1F> reserved3;
        Bit<3> bit_depth_chroma_minus8;

        // Bit<8> numOfSequenceParameterSetExt;
        std::vector<NALU> sps_ext_;

        void save(IBitstream & bin) const;
        bool load(IBitstream & bin);

        void save(Json::Value & json) const;
      };

      boost::shared_ptr<if_AVCProfileIndication> if_AVCProfileIndication_;

      virtual void save(IBitstream & bin) const;
      virtual bool load(IBitstream & bin);

      void save(Json::Value & json) const;
    };


    //----------------------------------------------------------------
    // HEVCDecoderConfigurationRecord
    //
    // ISO/IEC 14496-15:2017(E)
    // 8.3.3.1.2
    // HEVCDecoderConfigurationRecord
    //
    struct YAE_API HEVCDecoderConfigurationRecord : public IPayload
    {
      Bit<8, 0x01> configurationVersion;
      Bit<2> general_profile_space;
      Bit<1> general_tier_flag;
      Bit<5> general_profile_idc;
      Bit<32> general_profile_compatibility_flags;
      Bit<48> general_constraint_indicator_flags;
      Bit<8> general_level_idc;
      Bit<4, 0x0F> reserved1;
      Bit<12> min_spatial_segmentation_idc;
      Bit<6, 0x3F> reserved2;
      Bit<2> parallelismType;
      Bit<6, 0x3F> reserved3;
      Bit<2> chroma_format_idc;
      Bit<5, 0x1F> reserved4;
      Bit<3> bit_depth_luma_minus8;
      Bit<5, 0x1F> reserved5;
      Bit<3> bit_depth_chroma_minus8;
      Bit<16> avgFrameRate;
      Bit<2> constantFrameRate;
      Bit<3> numTemporalLayers;
      Bit<1> temporalIdNested;
      Bit<2> lengthSizeMinusOne;

      struct YAE_API NalArray : public IPayload
      {
        Bit<1> array_completeness;
        Bit<1, 0x00> reserved;
        Bit<6> NAL_unit_type;

        // Bit<16> numNalus;
        std::vector<NALU> nalus;

        virtual void save(IBitstream & bin) const;
        virtual bool load(IBitstream & bin);
      };

      // Bit<8> numOfArrays;
      std::vector<NalArray> nalArrays;

      virtual void save(IBitstream & bin) const;
      virtual bool load(IBitstream & bin);

      bool has_nalu(uint32_t nal_unit_type) const;
      void add_nalu(uint32_t nal_unit_type, const yae::Data & nal);
    };

    //----------------------------------------------------------------
    // DOVIDecoderConfigurationRecord
    //
    // https://professional.dolby.com/siteassets/content-creation/
    // dolby-vision-for-content-creators/
    // dolby_vision_bitstreams_within_the_iso_base_media_file_format_dec2017.pdf
    //
    // from P81_GlassBlowing2_1920x1080@59.94fps_15200kbps_fmp4.mp4
    // dvvC data: 0100102d 10000000 00000000 00000000 00000000 00000000
    //
    struct YAE_API DOVIDecoderConfigurationRecord : public IPayload
    {
      Bit<8, 1> dv_version_major;
      Bit<8, 0> dv_version_minor;

      // Valid values are Profile IDs as defined in Table 1 column 1 of
      // Signaling Dolby Vision Profiles and Levels.
      Bit<7> dv_profile;

      // Valid values are Level IDs as defined in Table 3 of
      // Signaling Dolby Vision Profiles and Levels.
      Bit<6> dv_level;

      Bit<1, 1> rpu_present_flag;
      Bit<1, 0> el_present_flag;
      Bit<1, 1> bl_present_flag;
      Bit<4, 1> dv_bl_signal_compatibility_id;
      Bit<28, 0> reserved1;
      Bit<32, 0> reserved2[4];

      virtual void save(IBitstream & bin) const;
      virtual bool load(IBitstream & bin);
    };

  }


  //----------------------------------------------------------------
  // save
  //
  template <>
  inline void
  save(Json::Value & json, const iso14496::NALU & v)
  { yae::save(json["nalu"], v.nalu); }

  //----------------------------------------------------------------
  // load
  //
  template <>
  inline void
  load(const Json::Value & json, iso14496::NALU & v)
  { yae::load(json["nalu"], v.nalu); }

}

#endif // YAE_ISO1446_H_
