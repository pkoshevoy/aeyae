// -*- Mode: c++; tab-width: 8; c-basic-offset: 2; indent-tabs-mode: nil -*-
// NOTE: the first line of this file sets up source code indentation rules
// for Emacs; it is also a hint to anyone modifying this file.

// Created   : Mon Jul 21 08:26:33 PM MDT 2025
// Copyright : Pavel Koshevoy
// License   : MIT -- http://www.opensource.org/licenses/mit-license.php

// aeyae:
#include "yae/utils/yae_utils.h"
#include "yae/video/yae_iso14496.h"

YAE_DISABLE_DEPRECATION_WARNINGS

// boost:
#include <boost/test/unit_test.hpp>

// namespace access:
using namespace yae::iso14496;

YAE_ENABLE_DEPRECATION_WARNINGS


//----------------------------------------------------------------
// iso14496_insert_emulation_prevention_0x03_all_zeros
//
BOOST_AUTO_TEST_CASE(iso14496_insert_emulation_prevention_0x03_all_zeros)
{
  const char * src_rbsp_hex =
    "000000"
    "000000"
    "000000"
    "000000"
    "000000"
    "000000"
    "000000"
    "000000"
    "000000"
    "000000"
    "000000"
    "000000"
    "000000"
    "000000"
    "000000"
    "000000";

  const char * src_0x03_hex =
    "000003"
    "000003"
    "000003"
    "000003"
    "000003"
    "000003"
    "000003"
    "000003"
    "000003"
    "000003"
    "000003"
    "000003"
    "000003"
    "000003"
    "000003"
    "000003"
    "000003"
    "000003"
    "000003"
    "000003"
    "000003"
    "000003"
    "000003"
    "000003";

  yae::Data src_rbsp = yae::load_hex(src_rbsp_hex);
  yae::Data src_0x03 = yae::load_hex(src_0x03_hex);

  yae::Data out_rbsp =
    yae::remove_emulation_prevention_0x03(src_0x03.get(), src_0x03.size());

  std::string out_rbsp_hex =
    yae::to_hex(out_rbsp.get(), out_rbsp.size());

  BOOST_CHECK(out_rbsp_hex == src_rbsp_hex);

  yae::Data out_0x03 =
    yae::insert_emulation_prevention_0x03(src_rbsp.get(), src_rbsp.size());

  std::string out_0x03_hex =
    yae::to_hex(out_0x03.get(), out_0x03.size());

  BOOST_CHECK(out_0x03_hex == src_0x03_hex);
}

//----------------------------------------------------------------
// iso14496_insert_emulation_prevention_0x03_all_start_codes
//
BOOST_AUTO_TEST_CASE(iso14496_insert_emulation_prevention_0x03_all_start_codes)
{
  const char * src_rbsp_hex =
    "000000"
    "000001"
    "000002"
    "000003"
    "000000"
    "000001"
    "000002"
    "000003"
    "000000"
    "000001"
    "000002"
    "000003"
    "000000"
    "000001"
    "000002"
    "000003";

  const char * src_0x03_hex =
    "00000300"
    "00030001"
    "00000302"
    "00000303"
    "00000300"
    "00030001"
    "00000302"
    "00000303"
    "00000300"
    "00030001"
    "00000302"
    "00000303"
    "00000300"
    "00030001"
    "00000302"
    "00000303";

  yae::Data src_rbsp =
    yae::load_hex(src_rbsp_hex);

  yae::Data out_0x03 =
    yae::insert_emulation_prevention_0x03(src_rbsp.get(), src_rbsp.size());

  std::string out_0x03_hex =
    yae::to_hex(out_0x03.get(), out_0x03.size());

  BOOST_CHECK(out_0x03_hex == src_0x03_hex);
}


//----------------------------------------------------------------
// TwoStrs
//
struct TwoStrs
{
  const char * nal_0x03_;
  const char * nal_rbsp_;
};

