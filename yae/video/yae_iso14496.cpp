// -*- Mode: c++; tab-width: 8; c-basic-offset: 2; indent-tabs-mode: nil -*-
// NOTE: the first line of this file sets up source code indentation rules
// for Emacs; it is also a hint to anyone modifying this file.

// Created   : Sun Jul 20 07:54:26 PM MDT 2025
// Copyright : Pavel Koshevoy
// License   : MIT -- http://www.opensource.org/licenses/mit-license.php

// aeyae:
#include "yae/api/yae_assert.h"
#include "yae/video/yae_iso14496.h"

// namespace access:
using namespace yae::iso14496;


//----------------------------------------------------------------
// yae::insert_emulation_prevention_0x03
//
// see ISO/IEC 14496-10:2004(E), 7.4.1 NAL unit semantics
// emulation_prevention_three_byte
//
yae::Data
yae::insert_emulation_prevention_0x03(const uint8_t * src,
                                      std::size_t src_size)
{
  yae::Data out((src_size * 3 + 1) / 2 + 1);
  uint8_t * dst = out.get();

  const uint8_t * end = src + src_size;
  uint8_t * dst_0 = dst;
  uint8_t * dst_1 = NULL;
  uint8_t * dst_2 = NULL;

  if (src < end)
  {
    *dst = *src;
    dst_1 = dst;
    ++dst;
    ++src;
  }

  if (src < end)
  {
    *dst = *src;
    dst_2 = dst_1;
    dst_1 = dst;
    ++dst;
    ++src;
  }

  while (src < end)
  {
    if ((dst_2 && *dst_2 == 0x00) &&
        (dst_1 && *dst_1 == 0x00) &&
        (*src <= 0x03))
    {
      *dst = 0x03;
      dst_2 = dst_1;
      dst_1 = dst;
      ++dst;
    }

    *dst = *src;
    dst_2 = dst_1;
    dst_1 = dst;
    ++dst;
    ++src;
  }

  if (dst_2 && *dst_2 == 0x00 &&
      dst_1 && *dst_1 == 0x00)
  {
    // NOTE: when the last byte of the RBSP data is equal to 0x00
    // (which can only occur when the RBSP ends in a cabac_zero_word),
    // a final byte equal to 0x03 is appended to the end of the data.
    *dst = 0x03;
    ++dst;
  }

  out.truncate(dst - out.get());
  return out;
}

//----------------------------------------------------------------
// yae::remove_emulation_prevention_0x03
//
// see ISO/IEC 14496-10:2004(E), 7.4.1 NAL unit semantics
// emulation_prevention_three_byte
//
// convert 0x000003 back to 0x0000
//
yae::Data
yae::remove_emulation_prevention_0x03(const uint8_t * src, std::size_t src_size)
{
  yae::Data out(src_size);
  uint8_t * dst = out.get();

  const uint8_t * end = src + src_size;
  const uint8_t * src_0 = src;
  const uint8_t * src_1 = NULL;
  const uint8_t * src_2 = NULL;

  while (src_0 < end)
  {
    if ((src_0 && *src_0 != 0x03) ||
        (src_1 && *src_1 != 0x00) ||
        (src_2 && *src_2 != 0x00))
    {
      *dst = *src_0;
      ++dst;
    }

    src_2 = src_1;
    src_1 = src_0;
    ++src_0;
  }

  out.truncate(dst - out.get());
  return out;
}


//----------------------------------------------------------------
// LengthField::split_into_bytes
//
// helper, returns number of bytes required to store length field,
// breaks up length field into bytes and stores values to the
// passed in b[4] in reverse order:
//
std::size_t
LengthField::split_into_bytes(uint8_t b[4]) const
{
  std::size_t payload_size = payload_size_;
  std::size_t n = 0;

  do
  {
    if (n > 3)
    {
      throw std::runtime_error("illegal LengthField");
    }

    b[n] = payload_size & 0x7F;
    n++;
    payload_size = payload_size >> 7;
  }
  while (payload_size);
  return n;
}

