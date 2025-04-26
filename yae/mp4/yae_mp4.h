// -*- Mode: c++; tab-width: 8; c-basic-offset: 2; indent-tabs-mode: nil -*-
// NOTE: the first line of this file sets up source code indentation rules
// for Emacs; it is also a hint to anyone modifying this file.

// Created   : Sun Mar 30 12:09:04 PM MDT 2025
// Copyright : Pavel Koshevoy
// License   : MIT -- http://www.opensource.org/licenses/mit-license.php

#ifndef YAE_MP4_H_
#define YAE_MP4_H_

// aeyae:
#include "yae/api/yae_api.h"
#include "yae/api/yae_shared_ptr.h"
#include "yae/utils/yae_data.h"
#include "yae/utils/yae_json.h"
#include "yae/utils/yae_utils.h"

// boost:
#ifndef Q_MOC_RUN
#include <boost/shared_ptr.hpp>
#endif

// standard:
#include <string.h>
#include <list>
#include <vector>


namespace yae
{

  // forward declarations:
  struct Mp4Context;

  //----------------------------------------------------------------
  // FourCC
  //
  struct YAE_API FourCC
  {
    FourCC(const char * fourcc = "")
    { this->set(fourcc); }

    inline void clear()
    { memset(str_, 0, sizeof(str_)); }

    inline bool empty() const
    { return memcmp(str_, "\0\0\0\0", 4) == 0; }

    inline uint32_t get() const
    { return yae::load_be32((const uint8_t *)str_); }

    inline void set(uint32_t fourcc)
    { yae::save_be32((uint8_t *)str_, fourcc); }

    inline void set(const char * fourcc)
    {
      strncpy(str_, fourcc, 4);
      str_[4] = 0;
    }

    inline void load(IBitstream & bin)
    { bin.read_bytes(str_, 4); }

    inline bool same_as(const char * fourcc) const
    { return strncmp(str_, fourcc, 4) == 0; }

    inline bool operator < (const FourCC & other) const
    { return strncmp(str_, other.str_, 4) < 0; }

    char str_[5];
  };

  inline void save(Json::Value & json, const FourCC & v)
  { json = v.str_; }

  inline void load(const Json::Value & json, FourCC & v)
  { v.set(json.asCString()); }


  // see ISO/IEC 14496-12:2015(E)
  namespace iso_14496_12
  {
    // forward declarations:
    struct Box;

    //----------------------------------------------------------------
    // TBoxConstructor
    //
    typedef Box *(*TBoxConstructor)(const char * fourcc);

    //----------------------------------------------------------------
    // TBoxPtr
    //
    typedef boost::shared_ptr<Box> TBoxPtr;

    //----------------------------------------------------------------
    // TBoxPtrVec
    //
    typedef std::vector<TBoxPtr> TBoxPtrVec;
  }


  //----------------------------------------------------------------
  // Mp4Context
  //
  struct YAE_API Mp4Context
  {
    Mp4Context():
      load_mdat_data_(false),
      parse_mdat_data_(false),
      senc_iv_size_(0)
    {}

    iso_14496_12::TBoxPtr parse(IBitstream & bin, std::size_t end_pos);

    bool load_mdat_data_;
    bool parse_mdat_data_;
    uint32_t senc_iv_size_;
  };


  //----------------------------------------------------------------
  // LoadVec
  //
  template <typename TData>
  struct LoadVec
  {
    template <typename TContainer>
    LoadVec(TContainer & data,
            IBitstream & bin,
            std::size_t end_pos)
    {
      std::size_t item_size = sizeof(TData) * 8;
      while (bin.position() + item_size <= end_pos)
      {
        TData v = bin.read<TData>();
        data.push_back(v);
      }
    }

    template <typename TContainer>
    LoadVec(std::size_t num_items,
            TContainer & data,
            IBitstream & bin,
            std::size_t end_pos)
    {
      std::size_t item_size = sizeof(TData) * 8;
      for (std::size_t i = 0; i < num_items; ++i)
      {
        if (end_pos < bin.position() + item_size < end_pos)
        {
          break;
        }

        TData v = bin.read<TData>();
        data.push_back(v);
      }
    }
  };


  namespace iso_14496_12
  {
    //----------------------------------------------------------------
    // Box
    //
    struct YAE_API Box
    {
      Box(): size_(0) {}
      virtual ~Box() {}

      virtual void load(Mp4Context & mp4, IBitstream & bin);

      // helper:
      virtual void to_json(Json::Value & out) const;

      // container boxes will override this:
      virtual const TBoxPtrVec * has_children() const
      { return NULL; }

      // breadth-first search through given boxes:
      static const Box * find(const TBoxPtrVec & boxes, const char * fourcc);

      // breadth-first search through children, does not check 'this':
      virtual const Box * find_child(const char * fourcc) const;

      // check 'this', if not found then breadth-first search through children:
      template <typename TBox>
      const TBox *
      find(const char * fourcc) const
      {
        const TBox * found = NULL;
        const Box * box =
          type_.same_as(fourcc) ? this : this->find_child(fourcc);
        if (box)
        {
          found = dynamic_cast<const TBox *>(box);
          YAE_ASSERT(found);
          return NULL;
        }
        return found;
      }

      uint64_t size_;
      FourCC type_;
      Data uuid_; // optional
    };

    //----------------------------------------------------------------
    // FullBox
    //
    struct YAE_API FullBox : public Box
    {
      FullBox(): version_(0), flags_(0) {}

      void load(Mp4Context & mp4, IBitstream & bin) YAE_OVERRIDE;
      void to_json(Json::Value & out) const YAE_OVERRIDE;

      uint8_t version_;
      uint32_t flags_;
    };


    //----------------------------------------------------------------
    // BoxWithChildren
    //
    template <typename TBox>
    struct YAE_API BoxWithChildren : public TBox
    {
      typedef TBox TBase;
      typedef BoxWithChildren<TBox> TSelf;

      const TBoxPtrVec * has_children() const YAE_OVERRIDE
      { return &children_; }

      void load(Mp4Context & mp4, IBitstream & bin) YAE_OVERRIDE
      {
        const std::size_t box_pos = bin.position();
        TBase::load(mp4, bin);

        const std::size_t end_pos = box_pos + TBox::size_ * 8;
        this->load_children_until(mp4, bin, end_pos);
      }

      void load_children_until(Mp4Context & mp4,
                               IBitstream & bin,
                               std::size_t end_pos)
      {
        children_.clear();
        while (bin.position() < end_pos)
        {
          TBoxPtr box = mp4.parse(bin, end_pos);
          YAE_ASSERT(box);
          if (box)
          {
            children_.push_back(box);
          }
        }
      }

