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
using yae::IBitstream;


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
// AudioObjectType::AudioObjectType
//
AudioObjectType::AudioObjectType(uint8_t audio_object_type)
{
  AudioObjectType::set(audio_object_type);
}

//----------------------------------------------------------------
// AudioObjectType::save
//
void
AudioObjectType::save(IBitstream & bin) const
{
  aot_.save(bin);

  if (aot_.data_ == 31)
  {
    ext_.save(bin);
  }
}

//----------------------------------------------------------------
// AudioObjectType::load
//
bool
AudioObjectType::load(IBitstream & bin)
{
  if (!aot_.load(bin))
  {
    return false;
  }

  if (aot_.data_ == 31 && !ext_.load(bin))
  {
    return false;
  }

  return true;
}


//----------------------------------------------------------------
// SamplingFrequency::save
//
void
SamplingFrequency::save(IBitstream & bin) const
{
  index_.save(bin);

  if (index_.data_ == 0xF)
  {
    value_.save(bin);
  }
}

//----------------------------------------------------------------
// SamplingFrequency::load
//
bool
SamplingFrequency::load(IBitstream & bin)
{
  if (!index_.load(bin))
  {
    return false;
  }

  if (index_.data_ == 0xF && !value_.load(bin))
  {
    return false;
  }

  return true;
}

//----------------------------------------------------------------
// sampling_frequency_table
//
static const uint32_t
sampling_frequency_table[16] = {
  96000,
  88200,
  64000,
  48000,
  44100,
  32000,
  24000,
  22050,
  16000,
  12000,
  11025,
  8000,
  7350,
  0, // 0xD reserved
  0, // 0xE reserved
  0, // 0xF escape value
};

//----------------------------------------------------------------
// SamplingFrequency::get
//
uint32_t
SamplingFrequency::get() const
{
  if (index_.data_ == 0xF)
  {
    return value_.data_;
  }

  uint32_t frequency = sampling_frequency_table[index_.data_ & 0xF];
  YAE_ASSERT(frequency);
  return frequency;
}

//----------------------------------------------------------------
// SamplingFrequency::set
//
void
SamplingFrequency::set(uint32_t frequency)
{
  for (uint8_t index = 0; index < 0xD; ++index)
  {
    if (sampling_frequency_table[index] == frequency)
    {
      index_.data_ = index;
      return;
    }
  }

  index_.data_ = 0xF;
  value_.data_ = frequency;
}




//----------------------------------------------------------------
// ProgramConfigElement::ChannelElement::save
//
void
ProgramConfigElement::ChannelElement::save(IBitstream & bin) const
{
  is_cpe.save(bin);
  tag_select.save(bin);
}

//----------------------------------------------------------------
// ProgramConfigElement::ChannelElement::load
//
bool
ProgramConfigElement::ChannelElement::load(IBitstream & bin)
{
  return is_cpe.load(bin) && tag_select.load(bin);
}


//----------------------------------------------------------------
// ProgramConfigElement::CCElement::save
//
void
ProgramConfigElement::CCElement::save(IBitstream & bin) const
{
  is_ind_sw.save(bin);
  tag_select.save(bin);
}

//----------------------------------------------------------------
// ProgramConfigElement::CCElement::load
//
bool
ProgramConfigElement::CCElement::load(IBitstream & bin)
{
  return is_ind_sw.load(bin) && tag_select.load(bin);
}