//----------------------------------------------------------------
// LengthField::size
//
std::size_t
LengthField::size() const
{
  uint8_t b[4] = { 0 };
  std::size_t n = split_into_bytes(b);
  return n * 8;
}

//----------------------------------------------------------------
// LengthField::save
//
void
LengthField::save(IBitstream & bin) const
{
  uint8_t b[4] = { 0 };
  std::size_t n = split_into_bytes(b);

  for (std::size_t i = 1; i <= n; i++)
  {
    uint8_t v = b[n - i];
    if (i != n)
    {
      v |= 0x80;
    }

    bin.write_bits(8, v);
  }
}

//----------------------------------------------------------------
// LengthField::load
//
bool
LengthField::load(IBitstream & bin)
{
  payload_size_ = 0;

  while (bin.has_enough_bits(8))
  {
    std::size_t v = bin.read_bits(8);
    bool more = (v & 0x80) == 0x80;
    payload_size_ <<= 7;
    payload_size_ |= (v & 0x7F);

    if (!more)
    {
      return true;
    }
  }

  return false;
}


//----------------------------------------------------------------
// BaseDescriptor::save
//
void
BaseDescriptor::save(IBitstream & bin) const
{
  tag_.save(bin);

  std::size_t payload_bits = payload_ ? payload_->size() : 0;
  std::size_t payload_bytes = (payload_bits + 7) / 8;
  LengthField(payload_bytes).save(bin);

  if (payload_)
  {
    payload_->save(bin);
  }

  bin.pad_until_byte_aligned();
}


//----------------------------------------------------------------
// SyncExtensionType0x2b7::SyncExtensionType0x2b7
//
SyncExtensionType0x2b7::SyncExtensionType0x2b7():
  audioObjectType(5)
{}

//----------------------------------------------------------------
// SyncExtensionType0x2b7::save
//
void
SyncExtensionType0x2b7::save(IBitstream & bin) const
{
  audioObjectType.save(bin);
  sbrPresentFlag.save(bin);

  if (sbrPresentFlag.data_)
  {
    extSamplingFrequencyIndex.save(bin);
  }
}

//----------------------------------------------------------------
// SyncExtensionType0x2b7::load
//
bool
SyncExtensionType0x2b7::load(IBitstream & bin)
{
  if (!(audioObjectType.load(bin) &&
        sbrPresentFlag.load(bin)))
  {
    return false;
  }

  if (sbrPresentFlag.data_ &&
      !extSamplingFrequencyIndex.load(bin))
  {
    return false;
  }

  return true;
}


//----------------------------------------------------------------
// AudioSpecificConfig_AAC_LC::save
//
void
AudioSpecificConfig_AAC_LC::save(IBitstream & bin) const
{
  audioObjectType.save(bin);
  samplingFrequencyIndex.save(bin);
  channelConfiguration.save(bin);

  // GASpecificConfig
  frameLengthFlag.save(bin);
  dependsOnCoreCoder.save(bin);
  extensionFlag.save(bin);

  if (syncExtensionType.data_ == 0x2b7)
  {
    syncExtensionType.save(bin);
    syncExtensionType0x2b7.save(bin);
  }
}

//----------------------------------------------------------------
// AudioSpecificConfig_AAC_LC::load
//
bool
AudioSpecificConfig_AAC_LC::load(IBitstream & bin)
{
  if (!(audioObjectType.load(bin) &&
        samplingFrequencyIndex.load(bin) &&
        channelConfiguration.load(bin) &&

        // GASpecificConfig
        frameLengthFlag.load(bin) &&
        dependsOnCoreCoder.load(bin) &&
        extensionFlag.load(bin)))
  {
    return false;
  }

  if (bin.has_enough_bits(11))
  {
    if (!syncExtensionType.load(bin))
    {
      return false;
    }

    if (syncExtensionType.data_ == 0x2b7)
    {
      return syncExtensionType0x2b7.load(bin);
    }
  }
  else
  {
    syncExtensionType.data_ = 0;
  }

  return true;
}