      void load_children(Mp4Context & mp4,
                         IBitstream & bin,
                         std::size_t end_pos,
                         std::size_t num_children)
      {
        children_.clear();
        for (std::size_t i = 0; i < num_children; ++i)
        {
          TBoxPtr box = mp4.parse(bin, end_pos);
          YAE_ASSERT(box);
          if (box)
          {
            children_.push_back(box);
          }
        }
      }

      void to_json(Json::Value & out) const YAE_OVERRIDE
      {
        TBase::to_json(out);
        Json::Value & children = out["children"];
        this->children_to_json(children);
      }

      void children_to_json(Json::Value & children) const
      {
        for (uint64_t i = 0, n = children_.size(); i < n; ++i)
        {
          const Box & box = *(children_[i]);
          Json::Value child;
          box.to_json(child);
          children.append(child);
        }
      }

      TBoxPtrVec children_;
    };

    //----------------------------------------------------------------
    // Container
    //
    struct YAE_API Container : public BoxWithChildren<Box> {};

    //----------------------------------------------------------------
    // ContainerEx
    //
    struct YAE_API ContainerEx : public BoxWithChildren<FullBox> {};

    //----------------------------------------------------------------
    // ContainerList
    //
    template <typename TBoxCount = uint32_t>
    struct YAE_API ContainerList : public BoxWithChildren<FullBox>
    {
      typedef BoxWithChildren<FullBox> TBase;
      typedef ContainerList<TBoxCount> TSelf;

      ContainerList(): entry_count_(0) {}

      void load(Mp4Context & mp4, IBitstream & bin) YAE_OVERRIDE
      {
        const std::size_t box_pos = bin.position();
        TBase::load(mp4, bin);

        const std::size_t end_pos = box_pos + Box::size_ * 8;
        entry_count_ = bin.read<TBoxCount>();
        TBase::load_children(mp4, bin, end_pos, entry_count_);
      }

      void to_json(Json::Value & out) const YAE_OVERRIDE
      {
        TBase::to_json(out);

        out["entry_count"] = Json::UInt64(entry_count_);

        Json::Value & children = out["children"];
        for (uint64_t i = 0, n = children_.size(); i < n; ++i)
        {
          const Box & box = *(children_[i]);
          Json::Value child;
          box.to_json(child);
          children.append(child);
        }
      }

      TBoxCount entry_count_;
    };

    //----------------------------------------------------------------
    // ContainerList16
    //
    struct YAE_API ContainerList16 : public ContainerList<uint16_t> {};

    //----------------------------------------------------------------
    // ContainerList32
    //
    struct YAE_API ContainerList32 : public ContainerList<uint32_t> {};


    //----------------------------------------------------------------
    // FileTypeBox
    //
    struct YAE_API FileTypeBox : public Box
    {
      FileTypeBox(): minor_(0) {}

      void load(Mp4Context & mp4, IBitstream & bin) YAE_OVERRIDE;
      void to_json(Json::Value & out) const YAE_OVERRIDE;

      FourCC major_;
      uint32_t minor_;
      std::vector<FourCC> compatible_;
    };

    //----------------------------------------------------------------
    // FreeSpaceBox
    //
    struct YAE_API FreeSpaceBox : public Box
    {
      void load(Mp4Context & mp4, IBitstream & bin) YAE_OVERRIDE;
    };

    //----------------------------------------------------------------
    // MediaDataBox
    //
    struct YAE_API MediaDataBox : public Box
    {
      void load(Mp4Context & mp4, IBitstream & bin) YAE_OVERRIDE;

      Data data_;
    };

    //----------------------------------------------------------------
    // ProgressiveDownloadInfoBox
    //
    struct YAE_API ProgressiveDownloadInfoBox : public FullBox
    {
      void load(Mp4Context & mp4, IBitstream & bin) YAE_OVERRIDE;
      void to_json(Json::Value & out) const YAE_OVERRIDE;

      std::vector<uint32_t> rate_; // bytes per sec
      std::vector<uint32_t> initial_delay_;
    };

    //----------------------------------------------------------------
    // MovieHeaderBox
    //
    struct YAE_API MovieHeaderBox : public FullBox
    {
      MovieHeaderBox();

      void load(Mp4Context & mp4, IBitstream & bin) YAE_OVERRIDE;
      void to_json(Json::Value & out) const YAE_OVERRIDE;

      uint64_t creation_time_; // seconds since 1904/01/01 00:00:00, UTC
      uint64_t modification_time_; // seconds since 1904/01/01 00:00:00, UTC
      uint32_t timescale_;
      uint64_t duration_;

      int32_t rate_; // 16.16 fixed point number
      int16_t volume_; // 8.8 fixed point number
      uint16_t reserved_16_;
      uint32_t reserved_[2];

      int32_t matrix_[9]; // 16.16 fixed point numbers

      uint32_t pre_defined_[6]; // zeros
      uint32_t next_track_ID_; // non-zero
    };

    //----------------------------------------------------------------
    // TrackHeaderBox
    //
    struct YAE_API TrackHeaderBox : public FullBox
    {
      TrackHeaderBox();

      void load(Mp4Context & mp4, IBitstream & bin) YAE_OVERRIDE;
      void to_json(Json::Value & out) const YAE_OVERRIDE;

      enum Flags {
        kTrackEnabled   = 0x000001,
        kTrackInMovie   = 0x000002,
        kTrackInPreview = 0x000004,
      };

      inline bool is_track_enabled() const
      { return (FullBox::flags_ & kTrackEnabled) != 0; }

      inline bool is_track_in_movie() const
      { return (FullBox::flags_ & kTrackInMovie) != 0; }

      inline bool is_track_in_preview() const
      { return (FullBox::flags_ & kTrackInPreview) != 0; }

      uint64_t creation_time_; // seconds since 1904/01/01 00:00:00, UTC
      uint64_t modification_time_; // seconds since 1904/01/01 00:00:00, UTC
      uint32_t track_ID_; // unique, non-zero
      uint32_t reserved1_;
      uint64_t duration_; // in mvhd timescale

      uint32_t reserved2_[2];
      int16_t layer_; // front-to-back, lower number closer to viewer
      int16_t alternate_group_;
      int16_t volume_; // fixed point 8.8 0x0100 if audio else 0
      uint16_t reserved3_;

      int32_t matrix_[9]; // 16.16 fixed point numbers

      uint32_t width_; // fixed point 16.16
      uint32_t height_; // fixed point 16.16
    };