//----------------------------------------------------------------
// nals
//
static const TwoStrs nals[] =
{
  {
    "40010c01ffff0140000003000003000003000003005dac09",
    "40010c01ffff01400000000000000000005dac09"
  },
  {
    "4201010140000003000003000003000003005da00280802e1f1396bb9096"
    "4b8c05a80808082000007d20000bb80c00bbca20001499700005265c20",
    "42010101400000000000000000005da00280802e1f1396bb90964b8c05a8"
    "0808082000007d20000bb80c00bbca20001499700005265c20"
  },
  {
    "4e0101030000030280",
    "4e01010300000280"
  },
  {
    "4e0101030000030280",
    "4e01010300000280"
  },
  {
    "0201d15f23f0793e551243533fca19eef66d8fca372f00013d853563f892"
    "a42fc96cae64f096ce2e2479c8f10903f71387e7d80f2dbf8dbbab14c76b"
    "f16e07acde119fc7a49d8ce45ef4fd2064c5b807e2bf817d18a73d17b029"
    "395dfb86babc74d2dbb13f6d70c67438e484745e3ed8b72e6c17ea2333fd"
    "950e41b83bfb9278ec013f6b1c40807f0000030000030000030332b4d751"
    "4eb96efe5e1018",
    "0201d15f23f0793e551243533fca19eef66d8fca372f00013d853563f892"
    "a42fc96cae64f096ce2e2479c8f10903f71387e7d80f2dbf8dbbab14c76b"
    "f16e07acde119fc7a49d8ce45ef4fd2064c5b807e2bf817d18a73d17b029"
    "395dfb86babc74d2dbb13f6d70c67438e484745e3ed8b72e6c17ea2333fd"
    "950e41b83bfb9278ec013f6b1c40807f0000000000000332b4d7514eb96e"
    "fe5e1018"
  },
  {
    "0201d16723f0793e551243533fca19eef66d8fca372f00000ada1298a1ab"
    "e7ad688332430163a97011b4f16f577084478ec3aba780000003000006ca"
    "529f80d65bc548133860",
    "0201d16723f0793e551243533fca19eef66d8fca372f00000ada1298a1ab"
    "e7ad688332430163a97011b4f16f577084478ec3aba7800000000006ca52"
    "9f80d65bc548133860"
  },
  {
    "0201d177211caf60421b34f3ee42555130a61d378b4b0000b559fd59841d"
    "7ce06d11a49de039cba2acd643a3bf36441c3f787d83f35166aab5e1a55c"
    "bd91dc7aea4e185e022937d458a83f3be4116c5957a2607ca9ed1c236201"
    "e6ce387458264cd8ab0eca7f084541b80b3c8a06bf6971b634337a79af59"
    "429df40796d868c59ff34ef9c39b656c3f3733b8698a6fda8be057896f2f"
    "d502bb9b48000003000003000003000492447d4d2e7dbf2c",
    "0201d177211caf60421b34f3ee42555130a61d378b4b0000b559fd59841d"
    "7ce06d11a49de039cba2acd643a3bf36441c3f787d83f35166aab5e1a55c"
    "bd91dc7aea4e185e022937d458a83f3be4116c5957a2607ca9ed1c236201"
    "e6ce387458264cd8ab0eca7f084541b80b3c8a06bf6971b634337a79af59"
    "429df40796d868c59ff34ef9c39b656c3f3733b8698a6fda8be057896f2f"
    "d502bb9b48000000000000000492447d4d2e7dbf2c"
  },
  {
    "0201d17f211caf60421b34f3ee42555130a61d378b4b00003373e5a96d8c"
    "34374836d64004bf3f88a33d63f54c2982a04d6422eea4a1eeb9fcc83f71"
    "d2e070d2898f99a5e704b00fd2099ab72ddb2f75bb3148002c862c422900"
    "000300000300052ac36175081e2e88",
    "0201d17f211caf60421b34f3ee42555130a61d378b4b00003373e5a96d8c"
    "34374836d64004bf3f88a33d63f54c2982a04d6422eea4a1eeb9fcc83f71"
    "d2e070d2898f99a5e704b00fd2099ab72ddb2f75bb3148002c862c422900"
    "00000000052ac36175081e2e88"
  },
  {
    "0201d19f213ceb355819f11f4016dfecd0d11cce00ec00000f2a347c13dd"
    "c9b0f38875c6bda3db98819154d77994cd00000300000300002bb84afe",
    "0201d19f213ceb355819f11f4016dfecd0d11cce00ec00000f2a347c13dd"
    "c9b0f38875c6bda3db98819154d77994cd0000000000002bb84afe"
  },
  {
    "0201d1a7213ceb355819f11f4016dfecd0d11cce00ec0000030000030000"
    "030000030004924894a0",
    "0201d1a7213ceb355819f11f4016dfecd0d11cce00ec0000000000000000"
    "0004924894a0"
  },
  {
    "0201d1af213ceb355819f11f4016dfecd0d11cce00ec0000030000030000"
    "030000030004924894a0",
    "0201d1af213ceb355819f11f4016dfecd0d11cce00ec0000000000000000"
    "0004924894a0"
  },
  {
    "0201d1b7213ceb355819f11f4016dfecd0d11cce00ec0000030000030000"
    "030000030004924894a0",
    "0201d1b7213ceb355819f11f4016dfecd0d11cce00ec0000000000000000"
    "0004924894a0"
  },
  {
    "0201d1bf213ceb355819f11f4016dfecd0d11cce00ec0000030000030000"
    "030000030004924894a0",
    "0201d1bf213ceb355819f11f4016dfecd0d11cce00ec0000000000000000"
    "0004924894a0"
  },
  {
    "0201d1c7215c75d40d34c15c133c5119369711128e9500000c77b6f81468"
    "ba4cd154fd9e3d78613bb777e244259b0362b6d27db9800b0b9d3fb31814"
    "cd3fa9b829b9da965db500cebb39e441eef5882375b142e06b794a8330b1"
    "e68760b9df600000030000030000285529a484421e235a591dec6f70",
    "0201d1c7215c75d40d34c15c133c5119369711128e9500000c77b6f81468"
    "ba4cd154fd9e3d78613bb777e244259b0362b6d27db9800b0b9d3fb31814"
    "cd3fa9b829b9da965db500cebb39e441eef5882375b142e06b794a8330b1"
    "e68760b9df60000000000000285529a484421e235a591dec6f70"
  },
  {
    "0201d1cf215c75d40d34c15c133c5119369711128e9500000bccaf3081b4"
    "7a7de117d1e8630678f6ea37bbd5d78019b1d828d0000003000003000a9a"
    "919779b20a608d4b57f462c9e0",
    "0201d1cf215c75d40d34c15c133c5119369711128e9500000bccaf3081b4"
    "7a7de117d1e8630678f6ea37bbd5d78019b1d828d000000000000a9a9197"
    "79b20a608d4b57f462c9e0"
  },
  {
    "0201d1d7215c75d40d34c15c133c5119369711128e9500000bccaf3081b4"
    "7a7de117d1ec373a72d2bf22247a8f0000030000030021b5ced644b1a642"
    "9da458cd18d268",
    "0201d1d7215c75d40d34c15c133c5119369711128e9500000bccaf3081b4"
    "7a7de117d1ec373a72d2bf22247a8f000000000021b5ced644b1a6429da4"
    "58cd18d268"
  },
  {
    "0201d1df215c75d40d34c15c133c5119369711128e9500000bccaf3081b4"
    "7a7de019dbe68000000300000300aec0685af5904f0a77d74134634c60",
    "0201d1df215c75d40d34c15c133c5119369711128e9500000bccaf3081b4"
    "7a7de019dbe6800000000000aec0685af5904f0a77d74134634c60"
  },
  {
    "0201d1e7215c75d40d34c15c133c5119369711128e9500000bccaf3081b4"
    "7a7de019dbe68000000300000300aec0685af5904f0a77d74134634c60",
    "0201d1e7215c75d40d34c15c133c5119369711128e9500000bccaf3081b4"
    "7a7de019dbe6800000000000aec0685af5904f0a77d74134634c60"
  },
  {
    "0201d1ef215c75d40d34c15c133c5119369711128e9500000bccaf3081b4"
    "7a7de019dbe68000000300000300aec0685af5904f0a77d74134634c60",
    "0201d1ef215c75d40d34c15c133c5119369711128e9500000bccaf3081b4"
    "7a7de019dbe6800000000000aec0685af5904f0a77d74134634c60"
  },
  {
    "0201d1f7215c75d40d34c15c133c5119369711128e9500000bccaf3081b4"
    "7a7de019dbe68000000300000300aec0685af5904f0a77d74134634c60",
    "0201d1f7215c75d40d34c15c133c5119369711128e9500000bccaf3081b4"
    "7a7de019dbe6800000000000aec0685af5904f0a77d74134634c60"
  },
  {
    "0201d1ff215c75d40d34c15c133c5119369711128e9500000bccaf3081b4"
    "7a7de019dbe68000000300000300aec0685af5904f0a77d74134634c60",
    "0201d1ff215c75d40d34c15c133c5119369711128e9500000bccaf3081b4"
    "7a7de019dbe6800000000000aec0685af5904f0a77d74134634c60"
  },
  {
    "0201d207215c75d40d34c15c133c5119369711128e9500000bccaf3081b4"
    "7a7de019dbe68000000300000300aec0685af5904f0a77d74134634c60",
    "0201d207215c75d40d34c15c133c5119369711128e9500000bccaf3081b4"
    "7a7de019dbe6800000000000aec0685af5904f0a77d74134634c60"
  },
  {
    "0201d20f215c75d40d34c15c133c5119369711128e9500000bccaf3081b4"
    "7a7de019dbe68000000300000300aec0685af5904f0a77d74134634c60",
    "0201d20f215c75d40d34c15c133c5119369711128e9500000bccaf3081b4"
    "7a7de019dbe6800000000000aec0685af5904f0a77d74134634c60"
  },
  {
    "0201d217215c75d40d34c15c133c5119369711128e9500000bccaf3081b4"
    "7a7de019dbe68000000300000300aec0685af5904f0a77d74134634c60",
    "0201d217215c75d40d34c15c133c5119369711128e9500000bccaf3081b4"
    "7a7de019dbe6800000000000aec0685af5904f0a77d74134634c60"
  },
  {
    "40010c01ffff0140000003000003000003000003005dac09",
    "40010c01ffff01400000000000000000005dac09"
  },
  {
    "4201010140000003000003000003000003005da00280802e1f1396bb9096"
    "4b8c05a80808082000007d20000bb80c00bbca20001499700005265c20",
    "42010101400000000000000000005da00280802e1f1396bb90964b8c05a8"
    "0808082000007d20000bb80c00bbca20001499700005265c20"
  },
  {
    "40010c01ffff0140000003000003000003000003005dac09",
    "40010c01ffff01400000000000000000005dac09"
  },
  {
    "4201010140000003000003000003000003005da00280802e1f1396bb9096"
    "4b8c05a80808082000007d20000bb80c00bbca20001499700005265c20",
    "42010101400000000000000000005da00280802e1f1396bb90964b8c05a8"
    "0808082000007d20000bb80c00bbca20001499700005265c20"
  },
};