//----------------------------------------------------------------
// DecoderConfigDescriptor::Payload::save
//
void
DecoderConfigDescriptor::Payload::save(IBitstream & bin) const
{
  objectTypeIndication.save(bin);
  streamType.save(bin);
  upStream.save(bin);
  reserved.save(bin);
  bufferSizeDB.save(bin);
  maxBitrate.save(bin);
  avgBitrate.save(bin);
  decSpecificInfo.save(bin);
}

//----------------------------------------------------------------
// DecoderConfigDescriptor::Payload::load
//
bool
DecoderConfigDescriptor::Payload::load(IBitstream & bin)
{
  if (!(objectTypeIndication.load(bin) &&
        streamType.load(bin) &&
        upStream.load(bin)))
  {
    return false;
  }

  bin.skip(1); // reserved '1' bit

  if (!(bufferSizeDB.load(bin) &&
        maxBitrate.load(bin) &&
        avgBitrate.load(bin)))
  {
    return false;
  }

  if (objectTypeIndication.data_ == 0x40 &&
      !decSpecificInfo.load(bin))
  {
    return false;
  }

  return true;
}

//----------------------------------------------------------------
// DecoderConfigDescriptor::DecoderConfigDescriptor
//
DecoderConfigDescriptor::DecoderConfigDescriptor():
  BaseDescriptor(DecoderConfigDescrTag)
{
  payload_.reset(new Payload());
}


//----------------------------------------------------------------
// SLConfigDescriptor::Payload::if_not_predefined::save
//
void
SLConfigDescriptor::Payload::if_not_predefined::save(IBitstream & bin) const
{
  useAccessUnitStartFlag.save(bin);
  useAccessUnitEndFlag.save(bin);
  useRandomAccessPointFlag.save(bin);
  hasRandomAccessUnitsOnlyFlag.save(bin);
  usePaddingFlag.save(bin);
  useTimeStampsFlag.save(bin);
  useIdleFlag.save(bin);
  durationFlag.save(bin);
  timeStampResolution.save(bin);
  OCRResolution.save(bin);
  timeStampLength.save(bin);
  OCRLength.save(bin);
  AU_Length.save(bin);
  instantBitrateLength.save(bin);
  degradationPriorityLength.save(bin);
  AU_seqNumLength.save(bin);
  packetSeqNumLength.save(bin);
  reserved.save(bin);
}

//----------------------------------------------------------------
// SLConfigDescriptor::Payload::if_not_predefined::load
//
bool
SLConfigDescriptor::Payload::if_not_predefined::load(IBitstream & bin)
{
  return (useAccessUnitStartFlag.load(bin) &&
          useAccessUnitEndFlag.load(bin) &&
          useRandomAccessPointFlag.load(bin) &&
          hasRandomAccessUnitsOnlyFlag.load(bin) &&
          usePaddingFlag.load(bin) &&
          useTimeStampsFlag.load(bin) &&
          useIdleFlag.load(bin) &&
          durationFlag.load(bin) &&
          timeStampResolution.load(bin) &&
          OCRResolution.load(bin) &&
          timeStampLength.load(bin) &&
          OCRLength.load(bin) &&
          AU_Length.load(bin) &&
          instantBitrateLength.load(bin) &&
          degradationPriorityLength.load(bin) &&
          AU_seqNumLength.load(bin) &&
          packetSeqNumLength.load(bin) &&
          reserved.load(bin));
}

//----------------------------------------------------------------
// SLConfigDescriptor::Payload::if_duration_flag::save
//
void
SLConfigDescriptor::Payload::if_duration_flag::save(IBitstream & bin) const
{
  timeScale.save(bin);
  accessUnitDuration.save(bin);
  compositionUnitDuration.save(bin);
}

//----------------------------------------------------------------
// SLConfigDescriptor::Payload::if_duration_flag::load
//
bool
SLConfigDescriptor::Payload::if_duration_flag::load(IBitstream & bin)
{
  return (timeScale.load(bin) &&
          accessUnitDuration.load(bin) &&
          compositionUnitDuration.load(bin));
}