    //----------------------------------------------------------------
    // TrackReferenceTypeBox
    //
    struct YAE_API TrackReferenceTypeBox : public Box
    {
      void load(Mp4Context & mp4, IBitstream & bin) YAE_OVERRIDE;
      void to_json(Json::Value & out) const YAE_OVERRIDE;

      std::vector<uint32_t> track_IDs_;
    };

    //----------------------------------------------------------------
    // TrackGroupTypeBox
    //
    struct YAE_API TrackGroupTypeBox : public FullBox
    {
      TrackGroupTypeBox(): track_group_id_(0) {}

      void load(Mp4Context & mp4, IBitstream & bin) YAE_OVERRIDE;
      void to_json(Json::Value & out) const YAE_OVERRIDE;

      uint32_t track_group_id_;
      Data data_;
    };

    //----------------------------------------------------------------
    // MediaHeaderBox
    //
    struct YAE_API MediaHeaderBox : public FullBox
    {
      MediaHeaderBox();

      void load(Mp4Context & mp4, IBitstream & bin) YAE_OVERRIDE;
      void to_json(Json::Value & out) const YAE_OVERRIDE;

      uint64_t creation_time_; // seconds since 1904/01/01 00:00:00, UTC
      uint64_t modification_time_; // seconds since 1904/01/01 00:00:00, UTC
      uint32_t timescale_;
      uint64_t duration_;

      char language_[4]; // ISO-639-2/T language code
      uint16_t pre_defined_; // zero
    };

    //----------------------------------------------------------------
    // HandlerBox
    //
    struct YAE_API HandlerBox : public FullBox
    {
      HandlerBox();

      void load(Mp4Context & mp4, IBitstream & bin) YAE_OVERRIDE;
      void to_json(Json::Value & out) const YAE_OVERRIDE;

      uint32_t pre_defined_; // zero
      FourCC handler_type_;
      uint32_t reserved_[3]; // zero
      std::string name_; // null-terminated UTF-8
    };

    //----------------------------------------------------------------
    // NullMediaHeaderBox
    //
    struct YAE_API NullMediaHeaderBox : public FullBox {};

    //----------------------------------------------------------------
    // ExtendedLanguageBox
    //
    struct YAE_API ExtendedLanguageBox : public FullBox
    {
      void load(Mp4Context & mp4, IBitstream & bin) YAE_OVERRIDE;
      void to_json(Json::Value & out) const YAE_OVERRIDE;

      // null-terminated C string containing an RFC 4646 (BCP 47) compliant
      // language tag string, such as "en-US", "fr-FR", "zh-CN":
      std::string extended_language_;
    };

    //----------------------------------------------------------------
    // SampleEntryBox
    //
    struct YAE_API SampleEntryBox : public Box
    {
      SampleEntryBox();

      void load(Mp4Context & mp4, IBitstream & bin) YAE_OVERRIDE;
      void to_json(Json::Value & out) const YAE_OVERRIDE;

      uint8_t reserved_[2]; // zero
      uint16_t data_reference_index_;
    };

    //----------------------------------------------------------------
    // BitRateBox
    //
    struct YAE_API BitRateBox : public Box
    {
      BitRateBox();

      void load(Mp4Context & mp4, IBitstream & bin) YAE_OVERRIDE;
      void to_json(Json::Value & out) const YAE_OVERRIDE;

      // size of decoding buffer for the elementary stream, in bytes:
      uint32_t bufferSizeDB_;

      // bits/second rate over 1s window:
      uint32_t maxBitrate_;
      uint32_t avgBitrate_;
    };

    //----------------------------------------------------------------
    // DegradationPriorityBox
    //
    struct YAE_API DegradationPriorityBox : public FullBox
    {
      void load(Mp4Context & mp4, IBitstream & bin) YAE_OVERRIDE;
      void to_json(Json::Value & out) const YAE_OVERRIDE;

      // sample count is specified in 'stsz' SampleSizeBox:
      std::vector<uint16_t> priority_;
    };

    //----------------------------------------------------------------
    // TimeToSampleBox
    //
    struct YAE_API TimeToSampleBox : public FullBox
    {
      TimeToSampleBox(): entry_count_(0) {}

      void load(Mp4Context & mp4, IBitstream & bin) YAE_OVERRIDE;
      void to_json(Json::Value & out) const YAE_OVERRIDE;

      uint32_t entry_count_;
      std::vector<uint32_t> sample_count_;
      std::vector<uint32_t> sample_delta_;
    };

    //----------------------------------------------------------------
    // CompositionOffsetBox
    //
    struct YAE_API CompositionOffsetBox : public FullBox
    {
      CompositionOffsetBox(): entry_count_(0) {}

      void load(Mp4Context & mp4, IBitstream & bin) YAE_OVERRIDE;
      void to_json(Json::Value & out) const YAE_OVERRIDE;

      uint32_t entry_count_;
      std::vector<uint32_t> sample_count_;
      std::vector<int32_t> sample_offset_;
    };

    //----------------------------------------------------------------
    // CompositionToDecodeBox
    //
    struct YAE_API CompositionToDecodeBox : public FullBox
    {
      CompositionToDecodeBox();

      void load(Mp4Context & mp4, IBitstream & bin) YAE_OVERRIDE;
      void to_json(Json::Value & out) const YAE_OVERRIDE;

      int64_t composition_to_dts_shift_;
      int64_t least_decode_to_display_delta_;
      int64_t greatest_decode_to_display_delta_;
      int64_t composition_start_time_;
      int64_t composition_end_time_;
    };

    //----------------------------------------------------------------
    // SyncSampleBox
    //
    struct YAE_API SyncSampleBox : public FullBox
    {
      SyncSampleBox(): entry_count_(0) {}

      void load(Mp4Context & mp4, IBitstream & bin) YAE_OVERRIDE;
      void to_json(Json::Value & out) const YAE_OVERRIDE;

      // keyframes:
      uint32_t entry_count_;
      std::vector<uint32_t> sample_number_;
    };

    //----------------------------------------------------------------
    // ShadowSyncSampleBox
    //
    struct YAE_API ShadowSyncSampleBox : public FullBox
    {
      ShadowSyncSampleBox(): entry_count_(0) {}

      void load(Mp4Context & mp4, IBitstream & bin) YAE_OVERRIDE;
      void to_json(Json::Value & out) const YAE_OVERRIDE;

      uint32_t entry_count_;
      std::vector<uint32_t> shadowed_sample_number_;
      std::vector<uint32_t> sync_sample_number_;
    };

    //----------------------------------------------------------------
    // SampleDependencyTypeBox
    //
    struct YAE_API SampleDependencyTypeBox : public FullBox
    {
      void load(Mp4Context & mp4, IBitstream & bin) YAE_OVERRIDE;
      void to_json(Json::Value & out) const YAE_OVERRIDE;