//----------------------------------------------------------------
// ProgramConfigElement::save
//
void
ProgramConfigElement::save(IBitstream & bin) const
{
  element_instance_tag.save(bin);
  object_type.save(bin);
  sampling_frequency_index.save(bin);
  num_front_channel_elements.save(bin);
  num_side_channel_elements.save(bin);
  num_back_channel_elements.save(bin);
  num_lfe_channel_elements.save(bin);
  num_assoc_data_elements.save(bin);
  num_valid_cc_elements.save(bin);

  mono_mixdown_present.save(bin);
  if (mono_mixdown_present.data_)
  {
    mono_mixdown_element_number.save(bin);
  }

  stereo_mixdown_present.save(bin);
  if (stereo_mixdown_present.data_)
  {
    stereo_mixdown_element_number.save(bin);
  }

  matrix_mixdown_idx_present.save(bin);
  if (matrix_mixdown_idx_present.data_)
  {
    matrix_mixdown_idx.save(bin);
    pseudo_surround_enable.save(bin);
  }

  YAE_THROW_IF(front_element_.size() != num_front_channel_elements.data_);
  for (std::size_t i = 0, n = front_element_.size(); i < n; ++i)
  {
    const ChannelElement & element = front_element_[i];
    element.save(bin);
  }

  YAE_THROW_IF(side_element_.size() != num_side_channel_elements.data_);
  for (std::size_t i = 0, n = side_element_.size(); i < n; ++i)
  {
    const ChannelElement & element = side_element_[i];
    element.save(bin);
  }

  YAE_THROW_IF(back_element_.size() != num_back_channel_elements.data_);
  for (std::size_t i = 0, n = back_element_.size(); i < n; ++i)
  {
    const ChannelElement & element = back_element_[i];
    element.save(bin);
  }

  YAE_THROW_IF(lfe_element_.size() != num_lfe_channel_elements.data_);
  for (std::size_t i = 0, n = lfe_element_.size(); i < n; ++i)
  {
    const TagSelect & tag_select = lfe_element_[i];
    tag_select.save(bin);
  }

  YAE_THROW_IF(assoc_data_element_.size() != num_assoc_data_elements.data_);
  for (std::size_t i = 0, n = assoc_data_element_.size(); i < n; ++i)
  {
    const TagSelect & tag_select = assoc_data_element_[i];
    tag_select.save(bin);
  }

  YAE_THROW_IF(cc_element_.size() != num_valid_cc_elements.data_);
  for (std::size_t i = 0, n = cc_element_.size(); i < n; ++i)
  {
    const CCElement & element = cc_element_[i];
    element.save(bin);
  }

  byte_alignment_.save(bin);

  YAE_THROW_IF(comment_field_bytes_.data_ != comment_field_data_.size());
  comment_field_bytes_.save(bin);
  bin.write_bytes(comment_field_data_.get(),
                  comment_field_data_.size());
}

//----------------------------------------------------------------
// ProgramConfigElement::load
//
bool
ProgramConfigElement::load(IBitstream & bin)
{
  if (!(element_instance_tag.load(bin) &&
        object_type.load(bin) &&
        sampling_frequency_index.load(bin) &&
        num_front_channel_elements.load(bin) &&
        num_side_channel_elements.load(bin) &&
        num_back_channel_elements.load(bin) &&
        num_lfe_channel_elements.load(bin) &&
        num_assoc_data_elements.load(bin) &&
        num_valid_cc_elements.load(bin)))
  {
    return false;
  }

  if (!mono_mixdown_present.load(bin))
  {
    return false;
  }

  if (mono_mixdown_present.data_ &&
      !mono_mixdown_element_number.load(bin))
  {
    return false;
  }

  if (!stereo_mixdown_present.load(bin))
  {
    return false;
  }

  if (stereo_mixdown_present.data_ &&
      !stereo_mixdown_element_number.load(bin))
  {
    return false;
  }

  if (!matrix_mixdown_idx_present.load(bin))
  {
    return false;
  }

  if (matrix_mixdown_idx_present.data_ &&
      !(matrix_mixdown_idx.load(bin) &&
        pseudo_surround_enable.load(bin)))
  {
    return false;
  }

  front_element_.resize(num_front_channel_elements.data_);
  for (std::size_t i = 0, n = front_element_.size(); i < n; ++i)
  {
    ChannelElement & element = front_element_[i];
    if (!element.load(bin))
    {
      return false;
    }
  }

  side_element_.resize(num_side_channel_elements.data_);
  for (std::size_t i = 0, n = side_element_.size(); i < n; ++i)
  {
    ChannelElement & element = side_element_[i];
    if (!element.load(bin))
    {
      return false;
    }
  }

  back_element_.resize(num_back_channel_elements.size());
  for (std::size_t i = 0, n = back_element_.size(); i < n; ++i)
  {
    ChannelElement & element = back_element_[i];
    if (!element.load(bin))
    {
      return false;
    }
  }

  lfe_element_.resize(num_lfe_channel_elements.data_);
  for (std::size_t i = 0, n = lfe_element_.size(); i < n; ++i)
  {
    TagSelect & tag_select = lfe_element_[i];
    if (!tag_select.load(bin))
    {
      return false;
    }
  }

  assoc_data_element_.resize(num_assoc_data_elements.data_);
  for (std::size_t i = 0, n = assoc_data_element_.size(); i < n; ++i)
  {
    TagSelect & tag_select = assoc_data_element_[i];
    if (!tag_select.load(bin))
    {
      return false;
    }
  }

  cc_element_.resize(num_valid_cc_elements.data_);
  for (std::size_t i = 0, n = cc_element_.size(); i < n; ++i)
  {
    CCElement & element = cc_element_[i];
    if (!element.load(bin))
    {
      return false;
    }
  }

  if (!byte_alignment_.load(bin))
  {
    return false;
  }

  if (!comment_field_bytes_.load(bin))
  {
    return false;
  }

  if (!bin.has_enough_bytes(comment_field_bytes_.data_))
  {
    return false;
  }

  comment_field_data_.resize(comment_field_bytes_.data_);
  bin.read_bytes(comment_field_data_.get(),
                 comment_field_data_.size());
  return true;
}