//----------------------------------------------------------------
// SLConfigDescriptor::Payload::if_not_use_timestamps_flag::save
//
void
SLConfigDescriptor::Payload::
if_not_use_timestamps_flag::save(IBitstream & bin) const
{
  startDecodingTimeStamp.save(bin);
  startCompositionTimeStamp.save(bin);
}

//----------------------------------------------------------------
// SLConfigDescriptor::Payload::if_not_use_timestamps_flag::load
//
bool
SLConfigDescriptor::Payload::
if_not_use_timestamps_flag::load(IBitstream & bin)
{
  return (startDecodingTimeStamp.load(bin) &&
          startCompositionTimeStamp.load(bin));
}


//----------------------------------------------------------------
// SLConfigDescriptor::Payload::save
//
void
SLConfigDescriptor::Payload::save(IBitstream & bin) const
{
  predefined.save(bin);

  if (predefined.data_ == 0)
  {
    YAE_THROW_IF(!if_not_predefined_);
    if_not_predefined_->save(bin);

    if (if_not_predefined_->durationFlag.data_)
    {
      YAE_THROW_IF(!if_duration_flag_);
      if_duration_flag_->save(bin);
    }

    if (!if_not_predefined_->useTimeStampsFlag.data_)
    {
      YAE_THROW_IF(!if_not_use_timestamps_flag_);
      if_not_use_timestamps_flag_->save(bin);
    }
  }
}

//----------------------------------------------------------------
// SLConfigDescriptor::Payload::load
//
bool
SLConfigDescriptor::Payload::load(IBitstream & bin)
{
  if (!predefined.load(bin))
  {
    return false;
  }

  if (predefined.data_ != 0)
  {
    return true;
  }

  if_not_predefined_.reset(new if_not_predefined());
  if (!if_not_predefined_->load(bin))
  {
    return false;
  }

  if (if_not_predefined_->durationFlag.data_)
  {
    if_duration_flag_.reset(new if_duration_flag());
    if (!if_duration_flag_->load(bin))
    {
      return false;
    }
  }

  if (!if_not_predefined_->useTimeStampsFlag.data_)
  {
    if_not_use_timestamps_flag_.reset(new if_not_use_timestamps_flag());
    if_not_use_timestamps_flag & desc =
      *if_not_use_timestamps_flag_;
    desc.startDecodingTimeStamp.nbits_ =
      if_not_predefined_->timeStampLength.data_;
    desc.startCompositionTimeStamp.nbits_ =
      if_not_predefined_->timeStampLength.data_;
    if (!if_not_use_timestamps_flag_->load(bin))
    {
      return false;
    }
  }

  return true;
}

//----------------------------------------------------------------
// ES_Descriptor::Payload::if_url_flag::save
//
void
ES_Descriptor::Payload::if_url_flag::save(IBitstream & bin) const
{
  URLlength.save(bin);
  for (uint64_t i = 0; i < URLlength.data_; i++)
  {
    URLstring[i].save(bin);
  }
}

//----------------------------------------------------------------
// ES_Descriptor::Payload::if_url_flag::load
//
bool
ES_Descriptor::Payload::if_url_flag::load(IBitstream & bin)
{
  if (!URLlength.load(bin))
  {
    return false;
  }

  URLstring.resize(URLlength.data_);
  for (uint64_t i = 0; i < URLlength.data_; i++)
  {
    if (!URLstring[i].load(bin))
    {
      return false;
    }
  }

  return true;
}

//----------------------------------------------------------------
// ES_Descriptor::Payload::save
//
void
ES_Descriptor::Payload::save(IBitstream & bin) const
{
  ES_ID.save(bin);
  streamDependenceFlag.save(bin);
  URL_Flag.save(bin);
  OCRstreamFlag.save(bin);
  streamPriority.save(bin);

  if (if_stream_dependence_flag_)
  {
    if_stream_dependence_flag_->save(bin);
  }

  if (if_url_flag_)
  {
    if_url_flag_->save(bin);
  }

  if (if_ocr_stream_flag_)
  {
    if_ocr_stream_flag_->save(bin);
  }

  decConfigDescr.save(bin);

  slConfigDescr.save(bin);
}