      struct YAE_API Sample
      {
        uint8_t is_leading_ : 2;
        uint8_t depends_on_ : 2;
        uint8_t is_depended_on_ : 2;
        uint8_t has_redundancy_ : 2;
      };

      std::vector<Sample> samples_;
    };

    //----------------------------------------------------------------
    // EditListBox
    //
    struct YAE_API EditListBox : public FullBox
    {
      EditListBox(): entry_count_(0) {}

      void load(Mp4Context & mp4, IBitstream & bin) YAE_OVERRIDE;
      void to_json(Json::Value & out) const YAE_OVERRIDE;

      uint32_t entry_count_;
      std::vector<uint64_t> segment_duration_;
      std::vector<int64_t> media_time_;
      std::vector<int16_t> media_rate_integer_;
      std::vector<int16_t> media_rate_fraction_;
    };

    //----------------------------------------------------------------
    // DataEntryUrlBox
    //
    struct YAE_API DataEntryUrlBox : public FullBox
    {
      void load(Mp4Context & mp4, IBitstream & bin) YAE_OVERRIDE;
      void to_json(Json::Value & out) const YAE_OVERRIDE;

      // null-terminated UTF-8 string:
      std::string location_;
    };

    //----------------------------------------------------------------
    // DataEntryUrnBox
    //
    struct YAE_API DataEntryUrnBox : public FullBox
    {
      void load(Mp4Context & mp4, IBitstream & bin) YAE_OVERRIDE;
      void to_json(Json::Value & out) const YAE_OVERRIDE;

      // null-terminated UTF-8 strings:
      std::string name_;
      std::string location_;
    };

    //----------------------------------------------------------------
    // SampleSizeBox
    //
    struct YAE_API SampleSizeBox : public FullBox
    {
      SampleSizeBox();

      void load(Mp4Context & mp4, IBitstream & bin) YAE_OVERRIDE;
      void to_json(Json::Value & out) const YAE_OVERRIDE;

      uint32_t sample_size_;
      uint32_t sample_count_;
      std::vector<uint32_t> entry_size_;
    };

    //----------------------------------------------------------------
    // CompactSampleSizeBox
    //
    struct YAE_API CompactSampleSizeBox : public FullBox
    {
      CompactSampleSizeBox();

      void load(Mp4Context & mp4, IBitstream & bin) YAE_OVERRIDE;
      void to_json(Json::Value & out) const YAE_OVERRIDE;

      uint32_t reserved_;
      uint8_t field_size_; // 4, 8, 16 bits
      uint32_t sample_count_;
      std::vector<uint16_t> entry_size_;
    };

    //----------------------------------------------------------------
    // SampleToChunkBox
    //
    struct YAE_API SampleToChunkBox : public FullBox
    {
      SampleToChunkBox(): entry_count_(0) {}

      void load(Mp4Context & mp4, IBitstream & bin) YAE_OVERRIDE;
      void to_json(Json::Value & out) const YAE_OVERRIDE;

      uint32_t entry_count_;
      std::vector<uint32_t> first_chunk_;
      std::vector<uint32_t> samples_per_chunk_;
      std::vector<uint32_t> sample_description_index_;
    };

    //----------------------------------------------------------------
    // ChunkOffsetBox
    //
    struct YAE_API ChunkOffsetBox : public FullBox
    {
      ChunkOffsetBox(): entry_count_(0) {}

      void load(Mp4Context & mp4, IBitstream & bin) YAE_OVERRIDE;
      void to_json(Json::Value & out) const YAE_OVERRIDE;

      uint32_t entry_count_;
      std::vector<uint32_t> chunk_offset_;
    };

    //----------------------------------------------------------------
    // ChunkLargeOffsetBox
    //
    struct YAE_API ChunkLargeOffsetBox : public FullBox
    {
      ChunkLargeOffsetBox(): entry_count_(0) {}

      void load(Mp4Context & mp4, IBitstream & bin) YAE_OVERRIDE;
      void to_json(Json::Value & out) const YAE_OVERRIDE;

      uint32_t entry_count_;
      std::vector<uint64_t> chunk_offset_;
    };

    //----------------------------------------------------------------
    // PaddingBitsBox
    //
    struct YAE_API PaddingBitsBox : public FullBox
    {
      PaddingBitsBox(): sample_count_(0) {}

      void load(Mp4Context & mp4, IBitstream & bin) YAE_OVERRIDE;
      void to_json(Json::Value & out) const YAE_OVERRIDE;

      struct YAE_API Sample
      {
        uint8_t reserved1_ : 1;
        uint8_t pad1_ : 3;
        uint8_t reserved2_ : 1;
        uint8_t pad2_ : 3;
      };

      uint32_t sample_count_;
      std::vector<Sample> samples_;
    };

    //----------------------------------------------------------------
    // SubSampleInformationBox
    //
    struct YAE_API SubSampleInformationBox : public FullBox
    {
      SubSampleInformationBox(): entry_count_(0) {}

      void load(Mp4Context & mp4, IBitstream & bin) YAE_OVERRIDE;
      void to_json(Json::Value & out) const YAE_OVERRIDE;

      struct YAE_API Entry
      {
        uint32_t sample_delta_;
        std::vector<uint32_t> subsample_size_;
        std::vector<uint8_t> subsample_priority_;
        std::vector<uint8_t> discardable_;
        std::vector<uint32_t> codec_specific_parameters_;
      };

      uint32_t entry_count_;
      std::vector<Entry> entries_;
    };

    //----------------------------------------------------------------
    // SampleAuxiliaryInformationSizesBox
    //
    struct YAE_API SampleAuxiliaryInformationSizesBox : public FullBox
    {
      SampleAuxiliaryInformationSizesBox():
        aux_info_type_(0),
        aux_info_type_parameters_(0),
        default_sample_info_size_(0),
        sample_count_(0)
      {}

      void load(Mp4Context & mp4, IBitstream & bin) YAE_OVERRIDE;
      void to_json(Json::Value & out) const YAE_OVERRIDE;

      // if ((flags & 1) == 1):
      uint32_t aux_info_type_;
      uint32_t aux_info_type_parameters_;

      uint8_t default_sample_info_size_;
      uint32_t sample_count_;

      // if default_sample_info_size == 0:
      std::vector<uint8_t> sample_info_sizes_;
    };

    //----------------------------------------------------------------
    // SampleAuxiliaryInformationOffsetsBox
    //
    struct YAE_API SampleAuxiliaryInformationOffsetsBox : public FullBox
    {
      SampleAuxiliaryInformationOffsetsBox():
        aux_info_type_(0),
        aux_info_type_parameters_(0),
        entry_count_(0)
      {}