//----------------------------------------------------------------
// iso14496_insert_remove_emulation_prevention_0x03
//
BOOST_AUTO_TEST_CASE(iso14496_insert_remove_emulation_prevention_0x03)
{
  for (std::size_t i = 0, n = sizeof(nals) / sizeof(nals[0]); i < n; i++)
  {
    const TwoStrs & x = nals[i];

    yae::Data src_0x03 = yae::load_hex(x.nal_0x03_);
    yae::Data src_rbsp = yae::load_hex(x.nal_rbsp_);

    yae::Data out_0x03 =
      yae::insert_emulation_prevention_0x03(src_rbsp.get(), src_rbsp.size());
    BOOST_CHECK(out_0x03.same_as(src_0x03));

    yae::Data new_rbsp =
      yae::remove_emulation_prevention_0x03(out_0x03.get(), out_0x03.size());
    BOOST_CHECK(new_rbsp.same_as(src_rbsp));

    yae::Data out_rbsp =
      yae::remove_emulation_prevention_0x03(src_0x03.get(), src_0x03.size());
    BOOST_CHECK(out_rbsp.same_as(src_rbsp));

    yae::Data new_0x03 =
      yae::insert_emulation_prevention_0x03(out_rbsp.get(), out_rbsp.size());
    BOOST_CHECK(new_0x03.same_as(src_0x03));
  }
}