//----------------------------------------------------------------
// ES_Descriptor::Payload::load
//
bool
ES_Descriptor::Payload::load(IBitstream & bin)
{
  if (!(ES_ID.load(bin) &&
        streamDependenceFlag.load(bin) &&
        URL_Flag.load(bin) &&
        OCRstreamFlag.load(bin) &&
        streamPriority.load(bin)))
  {
    return false;
  }

  if (streamDependenceFlag.data_)
  {
    if_stream_dependence_flag_.reset(new if_stream_dependence_flag());
    if (!if_stream_dependence_flag_->load(bin))
    {
      return false;
    }
  }

  if (URL_Flag.data_)
  {
    if_url_flag_.reset(new if_url_flag());
    if (!if_url_flag_->load(bin))
    {
      return false;
    }
  }

  if (OCRstreamFlag.data_)
  {
    if_ocr_stream_flag_.reset(new if_ocr_stream_flag());
    if (!if_ocr_stream_flag_->load(bin))
    {
      return false;
    }
  }

  if (!decConfigDescr.load(bin))
  {
    return false;
  }

  if (!slConfigDescr.load(bin))
  {
    return false;
  }

  return true;
}

//----------------------------------------------------------------
// ES_Descriptor::ES_Descriptor
//
ES_Descriptor::ES_Descriptor():
  BaseDescriptor(ES_DescrTag)
{
  payload_.reset(new Payload());
}


//----------------------------------------------------------------
// EC3IndependentSubstream::save
//
void
EC3IndependentSubstream::save(IBitstream & bin) const
{
  fscod_.save(bin);
  bsid_.save(bin);
  reserved1_.save(bin);
  asvc_.save(bin);
  bsmod_.save(bin);
  acmod_.save(bin);
  lfeon_.save(bin);
  reserved2_.save(bin);
  num_dep_sub_.save(bin);

  if (num_dep_sub_.data_ > 0)
  {
    chan_loc_.save(bin);
  }
  else
  {
    reserved_.save(bin);
  }
}

//----------------------------------------------------------------
// EC3IndependentSubstream::load
//
bool
EC3IndependentSubstream::load(IBitstream & bin)
{
  if (!(fscod_.load(bin) &&
        bsid_.load(bin) &&
        reserved1_.load(bin) &&
        asvc_.load(bin) &&
        bsmod_.load(bin) &&
        acmod_.load(bin) &&
        lfeon_.load(bin) &&
        reserved2_.load(bin) &&
        num_dep_sub_.load(bin)))
  {
    return false;
  }

  if (num_dep_sub_.data_ > 0)
  {
    return chan_loc_.load(bin);
  }

  bin.skip(1); // reserved '0' bit
  return true;
}


//----------------------------------------------------------------
// NALU::save
//
void
NALU::save(IBitstream & bin) const
{
  yae::Data nal =
    yae::insert_emulation_prevention_0x03(nalu.get(), nalu.size());
  Bit<16> size;
  size.data_ = nal.size();
  size.save(bin);
  bin.write(nal);
}

//----------------------------------------------------------------
// NALU::load
//
bool
NALU::load(IBitstream & bin)
{
  Bit<16> size;
  if (!size.load(bin))
  {
    return false;
  }

  yae::Data nal = bin.read_bytes(size.data_);
  nalu = yae::remove_emulation_prevention_0x03(nal.get(), nal.size());
  return true;
}