      void load(Mp4Context & mp4, IBitstream & bin) YAE_OVERRIDE;
      void to_json(Json::Value & out) const YAE_OVERRIDE;

      // if ((flags & 1) == 1):
      uint32_t aux_info_type_;
      uint32_t aux_info_type_parameters_;

      uint32_t entry_count_;
      std::vector<uint64_t> offsets_;
    };

    //----------------------------------------------------------------
    // MovieExtendsHeaderBox
    //
    struct YAE_API MovieExtendsHeaderBox : public FullBox
    {
      MovieExtendsHeaderBox(): fragment_duration_(0) {}

      void load(Mp4Context & mp4, IBitstream & bin) YAE_OVERRIDE;
      void to_json(Json::Value & out) const YAE_OVERRIDE;

      uint64_t fragment_duration_;
    };

    //----------------------------------------------------------------
    // SampleFlags
    //
    // see ISO/IEC 14496-12:2015(E), 8.8.3.1
    //
    struct YAE_API SampleFlags
    {
      SampleFlags();

      void load(IBitstream & bin);
      void to_json(Json::Value & out) const;

      uint32_t reserved_ : 4; // 0
      uint32_t is_leading_ : 2;
      uint32_t depends_on_ : 2;
      uint32_t is_depended_on_ : 2;
      uint32_t has_redundancy_ : 2;
      uint32_t sample_padding_value_ : 3;
      uint32_t sample_is_non_sync_sample_ : 1;
      uint32_t sample_degradation_priority_ : 16;
    };

    //----------------------------------------------------------------
    // TrackExtendsBox
    //
    struct YAE_API TrackExtendsBox : public FullBox
    {
      TrackExtendsBox():
        track_ID_(0),
        default_sample_description_index_(0),
        default_sample_duration_(0),
        default_sample_size_(0)
      {}

      void load(Mp4Context & mp4, IBitstream & bin) YAE_OVERRIDE;
      void to_json(Json::Value & out) const YAE_OVERRIDE;

      uint32_t track_ID_;
      uint32_t default_sample_description_index_;
      uint32_t default_sample_duration_;
      uint32_t default_sample_size_;
      SampleFlags default_sample_flags_;
    };

    //----------------------------------------------------------------
    // MovieFragmentHeaderBox
    //
    struct YAE_API MovieFragmentHeaderBox : public FullBox
    {
      MovieFragmentHeaderBox(): sequence_number_(0) {}

      void load(Mp4Context & mp4, IBitstream & bin) YAE_OVERRIDE;
      void to_json(Json::Value & out) const YAE_OVERRIDE;

      uint32_t sequence_number_;
    };

    //----------------------------------------------------------------
    // TrackFragmentHeaderBox
    //
    struct YAE_API TrackFragmentHeaderBox : public FullBox
    {
      TrackFragmentHeaderBox():
        track_ID_(0),
        base_data_offset_(0),
        sample_description_index_(0),
        default_sample_duration_(0),
        default_sample_size_(0)
      {}

      enum Flags {
        kBaseDataOffsetPresent         = 0x000001,
        kSampleDescriptionIndexPresent = 0x000002,
        kDefaultSampleDurationPresent  = 0x000008,
        kDefaultSampleSizePresent      = 0x000010,
        kDefaultSampleFlagsPresent     = 0x000020,
        kDurationIsEmpty               = 0x010000,
        kDefaultBaseIsMoof             = 0x020000,
      };

      void load(Mp4Context & mp4, IBitstream & bin) YAE_OVERRIDE;
      void to_json(Json::Value & out) const YAE_OVERRIDE;

      uint32_t track_ID_;
      uint32_t base_data_offset_;
      uint32_t sample_description_index_;
      uint32_t default_sample_duration_;
      uint32_t default_sample_size_;
      SampleFlags default_sample_flags_;
    };

    //----------------------------------------------------------------
    // TrackRunBox
    //
    struct YAE_API TrackRunBox : public FullBox
    {
      TrackRunBox():
        sample_count_(0),
        data_offset_(0)
      {}

      enum Flags {
        kDataOffsetPresent                   = 0x000001,
        kFirstSampleFlagsPresent             = 0x000004,
        kSampleDurationPresent               = 0x000100,
        kSampleSizePresent                   = 0x000200,
        kSampleFlagsPresent                  = 0x000400,
        kSampleCompositionTimeOffsetsPresent = 0x000800,
      };

      void load(Mp4Context & mp4, IBitstream & bin) YAE_OVERRIDE;
      void to_json(Json::Value & out) const YAE_OVERRIDE;

      uint32_t sample_count_;
      int32_t data_offset_;
      SampleFlags first_sample_flags_;
      std::vector<uint32_t> sample_duration_;
      std::vector<uint32_t> sample_size_;
      std::vector<SampleFlags> sample_flags_;
      std::vector<int64_t> sample_composition_time_offset_;
    };

    //----------------------------------------------------------------
    // TrackFragmentRandomAccessBox
    //
    struct YAE_API TrackFragmentRandomAccessBox : public FullBox
    {
      TrackFragmentRandomAccessBox():
        track_ID_(0),
        reserved_(0),
        length_size_of_traf_num_(0),
        length_size_of_trun_num_(0),
        length_size_of_sample_num_(0),
        entry_count_(0)
      {}

      void load(Mp4Context & mp4, IBitstream & bin) YAE_OVERRIDE;
      void to_json(Json::Value & out) const YAE_OVERRIDE;

      uint32_t track_ID_;
      uint32_t reserved_ : 26;
      uint32_t length_size_of_traf_num_ : 2;
      uint32_t length_size_of_trun_num_ : 2;
      uint32_t length_size_of_sample_num_ : 2;
      uint32_t entry_count_;

      std::vector<uint64_t> time_;
      std::vector<uint64_t> moof_offset_;
      std::vector<uint32_t> traf_number_;
      std::vector<uint32_t> trun_number_;
      std::vector<uint32_t> sample_number_;
    };

    //----------------------------------------------------------------
    // MovieFragmentRandomAccessOffsetBoxBox
    //
    struct YAE_API MovieFragmentRandomAccessOffsetBoxBox : public FullBox
    {
      MovieFragmentRandomAccessOffsetBoxBox(): size_(0) {}

      void load(Mp4Context & mp4, IBitstream & bin) YAE_OVERRIDE;
      void to_json(Json::Value & out) const YAE_OVERRIDE;

      uint32_t size_; // number of bytes of the enclosing 'mfra' box
    };