//----------------------------------------------------------------
// iso14496_save_load_LengthField
//
BOOST_AUTO_TEST_CASE(iso14496_save_load_LengthField)
{
  LengthField src;
  src.payload_size_ = 12345;

  yae::Data data;

  // figure out the buffer size:
  {
    yae::NullBitstream bin;
    src.save(bin);
    data.resize((bin.position() + 7) / 8);
  }

  // save:
  yae::Bitstream writer(data);
  src.save(writer);

  // load:
  LengthField out;
  yae::Bitstream reader(data);
  out.load(reader);

  BOOST_CHECK_EQUAL(src.payload_size_, out.payload_size_);
  BOOST_CHECK_EQUAL(writer.position(), reader.position());
}

//----------------------------------------------------------------
// iso14496_load_ES_Descriptor
//
BOOST_AUTO_TEST_CASE(iso14496_load_ES_Descriptor)
{
  // backwards compatible SBR signaling:
  static const char * es_descriptor_hex =
    "031c0000000414401500000000000000000000000505131056e598060102";

  yae::Data data = yae::load_hex(es_descriptor_hex);
  yae::Bitstream bin(data);
  ES_Descriptor es_descriptor;
  BOOST_CHECK(es_descriptor.load(bin));

  ES_Descriptor::Payload & es_desc_payload =
    es_descriptor.payload<ES_Descriptor::Payload>();

  DecoderConfigDescriptor::Payload & dec_cfg_payload =
    es_desc_payload.decConfigDescr.payload<DecoderConfigDescriptor::Payload>();

  BOOST_CHECK_EQUAL(0x40, dec_cfg_payload.objectTypeIndication.data_);
  BOOST_CHECK_EQUAL(5, dec_cfg_payload.streamType.data_);
  BOOST_CHECK_EQUAL(0, dec_cfg_payload.upStream.data_);

  BOOST_CHECK_EQUAL(0, dec_cfg_payload.bufferSizeDB.data_);
  BOOST_CHECK_EQUAL(0, dec_cfg_payload.maxBitrate.data_);
  BOOST_CHECK_EQUAL(0, dec_cfg_payload.avgBitrate.data_);

  AudioSpecificConfig & asc =
    dec_cfg_payload.decSpecificInfo->payload<AudioSpecificConfig>();

  BOOST_CHECK_EQUAL(2, asc.audioObjectType_.get());
  BOOST_CHECK_EQUAL(24000, asc.samplingFrequency_.get());
  BOOST_CHECK_EQUAL(2, asc.channelConfiguration.data_);
  BOOST_CHECK_EQUAL(0x2b7, asc.syncExtensionType.data_);

  SyncExtensionType0x2b7 & ext = asc.syncExtensionType0x2b7;
  BOOST_CHECK_EQUAL(5, ext.extensionAudioObjectType_.get());
  BOOST_CHECK_EQUAL(1, ext.sbrPresentFlag.data_);
  BOOST_CHECK_EQUAL(48000, ext.extensionSamplingFrequency_.get());

  SLConfigDescriptor::Payload & sl_cfg_payload =
    es_desc_payload.slConfigDescr.payload<SLConfigDescriptor::Payload>();
  BOOST_CHECK_EQUAL(0x02, sl_cfg_payload.predefined.data_);
}