//----------------------------------------------------------------
// AVCDecoderConfigurationRecord::save
//
void
AVCDecoderConfigurationRecord::save(IBitstream & bin) const
{
  YAE_ASSERT(lengthSizeMinusOne.data_ > 0);

  configurationVersion.save(bin);
  AVCProfileIndication.save(bin);
  profile_compatibility.save(bin);
  AVCLevelIndication.save(bin);
  reserved1.save(bin);
  lengthSizeMinusOne.save(bin);
  reserved2.save(bin);

  // Bit<5> numOfSequenceParameterSets;
  save_array<5>(bin, sps_);

  // Bit<8> numOfPictureParameterSets;
  save_array<8>(bin, pps_);

  if ((AVCProfileIndication.data_ == 100 ||
       AVCProfileIndication.data_ == 110 ||
       AVCProfileIndication.data_ == 122 ||
       AVCProfileIndication.data_ == 144) &&
      if_AVCProfileIndication_)
  {
    if_AVCProfileIndication_->save(bin);
  }
}


//----------------------------------------------------------------
// AVCDecoderConfigurationRecord::save
//
void
AVCDecoderConfigurationRecord::save(Json::Value & json) const
{
  YAE_ASSERT(lengthSizeMinusOne.data_ > 0);

  yae::save(json["configurationVersion"], configurationVersion.data_);
  yae::save(json["AVCProfileIndication"], AVCProfileIndication.data_);
  yae::save(json["profile_compatibility"], profile_compatibility.data_);
  yae::save(json["AVCLevelIndication"], AVCLevelIndication.data_);
  yae::save(json["reserved1"], reserved1.data_);
  yae::save(json["lengthSizeMinusOne"], lengthSizeMinusOne.data_);
  yae::save(json["reserved2"], reserved2.data_);

  yae::save(json["sps"], sps_);
  yae::save(json["pps"], pps_);

  if ((AVCProfileIndication.data_ == 100 ||
       AVCProfileIndication.data_ == 110 ||
       AVCProfileIndication.data_ == 122 ||
       AVCProfileIndication.data_ == 144) &&
      if_AVCProfileIndication_)
  {
    if_AVCProfileIndication_->save(json["if_AVCProfileIndication_"]);
  }
}

//----------------------------------------------------------------
// AVCDecoderConfigurationRecord::load
//
bool
AVCDecoderConfigurationRecord::load(IBitstream & bin)
{
  if (!(configurationVersion.load(bin) &&
        AVCProfileIndication.load(bin) &&
        profile_compatibility.load(bin) &&
        AVCLevelIndication.load(bin) &&
        reserved1.load(bin) &&
        lengthSizeMinusOne.load(bin) &&
        reserved2.load(bin)))
  {
    return false;
  }

  // Bit<5> numOfSequenceParameterSets;
  if (!load_array<5>(bin, sps_))
  {
    return false;
  }

  // Bit<8> numOfPictureParameterSets;
  if (!load_array<8>(bin, pps_))
  {
    return false;
  }

  if ((AVCProfileIndication.data_ == 100 ||
       AVCProfileIndication.data_ == 110 ||
       AVCProfileIndication.data_ == 122 ||
       AVCProfileIndication.data_ == 144) &&
      bin.has_enough_bytes(4))
  {
    if_AVCProfileIndication_.reset(new if_AVCProfileIndication());
    if (!if_AVCProfileIndication_->load(bin))
    {
      return false;
    }
  }

  return true;
}

//----------------------------------------------------------------
// AVCDecoderConfigurationRecord::if_AVCProfileIndication::save
//
void
AVCDecoderConfigurationRecord::
if_AVCProfileIndication::save(IBitstream & bin) const
{
  reserved1.save(bin);
  chroma_format.save(bin);
  reserved2.save(bin);
  bit_depth_luma_minus8.save(bin);
  reserved3.save(bin);
  bit_depth_chroma_minus8.save(bin);

  // Bit<8> numOfSequenceParameterSetExt;
  save_array<8>(bin, sps_ext_);
}

//----------------------------------------------------------------
// AVCDecoderConfigurationRecord::if_AVCProfileIndication::save
//
void
AVCDecoderConfigurationRecord::
if_AVCProfileIndication::save(Json::Value & json) const
{
  yae::save(json["reserved1"], reserved1.data_);
  yae::save(json["chroma_format"], chroma_format.data_);
  yae::save(json["reserved2"], reserved2.data_);
  yae::save(json["bit_depth_luma_minus8"], bit_depth_luma_minus8.data_);
  yae::save(json["reserved3"], reserved3.data_);
  yae::save(json["bit_depth_chroma_minus8"], bit_depth_chroma_minus8.data_);

  // Bit<8> numOfSequenceParameterSetExt;
  yae::save(json["sps_ext_"], sps_ext_);
}