//----------------------------------------------------------------
// GASpecificConfig::GASpecificConfig
//
GASpecificConfig::GASpecificConfig(uint8_t samplingFrequencyIndex,
                                   uint8_t channelConfiguration,
                                   uint8_t audioObjectType):
  samplingFrequencyIndex_(samplingFrequencyIndex),
  channelConfiguration_(channelConfiguration),
  audioObjectType_(audioObjectType)
{}

//----------------------------------------------------------------
// GASpecificConfig::save
//
void
GASpecificConfig::save(IBitstream & bin) const
{
  frameLengthFlag.save(bin);
  dependsOnCoreCoder.save(bin);

  if (dependsOnCoreCoder.data_)
  {
    codeCoderDelay.save(bin);
  }

  extensionFlag.save(bin);

  if (!channelConfiguration_)
  {
    YAE_THROW_IF(!program_config_element);
    program_config_element->save(bin);
  }

  if (audioObjectType_ == 6 ||
      audioObjectType_ == 20)
  {
    layerNr.save(bin);
  }

  if (extensionFlag.data_)
  {
    if (audioObjectType_ == 22)
    {
      numOfSubFrame.save(bin);
      layer_length.save(bin);
    }

    if (audioObjectType_ == 17 ||
        audioObjectType_ == 19 ||
        audioObjectType_ == 20 ||
        audioObjectType_ == 23)
    {
      aacSectionDataResilienceFlag.save(bin);
      aacScalefactorDataResilienceFlag.save(bin);
      aacSpectralDataResilienceFlag.save(bin);
    }

    extensionFlag3.save(bin);
  }
}

//----------------------------------------------------------------
// GASpecificConfig::load
//
bool
GASpecificConfig::load(IBitstream & bin)
{
  if (!(frameLengthFlag.load(bin) &&
        dependsOnCoreCoder.load(bin)))
  {
    return false;
  }

   if (dependsOnCoreCoder.data_ &&
      !codeCoderDelay.load(bin))
  {
    return false;
  }

  if (!extensionFlag.load(bin))
  {
    return false;
  }

  if (!channelConfiguration_)
  {
    program_config_element.reset(new ProgramConfigElement());
    if (!program_config_element->load(bin))
    {
      return false;
    }
  }

  if ((audioObjectType_ == 6 ||
       audioObjectType_ == 20) &&
      !layerNr.load(bin))
  {
    return false;
  }

  if (extensionFlag.data_)
  {
    if (audioObjectType_ == 22 &&
        !(numOfSubFrame.load(bin) &&
          layer_length.load(bin)))
    {
      return false;
    }

    if ((audioObjectType_ == 17 ||
         audioObjectType_ == 19 ||
         audioObjectType_ == 20 ||
         audioObjectType_ == 23) &&
        !(aacSectionDataResilienceFlag.load(bin) &&
          aacScalefactorDataResilienceFlag.load(bin) &&
          aacSpectralDataResilienceFlag.load(bin)))
    {
      return false;
    }

    if (!extensionFlag3.load(bin))
    {
      return false;
    }
  }

  return true;
}