//----------------------------------------------------------------
// iso14496_load_AudioSpecificConfig
//
BOOST_AUTO_TEST_CASE(iso14496_load_AudioSpecificConfig)
{
  // explicit SBR signaling:
  yae::Data extradata = yae::load_hex("2b118800");
  yae::Bitstream bin(extradata);

  AudioSpecificConfig asc;
  asc.load(bin);

  BOOST_CHECK_EQUAL(5, asc.audioObjectType_.get());
  BOOST_CHECK_EQUAL(24000, asc.samplingFrequency_.get());
  BOOST_CHECK_EQUAL(2, asc.channelConfiguration.data_);
  BOOST_CHECK_EQUAL(48000, asc.extensionSamplingFrequency_.get());
  BOOST_CHECK_EQUAL(2, asc.extensionAudioObjectType_.get());
  BOOST_CHECK(asc.specific_config_.get() != NULL);
  BOOST_CHECK_EQUAL(0, asc.syncExtensionType.data_);
}

//----------------------------------------------------------------
// iso14496_load_AVCDecoderConfigurationRecord
//
BOOST_AUTO_TEST_CASE(iso14496_load_AVCDecoderConfigurationRecord)
{
  const std::string extradata_hex =
    "01640028FFE1001C67640028ACD940780227E59A8080"
    "80D2800001F480007530078C18CB01000468EAEF2C";

  yae::Data extradata = yae::load_hex(extradata_hex);
  AVCDecoderConfigurationRecord cfg;

  yae::Bitstream reader(extradata);
  cfg.load(reader);

  BOOST_CHECK_EQUAL(1, cfg.configurationVersion.data_);
  BOOST_CHECK_EQUAL(100, cfg.AVCProfileIndication.data_);
  BOOST_CHECK_EQUAL(0, cfg.profile_compatibility.data_);
  BOOST_CHECK_EQUAL(40, cfg.AVCLevelIndication.data_);
  BOOST_CHECK_EQUAL(3, cfg.lengthSizeMinusOne.data_);

  BOOST_CHECK_EQUAL(1, cfg.sps_.size());
  BOOST_CHECK_EQUAL(1, cfg.pps_.size());

  BOOST_CHECK(!cfg.if_AVCProfileIndication_);
}