//----------------------------------------------------------------
// AVCDecoderConfigurationRecord::if_AVCProfileIndication::load
//
bool
AVCDecoderConfigurationRecord::
if_AVCProfileIndication::load(IBitstream & bin)
{
  if (!(reserved1.load(bin) &&
        chroma_format.load(bin) &&
        reserved2.load(bin) &&
        bit_depth_luma_minus8.load(bin) &&
        reserved3.load(bin) &&
        bit_depth_chroma_minus8.load(bin)))
  {
    return false;
  }

  // Bit<8> numOfSequenceParameterSetExt;
  if (!load_array<8>(bin, sps_ext_))
  {
    return false;
  }

  return true;
}


//----------------------------------------------------------------
// HEVCDecoderConfigurationRecord::NalArray::save
//
void
HEVCDecoderConfigurationRecord::NalArray::save(IBitstream & bin) const
{
  array_completeness.save(bin);
  reserved.save(bin);
  NAL_unit_type.save(bin);

  // Bit<16> numNalus
  save_array<16>(bin, nalus);
}

//----------------------------------------------------------------
// HEVCDecoderConfigurationRecord::NalArray::load
//
bool
HEVCDecoderConfigurationRecord::NalArray::load(IBitstream & bin)
{
  if (!(array_completeness.load(bin) &&
        reserved.load(bin) &&
        NAL_unit_type.load(bin)))
  {
    return false;
  }

  YAE_ASSERT(reserved.data_ == 0x00);

  if (!load_array<16>(bin, nalus))
  {
    return false;
  }

  return true;
}

//----------------------------------------------------------------
// HEVCDecoderConfigurationRecord::save
//
void
HEVCDecoderConfigurationRecord::save(IBitstream & bin) const
{
  YAE_ASSERT(configurationVersion.data_ == 1);
  configurationVersion.save(bin);

  general_profile_space.save(bin);
  general_tier_flag.save(bin);
  general_profile_idc.save(bin);
  general_profile_compatibility_flags.save(bin);
  general_constraint_indicator_flags.save(bin);
  general_level_idc.save(bin);

  YAE_ASSERT(reserved1.data_ == 0x0F);
  reserved1.save(bin);

  min_spatial_segmentation_idc.save(bin);

  YAE_ASSERT(reserved2.data_ == 0x3F);
  reserved2.save(bin);

  parallelismType.save(bin);

  YAE_ASSERT(reserved3.data_ == 0x3F);
  reserved3.save(bin);

  chroma_format_idc.save(bin);

  YAE_ASSERT(reserved4.data_ == 0x1F);
  reserved4.save(bin);

  bit_depth_luma_minus8.save(bin);

  YAE_ASSERT(reserved5.data_ == 0x1F);
  reserved5.save(bin);

  bit_depth_chroma_minus8.save(bin);
  avgFrameRate.save(bin);
  constantFrameRate.save(bin);
  numTemporalLayers.save(bin);
  temporalIdNested.save(bin);
  lengthSizeMinusOne.save(bin);

  // Bit<8> numOfArrays;
  save_array<8>(bin, nalArrays);
}