//----------------------------------------------------------------
// SyncExtensionType0x2b7::SyncExtensionType0x2b7
//
SyncExtensionType0x2b7::SyncExtensionType0x2b7():
  extensionAudioObjectType_(5)
{}

//----------------------------------------------------------------
// SyncExtensionType0x2b7::save
//
void
SyncExtensionType0x2b7::save(IBitstream & bin) const
{
  extensionAudioObjectType_.save(bin);

  uint8_t extensionAudioObjectType = extensionAudioObjectType_.get();
  if (extensionAudioObjectType == 5 ||
      extensionAudioObjectType == 22)
  {
    sbrPresentFlag.save(bin);

    if (sbrPresentFlag.data_)
    {
      extensionSamplingFrequency_.save(bin);

      if (extensionAudioObjectType == 5)
      {
        syncExtensionType.save(bin);

        if (syncExtensionType.data_ == 0x548)
        {
          psPresentFlag.save(bin);
        }
      }
    }

    if (extensionAudioObjectType == 22)
    {
      extensionChannelConfiguration.save(bin);
    }
  }
}

//----------------------------------------------------------------
// SyncExtensionType0x2b7::load
//
bool
SyncExtensionType0x2b7::load(IBitstream & bin)
{
  if (!extensionAudioObjectType_.load(bin))
  {
    return false;
  }

  uint8_t extensionAudioObjectType = extensionAudioObjectType_.get();
  if (extensionAudioObjectType == 5 ||
      extensionAudioObjectType == 22)
  {
    if (!sbrPresentFlag.load(bin))
    {
      return false;
    }

    if (sbrPresentFlag.data_)
    {
      if (!extensionSamplingFrequency_.load(bin))
      {
        return false;
      }

      if (extensionAudioObjectType == 5 &&
          bin.has_enough_bits(12))
      {
        if (!syncExtensionType.load(bin))
        {
          return false;
        }

        if (syncExtensionType.data_ == 0x548 &&
            !psPresentFlag.load(bin))
        {
          return false;
        }
      }
    }

    if (extensionAudioObjectType == 22 &&
        !extensionChannelConfiguration.load(bin))
    {
      return false;
    }
  }

  return true;
}


//----------------------------------------------------------------
// AudioSpecificConfig::save
//
void
AudioSpecificConfig::save(IBitstream & bin) const
{
  audioObjectType_.save(bin);
  samplingFrequency_.save(bin);
  channelConfiguration.save(bin);

  YAE_ASSERT(specific_config_);
  if (specific_config_)
  {
    specific_config_->save(bin);
  }

  int audioObjectType = audioObjectType_.get();
  int extensionAudioObjectType = 0;

  if (audioObjectType == 5 ||
      audioObjectType == 29)
  {
    extensionAudioObjectType = 5;
    audioObjectType = extensionAudioObjectType_.get();
  }

  switch (audioObjectType)
  {
    case 17:
    case 19:
    case 20:
    case 21:
    case 22:
    case 23:
    case 24:
    case 25:
    case 26:
    case 27:
    case 39:
      // not implemented:
      YAE_THROW_IF(true);
      break;

    default:
      break;
  }

  if (extensionAudioObjectType != 5)
  {
    syncExtensionType.save(bin);

    if (syncExtensionType.data_ == 0x2b7)
    {
      syncExtensionType0x2b7.save(bin);
    }
  }
}