    //----------------------------------------------------------------
    // TrackFragmentBaseMediaDecodeTimeBox
    //
    struct YAE_API TrackFragmentBaseMediaDecodeTimeBox : public FullBox
    {
      TrackFragmentBaseMediaDecodeTimeBox(): baseMediaDecodeTime_(0) {}

      void load(Mp4Context & mp4, IBitstream & bin) YAE_OVERRIDE;
      void to_json(Json::Value & out) const YAE_OVERRIDE;

      // the sum of the decode durations of all earlier samples in the media,
      // expressed in the media's timescale:
      uint64_t baseMediaDecodeTime_; // PTS of the 1st sample in the fragment
    };

    //----------------------------------------------------------------
    // LevelAssignmentBox
    //
    struct YAE_API LevelAssignmentBox : public FullBox
    {
      LevelAssignmentBox(): level_count_(0) {}

      void load(Mp4Context & mp4, IBitstream & bin) YAE_OVERRIDE;
      void to_json(Json::Value & out) const YAE_OVERRIDE;

      struct YAE_API Level
      {
        Level();

        void load(IBitstream & bin);
        void to_json(Json::Value & out) const;

        uint32_t track_id_;
        uint8_t padding_flag_ : 1;
        uint8_t assignment_type_ : 7;

        // assignment type 0, 1:
        FourCC grouping_type_;

        // assignment type 1:
        uint32_t grouping_type_parameter_;

        // assignment type 4:
        uint32_t sub_track_id_;
      };

      uint8_t level_count_;
      std::vector<Level> levels_;
    };

    //----------------------------------------------------------------
    // TrackExtensionPropertiesBox
    //
    struct YAE_API TrackExtensionPropertiesBox : public ContainerEx
    {
      TrackExtensionPropertiesBox(): track_id_(0) {}

      void load(Mp4Context & mp4, IBitstream & bin) YAE_OVERRIDE;
      void to_json(Json::Value & out) const YAE_OVERRIDE;

      uint32_t track_id_;
    };

    //----------------------------------------------------------------
    // AlternativeStartupSequencePropertiesBox
    //
    struct YAE_API AlternativeStartupSequencePropertiesBox : public FullBox
    {
      AlternativeStartupSequencePropertiesBox():
        min_initial_alt_startup_offset_(0),
       num_entries_(0)
      {}

      void load(Mp4Context & mp4, IBitstream & bin) YAE_OVERRIDE;
      void to_json(Json::Value & out) const YAE_OVERRIDE;

      // version 0:
      int32_t min_initial_alt_startup_offset_;

      // version 1:
      uint32_t num_entries_;
      std::vector<uint32_t> grouping_type_parameters_;
      std::vector<int32_t> min_initial_alt_startup_offsets_;
    };

    //----------------------------------------------------------------
    // SampleToGroupBox
    //
    struct YAE_API SampleToGroupBox : public FullBox
    {
      SampleToGroupBox():
        grouping_type_parameter_(0),
        entry_count_(0)
      {}

      void load(Mp4Context & mp4, IBitstream & bin) YAE_OVERRIDE;
      void to_json(Json::Value & out) const YAE_OVERRIDE;

      inline std::size_t get_entry_count() const
      { return sample_count_.size(); }

      FourCC grouping_type_;
      uint32_t grouping_type_parameter_; // version 1

      uint32_t entry_count_;
      std::vector<uint32_t> sample_count_;
      std::vector<uint32_t> group_description_index_;
    };

    //----------------------------------------------------------------
    // SampleGroupDescriptionBox
    //
    struct YAE_API SampleGroupDescriptionBox : public FullBox
    {
      SampleGroupDescriptionBox():
        default_length_(0),
        default_sample_description_index_(0),
        entry_count_(0)
      {}

      void load(Mp4Context & mp4, IBitstream & bin) YAE_OVERRIDE;
      void to_json(Json::Value & out) const YAE_OVERRIDE;

      FourCC grouping_type_;
      uint32_t default_length_; // version 1+
      uint32_t default_sample_description_index_; // version 2+
      uint32_t entry_count_;

      // in version 0 of the entries the base classes for sample group
      // description entries are neither boxes nor have a size that is
      // signaled... there is no choice but consume it as opaque payload
      Data v0_sample_group_entries_;

      // version 1+
      std::vector<uint32_t> description_length_;
      std::vector<Data> sample_group_entries_;
    };

    //----------------------------------------------------------------
    // CopyrightBox
    //
    struct YAE_API CopyrightBox : public FullBox
    {
      CopyrightBox();

      void load(Mp4Context & mp4, IBitstream & bin) YAE_OVERRIDE;
      void to_json(Json::Value & out) const YAE_OVERRIDE;

      char language_[4]; // ISO-639-2/T language code
      std::string notice_; // UTF-8, possibly converted from UTF-16
    };

    //----------------------------------------------------------------
    // TrackSelectionBox
    //
    struct YAE_API TrackSelectionBox : public FullBox
    {
      TrackSelectionBox(): switch_group_(0) {}

      void load(Mp4Context & mp4, IBitstream & bin) YAE_OVERRIDE;
      void to_json(Json::Value & out) const YAE_OVERRIDE;

      int32_t switch_group_;
      std::list<FourCC> attribute_list_;
    };

    //----------------------------------------------------------------
    // KindBox
    //
    struct YAE_API KindBox : public FullBox
    {
      void load(Mp4Context & mp4, IBitstream & bin) YAE_OVERRIDE;
      void to_json(Json::Value & out) const YAE_OVERRIDE;

      std::string schemeURI_;
      std::string value_;
    };

    //----------------------------------------------------------------
    // XMLBox
    //
    struct YAE_API XMLBox : public FullBox
    {
      void load(Mp4Context & mp4, IBitstream & bin) YAE_OVERRIDE;
      void to_json(Json::Value & out) const YAE_OVERRIDE;

      std::string xml_; // UTF-8, possibly converted from UTF-16
    };

    //----------------------------------------------------------------
    // BinaryXMLBox
    //
    struct YAE_API BinaryXMLBox : public FullBox
    {
      void load(Mp4Context & mp4, IBitstream & bin) YAE_OVERRIDE;
      void to_json(Json::Value & out) const YAE_OVERRIDE;

      Data data_;
    };

    //----------------------------------------------------------------
    // ItemLocationBox
    //
    struct YAE_API ItemLocationBox : public FullBox
    {
      ItemLocationBox():
        offset_size_(0),
        length_size_(0),
        base_offset_size_(0),
        index_size_(0)
      {}