//----------------------------------------------------------------
// iso14496_load_save_HEVCDecoderConfigurationRecord
//
BOOST_AUTO_TEST_CASE(iso14496_load_save_HEVCDecoderConfigurationRecord)
{
  const std::string extradata_hex =
    "010220000000b0000000000099f000fc"
    "fdfafa00000f03200001001840010c01ffff022000000300"
    "b00000030000030099170240210001002942010102200000"
    "0300b00000030000030099a001e02002"
    "1c4d8817b916452ffcb9fc4fea6a12201201220001000a4401c06192e3414c90"
    "00";

  // load:
  HEVCDecoderConfigurationRecord cfg;
  {
    yae::Data extradata = yae::load_hex(extradata_hex);
    yae::Bitstream reader(extradata);
    cfg.load(reader);
  }

  // save:
  std::string extradata_out;
  {
    yae::NullBitstream devnull;
    cfg.save(devnull);
    BOOST_CHECK(devnull.position() % 8 == 0);
    std::size_t cfg_size = devnull.byte_pos();

    yae::Data extradata;
    extradata.allocz(cfg_size);

    yae::Bitstream writer(extradata);
    cfg.save(writer);

    extradata_out = extradata.to_hex();
  }

  BOOST_CHECK_EQUAL(extradata_hex, extradata_out);
}