//----------------------------------------------------------------
// HEVCDecoderConfigurationRecord::load
//
bool
HEVCDecoderConfigurationRecord::load(IBitstream & bin)
{
  if (!(configurationVersion.load(bin) &&
        general_profile_space.load(bin) &&
        general_tier_flag.load(bin) &&
        general_profile_idc.load(bin) &&
        general_profile_compatibility_flags.load(bin) &&
        general_constraint_indicator_flags.load(bin) &&
        general_level_idc.load(bin) &&
        reserved1.load(bin) &&
        min_spatial_segmentation_idc.load(bin) &&
        reserved2.load(bin) &&
        parallelismType.load(bin) &&
        reserved3.load(bin) &&
        chroma_format_idc.load(bin) &&
        reserved4.load(bin) &&
        bit_depth_luma_minus8.load(bin) &&
        reserved5.load(bin) &&
        bit_depth_chroma_minus8.load(bin) &&
        avgFrameRate.load(bin) &&
        constantFrameRate.load(bin) &&
        numTemporalLayers.load(bin) &&
        temporalIdNested.load(bin) &&
        lengthSizeMinusOne.load(bin)))
  {
    return false;
  }

  YAE_ASSERT(configurationVersion.data_ == 1);
  YAE_ASSERT(reserved1.data_ == 0x0F);
  YAE_ASSERT(reserved2.data_ == 0x3F);
  YAE_ASSERT(reserved3.data_ == 0x3F);
  YAE_ASSERT(reserved4.data_ == 0x1F);
  YAE_ASSERT(reserved5.data_ == 0x1F);

  // Bit<8> numOfArrays;
  if (!load_array<8>(bin, nalArrays))
  {
    return false;
  }

  return true;
}

//----------------------------------------------------------------
// HEVCDecoderConfigurationRecord::has_nalu
//
bool
HEVCDecoderConfigurationRecord::has_nalu(uint32_t nal_unit_type) const
{
  for (std::size_t i = 0, n = nalArrays.size(); i < n; i++)
  {
    const NalArray & nal_array = nalArrays[i];
    if (nal_array.NAL_unit_type.data_ == nal_unit_type)
    {
      YAE_ASSERT(!nal_array.nalus.empty());
      return true;
    }
  }

  return false;
}

//----------------------------------------------------------------
// HEVCDecoderConfigurationRecord::add_nalu
//
void
HEVCDecoderConfigurationRecord::add_nalu(uint32_t nal_unit_type,
                                         const yae::Data & nal)
{
  YAE_ASSERT(!has_nalu(nal_unit_type));

  NalArray nal_array;
  nal_array.array_completeness.data_ = 0;
  nal_array.NAL_unit_type.data_ = nal_unit_type;

  NALU nalu;
  nalu.nalu = nal.deep_copy();

  nal_array.nalus.push_back(nalu);
  nalArrays.push_back(nal_array);
}


//----------------------------------------------------------------
// DOVIDecoderConfigurationRecord::save
//
void
DOVIDecoderConfigurationRecord::save(IBitstream & bin) const
{
  dv_version_major.save(bin);
  dv_version_minor.save(bin);

  dv_profile.save(bin);
  dv_level.save(bin);

  rpu_present_flag.save(bin);
  el_present_flag.save(bin);
  bl_present_flag.save(bin);
  dv_bl_signal_compatibility_id.save(bin);

  reserved1.save(bin);
  save_array(bin, reserved2, 4);
}

//----------------------------------------------------------------
// DOVIDecoderConfigurationRecord::load
//
bool
DOVIDecoderConfigurationRecord::load(IBitstream & bin)
{
  if (!(dv_version_major.load(bin) &&
        dv_version_minor.load(bin)))
  {
    return false;
  }

  YAE_EXPECT(dv_version_major.data_ == 1);
  YAE_EXPECT(dv_version_minor.data_ == 0);

  if (!(dv_profile.load(bin) &&
        dv_level.load(bin) &&
        rpu_present_flag.load(bin) &&
        el_present_flag.load(bin) &&
        bl_present_flag.load(bin) &&
        dv_bl_signal_compatibility_id.load(bin) &&
        reserved1.load(bin)))
  {
    return false;
  }

  YAE_EXPECT(reserved1.data_ == 0x00);

  if (!load_array(bin, reserved2, 4))
  {
    return false;
  }

  YAE_EXPECT(reserved2[0].data_ == 0x00);
  YAE_EXPECT(reserved2[1].data_ == 0x00);
  YAE_EXPECT(reserved2[2].data_ == 0x00);
  YAE_EXPECT(reserved2[3].data_ == 0x00);

  return true;
}