      void load(Mp4Context & mp4, IBitstream & bin) YAE_OVERRIDE;
      void to_json(Json::Value & out) const YAE_OVERRIDE;

      uint16_t offset_size_ : 4;
      uint16_t length_size_ : 4;
      uint16_t base_offset_size_ : 4;
      uint16_t index_size_ : 4;
      uint32_t item_count_;

      struct YAE_API Item
      {
        void to_json(Json::Value & out, uint32_t box_version) const;

        uint32_t item_ID_;
        uint16_t reserved_ : 12;
        uint16_t construction_method_ : 4;
        uint16_t data_reference_index_;
        uint64_t base_offset_;

        uint16_t extent_count_;
        std::vector<uint64_t> extent_index_;
        std::vector<uint64_t> extent_offset_;
        std::vector<uint64_t> extent_length_;
      };

      std::vector<Item> items_;
    };

    //----------------------------------------------------------------
    // PrimaryItemBox
    //
    struct YAE_API PrimaryItemBox : public FullBox
    {
      PrimaryItemBox(): item_ID_(0) {}

      void load(Mp4Context & mp4, IBitstream & bin) YAE_OVERRIDE;
      void to_json(Json::Value & out) const YAE_OVERRIDE;

      uint32_t item_ID_;
    };

    //----------------------------------------------------------------
    // ItemInfoEntryBox
    //
    struct YAE_API ItemInfoEntryBox : public FullBox
    {
      void load(Mp4Context & mp4, IBitstream & bin) YAE_OVERRIDE;
      void to_json(Json::Value & out) const YAE_OVERRIDE;

      uint32_t item_ID_;
      uint16_t item_protection_index_;
      FourCC item_type_; // version 2+
      std::string item_name_;
      std::string content_type_; // also item_uri_type
      std::string content_encoding_; // optional

      // version 1 only, optional:
      struct YAE_API ItemInfoExtension
      {
        virtual ~ItemInfoExtension() {}
        virtual void load(IBitstream & bin, std::size_t end_pos) = 0;
        virtual void to_json(Json::Value & out) const = 0;
      };

      typedef boost::shared_ptr<ItemInfoExtension> TExtensionPtr;

      struct YAE_API FDItemInfo : public ItemInfoExtension
      {
        void load(IBitstream & bin, std::size_t end_pos) YAE_OVERRIDE;
        void to_json(Json::Value & out) const YAE_OVERRIDE;

        std::string content_location_;
        std::string content_MD5_;
        uint64_t content_length_;
        uint64_t transfer_length_;
        uint8_t entry_count_;
        std::vector<uint32_t> group_ids_;
      };

      struct YAE_API Unknown : public ItemInfoExtension
      {
        void load(IBitstream & bin, std::size_t end_pos) YAE_OVERRIDE;
        void to_json(Json::Value & out) const YAE_OVERRIDE;

        Data data_;
      };

      FourCC extension_type_;
      TExtensionPtr extension_;
    };

    //----------------------------------------------------------------
    // ItemInfoBox
    //
    struct YAE_API ItemInfoBox : public ContainerEx
    {
      ItemInfoBox(): entry_count_(0) {}

      void load(Mp4Context & mp4, IBitstream & bin) YAE_OVERRIDE;

      uint32_t entry_count_;
    };

    //----------------------------------------------------------------
    // MetaboxRelationBox
    //
    struct YAE_API MetaboxRelationBox : public FullBox
    {
      void load(Mp4Context & mp4, IBitstream & bin) YAE_OVERRIDE;
      void to_json(Json::Value & out) const YAE_OVERRIDE;

      FourCC first_metabox_handler_type_;
      FourCC second_metabox_handler_type_;
      uint8_t metabox_relation_;
    };

    //----------------------------------------------------------------
    // ItemDataBox
    //
    struct YAE_API ItemDataBox : public Box
    {
      void load(Mp4Context & mp4, IBitstream & bin) YAE_OVERRIDE;
      void to_json(Json::Value & out) const YAE_OVERRIDE;

      Data data_;
    };

    //----------------------------------------------------------------
    // ItemReferenceBox
    //
    struct YAE_API ItemReferenceBox : public ContainerEx
    {
      template <typename TSize>
      struct SingleItemTypeReferenceBox : public Box
      {
        SingleItemTypeReferenceBox():
          from_item_ID_(0),
          reference_count_(0)
        {}

        void load(Mp4Context & mp4, IBitstream & bin)
        {
          Box::load(mp4, bin);
          from_item_ID_ = bin.read<TSize>();
          reference_count_ = bin.read<uint16_t>();
          for (uint16_t i = 0; i < reference_count_; ++i)
          {
            TSize to_item_ID = bin.read<TSize>();
            to_item_ID_.push_back(to_item_ID);
          }
        }

        void to_json(Json::Value & out) const
        {
          Box::to_json(out);
          out["from_item_ID"] = from_item_ID_;
          out["reference_count"] = reference_count_;
          yae::save(out["to_item_ID"], to_item_ID_);
        }

        TSize from_item_ID_;
        uint16_t reference_count_;
        std::vector<TSize> to_item_ID_;
      };

      void load(Mp4Context & mp4, IBitstream & bin) YAE_OVERRIDE;
    };

    //----------------------------------------------------------------
    // OriginalFormatBox
    //
    struct YAE_API OriginalFormatBox : public Box
    {
      void load(Mp4Context & mp4, IBitstream & bin) YAE_OVERRIDE;
      void to_json(Json::Value & out) const YAE_OVERRIDE;

      FourCC data_format_;
    };

    //----------------------------------------------------------------
    // SchemeTypeBox
    //
    struct YAE_API SchemeTypeBox : public FullBox
    {
      SchemeTypeBox(): scheme_version_(0) {}

      enum Flags {
        kSchemeUriPreset = 0x000001,
      };

      void load(Mp4Context & mp4, IBitstream & bin) YAE_OVERRIDE;
      void to_json(Json::Value & out) const YAE_OVERRIDE;

      FourCC scheme_type_;
      uint32_t scheme_version_;
      std::string scheme_uri_;
    };

    //----------------------------------------------------------------
    // FDItemInformationBox
    //
    struct YAE_API FDItemInformationBox : public ContainerEx
    {
      FDItemInformationBox(): entry_count_(0) {}

      void load(Mp4Context & mp4, IBitstream & bin) YAE_OVERRIDE;
      void to_json(Json::Value & out) const YAE_OVERRIDE;

      uint16_t entry_count_;
    };