//----------------------------------------------------------------
// iso14496_load_bad_HEVCDecoderConfigurationRecord
//
BOOST_AUTO_TEST_CASE(iso14496_load_bad_HEVCDecoderConfigurationRecord)
{
  // good: hevc_videotoolbox
  const std::string good_extradata_hex =
    "010220000000b0000000000099f000fc"
    "fdfafa00000f03200001001840010c01ffff022000000300"
    "b00000030000030099170240210001002942010102200000"
    "0300b00000030000030099a001e02002"
    "1c4d8817b916452ffcb9fc4fea6a1220120122000100"
    "0a4401c06192e3414c90"
    "00";

  // bad:
  const std::string bad_extradata_hex =
    "010220000000b0000000000099f000ff"
    "fdfafa00000f03200001001840010c01ffff022000000300"
    "b00000030000030099170240210001002942010102200000"
    "0300b00000030000030099a001e02002"
    "1c4d8817b916452ffcb9fc4fea6a1220120122000100"
    "094401c06192e3414c90";

  HEVCDecoderConfigurationRecord cfg_good;
  HEVCDecoderConfigurationRecord cfg_bad;

  // load good:
  {
    yae::Data extradata = yae::load_hex(good_extradata_hex);
    yae::Bitstream reader(extradata);
    cfg_good.load(reader);
  }

  // load bad:
  {
    yae::Data extradata = yae::load_hex(bad_extradata_hex);
    yae::Bitstream reader(extradata);
    cfg_bad.load(reader);
  }

  BOOST_CHECK_EQUAL(0, cfg_good.parallelismType.data_);
  BOOST_CHECK_EQUAL(3, cfg_bad.parallelismType.data_);
}

//----------------------------------------------------------------
// iso14496_load_DOVIDecoderConfigurationRecord
//
BOOST_AUTO_TEST_CASE(iso14496_load_DOVIDecoderConfigurationRecord)
{
  const std::string extradata_hex =
    "0100102d1000000000000000000000000000000000000000";

  yae::Data extradata = yae::load_hex(extradata_hex);
  DOVIDecoderConfigurationRecord cfg;

  yae::Bitstream reader(extradata);
  BOOST_CHECK(cfg.load(reader));

  BOOST_CHECK_EQUAL(1, cfg.dv_version_major.data_);
  BOOST_CHECK_EQUAL(0, cfg.dv_version_minor.data_);

  BOOST_CHECK_EQUAL(8, cfg.dv_profile.data_);
  BOOST_CHECK_EQUAL(5, cfg.dv_level.data_);

  BOOST_CHECK_EQUAL(1, cfg.rpu_present_flag.data_);
  BOOST_CHECK_EQUAL(0, cfg.el_present_flag.data_);
  BOOST_CHECK_EQUAL(1, cfg.bl_present_flag.data_);

  BOOST_CHECK_EQUAL(1, cfg.dv_bl_signal_compatibility_id.data_);
}

//----------------------------------------------------------------
// iso14496_save_DOVIDecoderConfigurationRecord
//
BOOST_AUTO_TEST_CASE(iso14496_save_DOVIDecoderConfigurationRecord)
{
  DOVIDecoderConfigurationRecord cfg;
  cfg.dv_profile.data_ = 8;
  cfg.dv_level.data_ = 5;
  cfg.rpu_present_flag.data_ = 1;
  cfg.el_present_flag.data_ = 0;
  cfg.bl_present_flag.data_ = 1;
  cfg.dv_bl_signal_compatibility_id.data_ = 1;

  yae::Data payload(24);
  yae::Bitstream writer(payload);
  cfg.save(writer);

  std::string payload_hex = payload.to_hex();
  BOOST_CHECK_EQUAL("0100102d1000000000000000000000000000000000000000",
                    payload_hex);
}