//----------------------------------------------------------------
// AudioSpecificConfig::load
//
bool
AudioSpecificConfig::load(IBitstream & bin)
{
  if (!(audioObjectType_.load(bin) &&
        samplingFrequency_.load(bin) &&
        channelConfiguration.load(bin)))
  {
    return false;
  }

  // write-only variables:
  int sbrPresentFlag = -1;
  int psPresentFlag = -1;

  int audioObjectType = audioObjectType_.get();
  int extensionAudioObjectType = 0;

  if (audioObjectType == 5 ||
      audioObjectType == 29)
  {
    extensionAudioObjectType = 5;

    // write-only variable:
    sbrPresentFlag = 1;

    if (audioObjectType == 29)
    {
      // write-only variable:
      psPresentFlag = 1;
    }

    if (!extensionSamplingFrequency_.load(bin))
    {
      return false;
    }

    if (!extensionAudioObjectType_.load(bin))
    {
      return false;
    }

    audioObjectType = extensionAudioObjectType_.get();

    if (audioObjectType == 22 &&
        !extensionChannelConfiguration.load(bin))
    {
      return false;
    }
  }

  switch (audioObjectType)
  {
    case 1:
    case 2:
    case 3:
    case 4:
    case 6:
    case 7:
    case 17:
    case 19:
    case 20:
    case 21:
    case 22:
    case 23:
      specific_config_.
        reset(new GASpecificConfig(samplingFrequency_.index_.data_,
                                   channelConfiguration.data_,
                                   audioObjectType));
      break;

#if 0
    case 8:
      CelpSpecificConfig();
      break;

    case 9:
      HvxcSpecificConfig();
      break:

    case 12:
        TTSSpecificConfig();
      break;

    case 13:
    case 14:
    case 15:
    case 16:
      StructuredAudioSpecificConfig();
      break;

    case 24:
      ErrorResilientCelpSpecificConfig();
      break;

    case 25:
      ErrorResilientHvxcSpecificConfig();
      break;

    case 26:
    case 27:
      ParametricSpecificConfig();
      break;

    case 28:
      SSCSpecificConfig();
      break;

    case 30:
      sacPayloadEmbedding;
      SpatialSpecificConfig();
      break;

    case 32:
    case 33:
    case 34:
      MPEG_1_2_SpecificConfig();
      break;

    case 35:
      DSTSpecificConfig();
      break;

    case 36:
      bin.skip(5); // padding bits
      ALSSpecificConfig();
      break;

    case 37:
    case 38:
      SLSSpecificConfig();
      break;

    case 39:
      ELDSpecificConfig(channelConfiguration);
      break:

    case 40:
    case 41:
        SymbolicMusicSpecificConfig();
      break;
#endif

    default:
      // reserved, or not implemented:
      return false;
  }

  if (!(specific_config_ && specific_config_->load(bin)))
  {
    return false;
  }

  switch (audioObjectType)
  {
    case 17:
    case 19:
    case 20:
    case 21:
    case 22:
    case 23:
    case 24:
    case 25:
    case 26:
    case 27:
    case 39:
#if 0
      epConfig; // 2 bits
      if (epConfig == 2 || epConfig == 3)
      {
        ErrorProtectionSpecificConfig();
      }
      if (epConfig == 3)
      {
        directMapping; // 1 bit
        if ( ! directMapping )
        {
          /* tbd */
        }
      }
#endif
      // not implemented:
      return false;

    default:
      break;
  }

  if (extensionAudioObjectType != 5 && bin.has_enough_bits(16))
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
// OpaqueConfig::save
//
void
OpaqueConfig::save(IBitstream & bin) const
{
  bin.write(data_);
}

//----------------------------------------------------------------
// OpaqueConfig::load
//
bool
OpaqueConfig::load(IBitstream & bin)
{
  data_ = bin.read_bytes_until_end();
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

  if (decSpecificInfo)
  {
    decSpecificInfo->save(bin);
  }
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

  if (objectTypeIndication.data_ == 0x40)
  {
    decSpecificInfo.reset(new DecoderSpecificInfo<AudioSpecificConfig>());
  }
  else if (!bin.exhausted())
  {
    // don't care about BIFSv2Config, etc ... just load it as opaque data:
    decSpecificInfo.reset(new DecoderSpecificInfo<OpaqueConfig>());
  }
  else
  {
    decSpecificInfo.reset();
  }

  if (decSpecificInfo && !decSpecificInfo->load(bin))
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