    //----------------------------------------------------------------
    // FilePartitionBox
    //
    struct YAE_API FilePartitionBox : public FullBox
    {
      FilePartitionBox():
        item_ID_(0),
        packet_payload_size_(0),
        reserved_(0),
        FEC_encoding_ID_(0),
        FEC_instance_ID_(0),
        max_source_block_length_(0),
        encoding_symbol_length_(0),
        max_number_of_encoding_symbols_(0),
        entry_count_(0)
      {}

      void load(Mp4Context & mp4, IBitstream & bin) YAE_OVERRIDE;
      void to_json(Json::Value & out) const YAE_OVERRIDE;

      uint32_t item_ID_;
      uint16_t packet_payload_size_;
      uint8_t reserved_;
      uint8_t FEC_encoding_ID_;
      uint16_t FEC_instance_ID_;
      uint16_t max_source_block_length_;
      uint16_t encoding_symbol_length_;
      uint16_t max_number_of_encoding_symbols_;
      std::string scheme_specific_info_;
      uint32_t entry_count_;
      std::vector<uint16_t> block_count_;
      std::vector<uint32_t> block_size_;
    };

    //----------------------------------------------------------------
    // FECReservoirBox
    //
    struct YAE_API FECReservoirBox : public FullBox
    {
      FECReservoirBox(): entry_count_(0) {}

      void load(Mp4Context & mp4, IBitstream & bin) YAE_OVERRIDE;
      void to_json(Json::Value & out) const YAE_OVERRIDE;

      uint32_t entry_count_;
      std::vector<uint32_t> item_ID_;
      std::vector<uint32_t> symbol_count_;
    };

    //----------------------------------------------------------------
    // FDSessionGroupBox
    //
    struct YAE_API FDSessionGroupBox : public Box
    {
      void load(Mp4Context & mp4, IBitstream & bin) YAE_OVERRIDE;
      void to_json(Json::Value & out) const YAE_OVERRIDE;

      struct YAE_API SessionGroup
      {
        void load(IBitstream & bin, std::size_t end_pos);
        void to_json(Json::Value & out) const;

        uint8_t entry_count_;
        std::vector<uint32_t> group_ID_;

        uint16_t num_channels_in_session_group_;
        std::vector<uint32_t> hint_track_id_;
      };

      uint16_t num_session_groups_;
      std::vector<SessionGroup> session_groups_;
    };

    //----------------------------------------------------------------
    // GroupIdToNameBox
    //
    struct YAE_API GroupIdToNameBox : public FullBox
    {
      GroupIdToNameBox(): entry_count_(0) {}

      void load(Mp4Context & mp4, IBitstream & bin) YAE_OVERRIDE;
      void to_json(Json::Value & out) const YAE_OVERRIDE;

      uint16_t entry_count_;
      std::vector<uint32_t> group_ID_;
      std::vector<std::string> group_name_;
    };

    //----------------------------------------------------------------
    // FileReservoirBox
    //
    struct YAE_API FileReservoirBox : public FECReservoirBox {};

    //----------------------------------------------------------------
    // SubTrackInformationBox
    //
    struct YAE_API SubTrackInformationBox : public FullBox
    {
      SubTrackInformationBox():
        switch_group_(0),
        alternate_group_(0),
        sub_track_ID_(0)
      {}

      void load(Mp4Context & mp4, IBitstream & bin) YAE_OVERRIDE;
      void to_json(Json::Value & out) const YAE_OVERRIDE;

      uint16_t switch_group_;
      uint16_t alternate_group_;
      uint32_t sub_track_ID_;
      std::vector<FourCC> attribute_list_;
    };

    //----------------------------------------------------------------
    // SubTrackSampleGroupBox
    //
    struct YAE_API SubTrackSampleGroupBox : public FullBox
    {
      SubTrackSampleGroupBox(): entry_count_(0) {}

      void load(Mp4Context & mp4, IBitstream & bin) YAE_OVERRIDE;
      void to_json(Json::Value & out) const YAE_OVERRIDE;

      FourCC grouping_type_;
      uint16_t entry_count_;
      std::vector<uint32_t> group_description_index_;
    };

    //----------------------------------------------------------------
    // StereoVideoBox
    //
    struct YAE_API StereoVideoBox : public ContainerEx
    {
      StereoVideoBox():
        reserved_(0),
        single_view_allowed_(0),
        stereo_scheme_(0),
        length_(0)
      {}

      void load(Mp4Context & mp4, IBitstream & bin) YAE_OVERRIDE;
      void to_json(Json::Value & out) const YAE_OVERRIDE;

      uint32_t reserved_ : 30;
      uint32_t single_view_allowed_ : 2;
      uint32_t stereo_scheme_;
      uint32_t length_;
      std::vector<uint8_t> stereo_indication_type_;
    };

    //----------------------------------------------------------------
    // SegmentIndexBox
    //
    struct YAE_API SegmentIndexBox : public FullBox
    {
      SegmentIndexBox():
        reference_ID_(0),
        timescale_(0),
        earliest_presentation_time_(0),
        first_offset_(0),
        reference_count_(0)
      {}

      void load(Mp4Context & mp4, IBitstream & bin) YAE_OVERRIDE;
      void to_json(Json::Value & out) const YAE_OVERRIDE;

      struct YAE_API Reference
      {
        Reference();

        bool load(IBitstream & bin, std::size_t end_pos);
        void to_json(Json::Value & out) const;

        uint32_t reference_type_ : 1;
        uint32_t referenced_size_ : 31;
        uint32_t subsegment_duration_;
        uint32_t starts_with_SAP_ : 1;
        uint32_t SAP_type_ : 3;
        uint32_t SAP_delta_time_ : 28;
      };

      uint32_t reference_ID_;
      uint32_t timescale_;
      uint64_t earliest_presentation_time_;
      uint64_t first_offset_;
      uint16_t reserved_;
      uint16_t reference_count_;
      std::vector<Reference> references_;
    };

    //----------------------------------------------------------------
    // SubsegmentIndexBox
    //
    struct YAE_API SubsegmentIndexBox : public FullBox
    {
      SubsegmentIndexBox(): subsegment_count_(0) {}

      void load(Mp4Context & mp4, IBitstream & bin) YAE_OVERRIDE;
      void to_json(Json::Value & out) const YAE_OVERRIDE;

      struct YAE_API Subsegment
      {
        Subsegment(): range_count_(0) {}

        bool load(IBitstream & bin, std::size_t end_pos);
        void to_json(Json::Value & out) const;

        uint32_t range_count_;
        std::vector<uint8_t> levels_;
        std::vector<uint32_t> range_sizes_;
      };

      uint16_t subsegment_count_;
      std::vector<Subsegment> subsegments_;
    };

  }

}


#endif // YAE_MP4_H_
