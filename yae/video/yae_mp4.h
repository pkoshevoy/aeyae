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
#include "yae/utils/yae_time.h"
#include "yae/utils/yae_utils.h"
#include "yae/video/yae_iso14496.h"

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
  // namespace access:
  using yae::bitstream::IPayload;
  using yae::Bit;
  using yae::NBit;

  // forward declarations:
  struct Mp4Context;


  //----------------------------------------------------------------
  // ByteCode
  //
  template <int Size>
  struct ByteCode
  {
    enum { kSize = Size };

    ByteCode()
    { memset(data_, 0, sizeof(data_)); }

    inline void load(IBitstream & bin)
    { bin.read_bytes(data_, Size); }

    inline bool operator < (const ByteCode & other) const
    { return memcmp(data_, other.data_, Size) < 0; }

    uint8_t data_[Size];
  };

  //----------------------------------------------------------------
  // ByteCode16
  //
  struct YAE_API ByteCode16 : ByteCode<16> {};

  inline void save(Json::Value & json, const ByteCode16 & v)
  { json = yae::to_hex(v.data_, sizeof(v.data_)); }

  inline void load(const Json::Value & json, ByteCode16 & v)
  { yae::load_hex(v.data_, sizeof(v.data_), json.asCString()); }


  //----------------------------------------------------------------
  // CharCode
  //
  template <int NumChars>
  struct CharCode
  {
    enum { kNumChars = NumChars };

    CharCode(const char * code = "")
    { this->set(code); }

    inline void clear()
    { memset(str_, 0, sizeof(str_)); }

    inline bool empty() const
    { return memcmp(str_, "\0\0\0\0", NumChars) == 0; }

    inline void set(const char * code)
    {
      strncpy(str_, code, NumChars);
      str_[NumChars] = 0;
    }

    inline void load(IBitstream & bin)
    { bin.read_bytes(str_, NumChars); }

    inline bool same_as(const char * code) const
    { return strncmp(str_, code, NumChars) == 0; }

    inline bool same_as(uint32_t fourcc) const
    {
      return ((uint8_t(str_[0]) == (0xFF & (fourcc >> 24))) &&
              (uint8_t(str_[1]) == (0xFF & (fourcc >> 16))) &&
              (uint8_t(str_[2]) == (0xFF & (fourcc >> 8))) &&
              (uint8_t(str_[3]) == (0xFF & (fourcc >> 0))));
    }

    inline bool operator < (const CharCode & other) const
    { return strncmp(str_, other.str_, NumChars) < 0; }

    char str_[NumChars + 1];
  };


  //----------------------------------------------------------------
  // TwoCC
  //
  struct YAE_API TwoCC : public CharCode<2>
  {
    typedef CharCode<2> TBase;

    TwoCC(const char * two_cc = ""):
      TBase(two_cc)
    {}

    using TBase::set;

    inline uint16_t get() const
    { return yae::load_be16((const uint8_t *)str_); }

    inline void set(uint16_t cc)
    { yae::save_be16((uint8_t *)str_, cc); }
  };

  inline void save(Json::Value & json, const TwoCC & v)
  { json = v.str_; }

  inline void load(const Json::Value & json, TwoCC & v)
  { v.set(json.asCString()); }

  //----------------------------------------------------------------
  // FourCC
  //
  struct YAE_API FourCC : public CharCode<4>
  {
    typedef CharCode<4> TBase;

    FourCC(const char * fourcc = ""):
      TBase(fourcc)
    {}

    using TBase::set;

    inline uint32_t get() const
    { return yae::load_be32((const uint8_t *)str_); }

    inline void set(uint32_t fourcc)
    { yae::save_be32((uint8_t *)str_, fourcc); }
  };

  inline void save(Json::Value & json, const FourCC & v)
  { json = v.str_; }

  inline void load(const Json::Value & json, FourCC & v)
  { v.set(json.asCString()); }


  //----------------------------------------------------------------
  // read_mp4_box_size
  //
  // NOTE: this will preserve the current file position.
  //
  YAE_API bool
  read_mp4_box_size(FILE * file, uint64_t & box_size, FourCC & box_type);


  //----------------------------------------------------------------
  // iso_639_2t::PackedLang
  //
  // ISO-639-2/T language code:
  // unsigned int(5)[3] language;
  //
  // See ISO 639-2/T for the set of three character codes.
  // Each character is packed as the difference between its ASCII value
  // and 0x60. The code is confined to being three lower-case letters,
  // so these values are strictly positive.
  //
  namespace iso_639_2t
  {
    struct YAE_API PackedLang
    {
      void set(uint16_t packed_code);

      void load(IBitstream & bin);
      void save(IBitstream & bin) const;

      std::string str_;
    };
  }

  template<>
  inline void save(Json::Value & json, const yae::iso_639_2t::PackedLang & v)
  { json = v.str_; }

  template<>
  inline void load(const Json::Value & json, yae::iso_639_2t::PackedLang & v)
  { v.str_ = json.asString(); }


  // see ISO/IEC 14496-12:2015(E)
  namespace mp4
  {
    // forward declarations:
    struct Box;
    struct FileTypeBox;

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

    //----------------------------------------------------------------
    // TFileTypeBoxPtr
    //
    typedef boost::shared_ptr<FileTypeBox> TFileTypeBoxPtr;

    //----------------------------------------------------------------
    // BoxFactory
    //
    struct YAE_API BoxFactory : public std::map<FourCC, TBoxConstructor>
    {
      // helpers:
      inline TBoxConstructor get(const FourCC & fourcc) const
      {
        std::map<FourCC, TBoxConstructor>::const_iterator found =
          this->find(fourcc);
        return (found != this->end()) ? found->second : NULL;
      }

      inline TBoxConstructor get(const char * fourcc) const
      { return this->get(FourCC(fourcc)); }

      inline void add(const char * fourcc, TBoxConstructor box_constructor)
      {
        FourCC key(fourcc);
        YAE_ASSERT(!yae::has(*this, key));
        this->operator[](key) = box_constructor;
      }

      template <typename TBox>
      inline void add(const char * fourcc, TBox *(*box_ctor)(const char *))
      { this->add(fourcc, (TBoxConstructor)box_ctor); }
    };
  }


  //----------------------------------------------------------------
  // Mp4Context
  //
  struct YAE_API Mp4Context
  {
    Mp4Context():
      load_mdat_data_(false),
      parse_mdat_data_(false),
      per_sample_iv_size_(0),
      file_position_(0)
    {}

    // NOTE: end_pos is expressed in bits, not bytes:
    mp4::TBoxPtr parse(IBitstream & bin,
                       std::size_t end_pos,
                       mp4::BoxFactory * box_factory = NULL);

    //----------------------------------------------------------------
    // ParserCallbackInterface
    //
    struct YAE_API ParserCallbackInterface
    {
      virtual ~ParserCallbackInterface() {}

      // callback must return true to continue parsing,
      // callback may return false to stop the parser:
      virtual bool observe(const Mp4Context & mp4,
                           const mp4::TBoxPtr & box) = 0;
    };

    bool parse_file(const std::string & src_path,
                    ParserCallbackInterface * cb);

    bool is_quicktime() const;

    // MOTE: these helpers work only while parsing:
    bool is_ancestor_type(const char * fourcc) const;
    const mp4::Box * find_ancestor(const char * fourcc) const;

    // in order to be able to load the "senc" box we need to know
    // per_sample_iv_size, which may be specified by any of:
    // - TrackEncryptionBox (see "tenc")
    // - CencSampleAuxiliaryDataFormat (see "saiz", "saio")
    // - CencSampleEncryptionInformationGroupEntry (see "sbgp", "sbpd", "seig")
    // - externally specified Initialization Vector
    uint8_t get_per_sample_iv_size(uint32_t sample_index) const;
    void set_per_sample_iv_size(uint8_t n);

    bool load_mdat_data_;
    bool parse_mdat_data_;
    uint32_t per_sample_iv_size_;
    uint64_t file_position_;

    // these are set when discovered during parsing:
    mp4::TFileTypeBoxPtr ftyp_;
    mp4::TFileTypeBoxPtr styp_;

    // sometimes we need to know the parent box info
    // in order to parse the current box:
    std::list<const mp4::Box *> ancestors_;

    // push/pop boxes on the ancestor stack:
    struct YAE_API SetAncestor
    {
      Mp4Context & mp4_;

      SetAncestor(Mp4Context & mp4, const mp4::Box * box);
      ~SetAncestor();

    private:
      SetAncestor();
      SetAncestor & operator = (const SetAncestor &);
    };
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
        if (end_pos < (bin.position() + item_size))
        {
          break;
        }

        TData v = bin.read<TData>();
        data.push_back(v);
      }
    }

    LoadVec(std::size_t num_items,
            TData * data,
            IBitstream & bin,
            std::size_t end_pos)
    {
      memset(data, 0, sizeof(TData) * num_items);
      std::size_t item_size = sizeof(TData) * 8;
      for (std::size_t i = 0; i < num_items; ++i)
      {
        if (end_pos < (bin.position() + item_size))
        {
          break;
        }

        data[i] = bin.read<TData>();
      }
    }
  };


  namespace mp4
  {
    //----------------------------------------------------------------
    // Box
    //
    struct YAE_API Box
    {
      Box(): size_(0) {}
      virtual ~Box() {}

      virtual void load(Mp4Context & mp4, IBitstream & bin);

      // NOTE: this preserves the current bitstream position:
      void peek_box_type(Mp4Context & mp4, IBitstream & bin);

      // helper:
      virtual void to_json(Json::Value & out) const;

      // container boxes will override this:
      virtual const TBoxPtrVec * has_children() const
      { return NULL; }

      // helper:
      inline std::size_t num_children() const
      {
        const TBoxPtrVec * children = this->has_children();
        return children ? children->size() : 0;
      }

      // breadth-first search through given boxes:
      static const Box * find(const TBoxPtrVec & boxes, const char * fourcc);

      // breadth-first search through children, does not check 'this':
      virtual const Box * find_child(const char * fourcc) const;

      // check 'this', if not found then breadth-first search through children:
      template <typename TBox>
      const TBox *
      find(const char * fourcc) const
      {
        const Box * box =
          type_.same_as(fourcc) ? this : this->find_child(fourcc);
        if (!box)
        {
          return NULL;
        }

        const TBox * found = dynamic_cast<const TBox *>(box);
        YAE_ASSERT(found);
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

      uint32_t version_ : 8;
      uint32_t flags_ : 24;
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
        Mp4Context::SetAncestor set_ancestor(mp4, this);

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
        Mp4Context::SetAncestor set_ancestor(mp4, this);

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

        if (this->num_children())
        {
          Json::Value & children = out["children"];
          this->children_to_json(children);
        }
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
        FullBox::load(mp4, bin);

        const std::size_t end_pos = box_pos + Box::size_ * 8;
        entry_count_ = bin.read<TBoxCount>();
        TBase::load_children(mp4, bin, end_pos, entry_count_);
      }

      void to_json(Json::Value & out) const YAE_OVERRIDE
      {
        FullBox::to_json(out);

        out["entry_count"] = Json::UInt64(entry_count_);

        if (this->num_children())
        {
          Json::Value & children = out["children"];
          TBase::children_to_json(children);
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

      iso_639_2t::PackedLang language_;
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

      FourCC pre_defined_; // aka component type
      FourCC handler_type_; // aka component subtype

      // reserved, set to 0:
      uint32_t reserved_manufacturer_;
      uint32_t reserved_flags_;
      uint32_t reserved_flags_mask_;

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

      uint8_t reserved_[6]; // zero
      uint16_t data_reference_index_;
    };

    //----------------------------------------------------------------
    // AudioSampleEntryBox
    //
    // ISO/IEC 14496-12:2015(E), 12.2.3.2
    //
    struct YAE_API AudioSampleEntryBox : BoxWithChildren<SampleEntryBox>
    {
      AudioSampleEntryBox();

      void load(Mp4Context & mp4, IBitstream & bin) YAE_OVERRIDE;
      void to_json(Json::Value & out) const YAE_OVERRIDE;

      // version 0: AudioSampleEntry
      // version 1: AudioSampleEntryV1
      uint16_t entry_version_;

      uint16_t reserved1_[3]; // 0
      uint16_t channel_count_; // 2
      uint16_t sample_size_; // 16
      uint16_t pre_defined_; // 0
      uint16_t reserved2_; // 0

      // version 0: media_default_sample_rate << 16
      // version 1: 1 << 16
      uint32_t sample_rate_;
    };

    //----------------------------------------------------------------
    // VisualSampleEntryBox
    //
    struct YAE_API VisualSampleEntryBox : BoxWithChildren<SampleEntryBox>
    {
      VisualSampleEntryBox();

      void load(Mp4Context & mp4, IBitstream & bin) YAE_OVERRIDE;
      void to_json(Json::Value & out) const YAE_OVERRIDE;

      uint16_t pre_defined1_; // 0
      uint16_t reserved1_; // 0
      uint32_t pre_defined2_[3]; // 0, 0, 0
      uint16_t width_;
      uint16_t height_;
      uint32_t horizresolution_; // fixed point 16.16, 0x00480000 == 72 dpi
      uint32_t vertresolution_; // fixed point 16.16, 0x00480000 == 72 dpi
      uint32_t reserved2_; // 0
      uint16_t frame_count_; // 1
      std::string compressorname_;
      uint16_t depth_; // 24
      int16_t pre_defined3_; // -1;
    };

    //----------------------------------------------------------------
    // MpegSampleEntryBox
    //
    struct YAE_API MpegSampleEntryBox : BoxWithChildren<SampleEntryBox> {};


    //----------------------------------------------------------------
    // CleanApertureBox
    //
    struct YAE_API CleanApertureBox : public Box
    {
      CleanApertureBox():
        cleanApertureWidthN_(0),
        cleanApertureWidthD_(0),
        cleanApertureHeightN_(0),
        cleanApertureHeightD_(0),
        horizOffN_(0),
        horizOffD_(0),
        vertOffN_(0),
        vertOffD_(0)
      {}

      void load(Mp4Context & mp4, IBitstream & bin) YAE_OVERRIDE;
      void to_json(Json::Value & out) const YAE_OVERRIDE;

      uint32_t cleanApertureWidthN_;
      uint32_t cleanApertureWidthD_;
      uint32_t cleanApertureHeightN_;
      uint32_t cleanApertureHeightD_;
      uint32_t horizOffN_;
      uint32_t horizOffD_;
      uint32_t vertOffN_;
      uint32_t vertOffD_;
    };

    //----------------------------------------------------------------
    // PixelAspectRatioBox
    //
    struct YAE_API PixelAspectRatioBox : public Box
    {
      PixelAspectRatioBox():
        hSpacing_(0),
        vSpacing_(0)
      {}

      void load(Mp4Context & mp4, IBitstream & bin) YAE_OVERRIDE;
      void to_json(Json::Value & out) const YAE_OVERRIDE;

      uint32_t hSpacing_;
      uint32_t vSpacing_;
    };

    //----------------------------------------------------------------
    // ColourInformationBox
    //
    struct YAE_API ColourInformationBox : public Box
    {
      ColourInformationBox():
        colour_primaries_(0),
        transfer_characteristics_(0),
        matrix_coefficients_(0),
        full_range_flag_(0),
        reserved_(0)
      {}

      void load(Mp4Context & mp4, IBitstream & bin) YAE_OVERRIDE;
      void to_json(Json::Value & out) const YAE_OVERRIDE;

      FourCC colour_type_;

      // colour_type == "nclx"
      uint16_t colour_primaries_;
      uint16_t transfer_characteristics_;
      uint16_t matrix_coefficients_;
      uint8_t full_range_flag_ : 1;
      uint8_t reserved_ : 7;

      // colour_type == "rICC": restricted ICC profile
      // colour_type == "prof": unrestricted ICC profile
      // ICC.1:2010
      Data ICC_profile_;
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

      struct YAE_API Entry
      {
        Entry();

        void load(IBitstream & bin, uint32_t box_version);
        void to_json(Json::Value & out) const;

        uint64_t segment_duration_;
        int64_t media_time_;
        uint32_t media_rate_; // fixed point 16.16
      };

      uint32_t entry_count_;
      std::vector<Entry> entries_;
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
      SampleAuxiliaryInformationSizesBox();

      void load(Mp4Context & mp4, IBitstream & bin) YAE_OVERRIDE;
      void to_json(Json::Value & out) const YAE_OVERRIDE;

      // if ((flags & 1) == 1):
      FourCC aux_info_type_;
      uint32_t aux_info_type_parameter_;

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
      SampleAuxiliaryInformationOffsetsBox();

      void load(Mp4Context & mp4, IBitstream & bin) YAE_OVERRIDE;
      void to_json(Json::Value & out) const YAE_OVERRIDE;

      // if ((flags & 1) == 1):
      FourCC aux_info_type_;
      uint32_t aux_info_type_parameter_;

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

      inline bool base_data_offset_is_present() const
      { return (FullBox::flags_ & kBaseDataOffsetPresent) != 0; }

      inline bool sample_description_index_is_present() const
      { return (FullBox::flags_ & kSampleDescriptionIndexPresent) != 0; }

      inline bool default_sample_duration_is_present() const
      { return (FullBox::flags_ & kDefaultSampleDurationPresent) != 0; }

      inline bool default_sample_size_is_present() const
      { return (FullBox::flags_ & kDefaultSampleSizePresent) != 0; }

      inline bool default_sample_flags_are_present() const
      { return (FullBox::flags_ & kDefaultSampleFlagsPresent) != 0; }

      inline bool duration_is_empty() const
      { return (FullBox::flags_ & kDurationIsEmpty) != 0; }

      inline bool default_base_is_moof() const
      { return (FullBox::flags_ & kDefaultBaseIsMoof) != 0; }

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
    // MovieFragmentRandomAccessOffsetBox
    //
    struct YAE_API MovieFragmentRandomAccessOffsetBox : public FullBox
    {
      MovieFragmentRandomAccessOffsetBox(): size_(0) {}

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

      //----------------------------------------------------------------
      // Entry
      //
      struct YAE_API Entry
      {
        virtual ~Entry() {}
        virtual void load(Mp4Context & mp4, IBitstream & bin) = 0;
        virtual void to_json(Json::Value & out) const = 0;
      };

      //----------------------------------------------------------------
      // EntryPtr
      //
      typedef boost::shared_ptr<Entry> EntryPtr;

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
      std::vector<EntryPtr> sample_group_entries_;
    };

    //----------------------------------------------------------------
    // SampleGroupDescriptionEntry
    //
    typedef SampleGroupDescriptionBox::Entry SampleGroupDescriptionEntry;

    //----------------------------------------------------------------
    // DataSampleGroupDescriptionEntry
    //
    struct YAE_API DataSampleGroupDescriptionEntry :
      public SampleGroupDescriptionEntry
    {
      void load(Mp4Context & mp4, IBitstream & bin) YAE_OVERRIDE;
      void to_json(Json::Value & out) const YAE_OVERRIDE;

      Data data_;
    };

    //----------------------------------------------------------------
    // CencSampleEncryptionInformationGroupEntry
    //
    // grouping_type: "seig"
    //
    struct YAE_API CencSampleEncryptionInformationGroupEntry :
      public SampleGroupDescriptionEntry
    {
      CencSampleEncryptionInformationGroupEntry();

      void load(Mp4Context & mp4, IBitstream & bin) YAE_OVERRIDE;
      void to_json(Json::Value & out) const YAE_OVERRIDE;

      uint32_t reserved_ : 8;
      uint32_t crypt_byte_block_ : 4;
      uint32_t skip_byte_block_ : 4;
      uint32_t is_protected_ : 8;
      uint32_t per_sample_iv_size_ : 8;
      ByteCode16 KID_;

      // if (isProtected == 1 && Per_Sample_IV_Size == 0):
      // uint8_t constant_iv_size_
      // uint8_t constant_iv_[constant_iv_size_]
      Data constant_iv_;
    };

    //----------------------------------------------------------------
    // CencSampleAuxiliaryDataFormat
    //
    // ISO/IEC 23001-7:2016(E), 7.1
    //
    struct YAE_API CencSampleAuxiliaryDataFormat
    {
      CencSampleAuxiliaryDataFormat(): subsample_count_(0) {}

      void load(uint8_t per_sample_iv_size,
                uint8_t sample_info_size,
                IBitstream & bin);

      void to_json(Json::Value & out) const;

      struct YAE_API SubSample
      {
        SubSample(): bytes_clear_(0), bytes_protected_(0) {}
        uint16_t bytes_clear_;
        uint32_t bytes_protected_;
      };

      // Initialization Vector for the sample, unless a constant_IV
      // is present in the Track Encryption Box (‘tenc’)
      Data iv_;

      // if (sample_info_size > Per_Sample_IV_Size)
      uint16_t subsample_count_;
      std::vector<SubSample> subsample_;
    };

    //----------------------------------------------------------------
    // SampleEncryptionBox
    //
    struct YAE_API SampleEncryptionBox : FullBox
    {
      SampleEncryptionBox(): sample_count_(0) {}

      enum { kUseSubSampleEncryption = 0x2 };

      //----------------------------------------------------------------
      // SubSample
      //
      struct YAE_API SubSample
      {
        SubSample(): bytes_clear_(0), bytes_protected_(0) {}
        uint16_t bytes_clear_;
        uint32_t bytes_protected_;
      };

      //----------------------------------------------------------------
      // Sample
      //
      struct YAE_API Sample
      {
        Sample(): subsample_count_(0) {}
        Data iv_;

        // if flags & 0x000002
        uint16_t subsample_count_;
        std::vector<SubSample> subsample_;
      };

      void load(Mp4Context & mp4, IBitstream & bin) YAE_OVERRIDE;
      void to_json(Json::Value & out) const YAE_OVERRIDE;

      uint32_t sample_count_;
      std::vector<Sample> sample_;
    };


    //----------------------------------------------------------------
    // TrackEncryptionBox
    //
    struct YAE_API TrackEncryptionBox : FullBox
    {
      TrackEncryptionBox();

      void load(Mp4Context & mp4, IBitstream & bin) YAE_OVERRIDE;
      void to_json(Json::Value & out) const YAE_OVERRIDE;

      uint32_t reserved_ : 8;
      uint32_t default_crypt_byte_block_ : 4;
      uint32_t default_skip_byte_block_ : 4;
      uint32_t default_is_protected_ : 8;
      uint32_t default_per_sample_iv_size_ : 8;
      ByteCode16 default_KID_;

      // if (default_isProtected == 1 && default_Per_Sample_IV_Size == 0)
      uint8_t default_constant_iv_size_;
      Data default_constant_iv_;
    };


    //----------------------------------------------------------------
    // ProtectionSystemSpecificHeaderBox
    //
    struct YAE_API ProtectionSystemSpecificHeaderBox : FullBox
    {
      ProtectionSystemSpecificHeaderBox(): KID_count_(0), data_size_(0) {}

      void load(Mp4Context & mp4, IBitstream & bin) YAE_OVERRIDE;
      void to_json(Json::Value & out) const YAE_OVERRIDE;

      ByteCode16 SystemID_;

      // if version > 0:
      uint32_t KID_count_;
      std::vector<ByteCode16> KID_;

      uint32_t data_size_;
      Data data_;
    };


    //----------------------------------------------------------------
    // ID3v2Box
    //
    struct YAE_API ID3v2Box : public FullBox
    {
      void load(Mp4Context & mp4, IBitstream & bin) YAE_OVERRIDE;
      void to_json(Json::Value & out) const YAE_OVERRIDE;

      iso_639_2t::PackedLang language_;
      Data data_;
    };


    //----------------------------------------------------------------
    // CopyrightBox
    //
    struct YAE_API CopyrightBox : public FullBox
    {
      void load(Mp4Context & mp4, IBitstream & bin) YAE_OVERRIDE;
      void to_json(Json::Value & out) const YAE_OVERRIDE;

      iso_639_2t::PackedLang language_;
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

    //----------------------------------------------------------------
    // ProducerReferenceTimeBox
    //
    struct YAE_API ProducerReferenceTimeBox : public FullBox
    {
      ProducerReferenceTimeBox():
        reference_track_ID_(0),
        ntp_timestamp_(0),
        media_time_(0)
      {}

      void load(Mp4Context & mp4, IBitstream & bin) YAE_OVERRIDE;
      void to_json(Json::Value & out) const YAE_OVERRIDE;

      uint32_t reference_track_ID_;
      uint64_t ntp_timestamp_;
      uint64_t media_time_;
    };

    //----------------------------------------------------------------
    // RtpHintSampleEntryBox
    //
    struct YAE_API RtpHintSampleEntryBox : BoxWithChildren<SampleEntryBox>
    {
      RtpHintSampleEntryBox():
        hinttrackversion_(1),
        highestcompatibleversion_(1),
        maxpacketsize_(0)
      {}

      void load(Mp4Context & mp4, IBitstream & bin) YAE_OVERRIDE;
      void to_json(Json::Value & out) const YAE_OVERRIDE;

      uint16_t hinttrackversion_;
      uint16_t highestcompatibleversion_;
      uint32_t maxpacketsize_;
    };

    //----------------------------------------------------------------
    // TimescaleEntryBox
    //
    struct YAE_API TimescaleEntryBox : public Box
    {
      TimescaleEntryBox(): timescale_(0) {}

      void load(Mp4Context & mp4, IBitstream & bin) YAE_OVERRIDE;
      void to_json(Json::Value & out) const YAE_OVERRIDE;

      uint32_t timescale_;
    };

    //----------------------------------------------------------------
    // TimeOffsetBox
    //
    struct YAE_API TimeOffsetBox : public Box
    {
      TimeOffsetBox(): offset_(0) {}

      void load(Mp4Context & mp4, IBitstream & bin) YAE_OVERRIDE;
      void to_json(Json::Value & out) const YAE_OVERRIDE;

      uint32_t offset_;
    };

    //----------------------------------------------------------------
    // SRTPProcessBox
    //
    struct YAE_API SRTPProcessBox : public ContainerEx
    {
      void load(Mp4Context & mp4, IBitstream & bin) YAE_OVERRIDE;
      void to_json(Json::Value & out) const YAE_OVERRIDE;

      FourCC encryption_algorithm_rtp_;
      FourCC encryption_algorithm_rtcp_;
      FourCC integrity_algorithm_rtp_;
      FourCC integrity_algorithm_rtcp_;
    };

    //----------------------------------------------------------------
    // ObjectDescriptorBox
    //
    // this is defined in ISO/IEC 14496-14:2020(E)
    //
    struct YAE_API ObjectDescriptorBox : public FullBox
    {
      void load(Mp4Context & mp4, IBitstream & bin) YAE_OVERRIDE;
      void to_json(Json::Value & out) const YAE_OVERRIDE;

      // see ISO/IEC 14496-1:2010(E), 7.2.6.3  ObjectDescriptor
      Data data_;
    };

    //----------------------------------------------------------------
    // VideoMediaHeaderBox
    //
    struct YAE_API VideoMediaHeaderBox : public FullBox
    {
      VideoMediaHeaderBox();

      void load(Mp4Context & mp4, IBitstream & bin) YAE_OVERRIDE;
      void to_json(Json::Value & out) const YAE_OVERRIDE;

      uint16_t graphicsmode_;
      uint16_t opcolor_[3];
    };

    //----------------------------------------------------------------
    // SoundMediaHeaderBox
    //
    struct YAE_API SoundMediaHeaderBox : public FullBox
    {
      SoundMediaHeaderBox():
        balance_(0),
        reserved_(0)
      {}

      void load(Mp4Context & mp4, IBitstream & bin) YAE_OVERRIDE;
      void to_json(Json::Value & out) const YAE_OVERRIDE;

      uint16_t balance_;
      uint16_t reserved_;
    };

    //----------------------------------------------------------------
    // TextBox
    //
    struct YAE_API TextBox : public Box
    {
      void load(Mp4Context & mp4, IBitstream & bin) YAE_OVERRIDE;
      void to_json(Json::Value & out) const YAE_OVERRIDE;

      std::string data_;
    };

    //----------------------------------------------------------------
    // TextFullBox
    //
    struct YAE_API TextFullBox : public FullBox
    {
      void load(Mp4Context & mp4, IBitstream & bin) YAE_OVERRIDE;
      void to_json(Json::Value & out) const YAE_OVERRIDE;

      std::string data_;
    };

    //----------------------------------------------------------------
    // DataFullBox
    //
    struct YAE_API DataFullBox : public FullBox
    {
      void load(Mp4Context & mp4, IBitstream & bin) YAE_OVERRIDE;
      void to_json(Json::Value & out) const YAE_OVERRIDE;

      Data data_;
    };

    //----------------------------------------------------------------
    // DASHEventMessageBox
    //
    // see ISO/IEC 23009-1:2019(E), 5.10.3.3.2
    //
    struct YAE_API DASHEventMessageBox : public FullBox
    {
      DASHEventMessageBox():
        timescale_(0),
        presentation_(0),
        event_duration_(0),
        id_(0)
      {}

      void load(Mp4Context & mp4, IBitstream & bin) YAE_OVERRIDE;
      void to_json(Json::Value & out) const YAE_OVERRIDE;

      std::string scheme_id_uri_;
      std::string value_;
      uint32_t timescale_;

      // version 0: 32-bit presentation_time_delta
      // version 1: 64-bit presentation_time
      uint64_t presentation_;

      uint32_t event_duration_;
      uint32_t id_;
      Data message_data_;
    };

    //----------------------------------------------------------------
    // AVCSampleEntryBox
    //
    struct YAE_API AVCSampleEntryBox : VisualSampleEntryBox
    {};

    //----------------------------------------------------------------
    // AVCConfigurationBox
    //
    struct YAE_API AVCConfigurationBox : public Box
    {
      void load(Mp4Context & mp4, IBitstream & bin) YAE_OVERRIDE;
      void to_json(Json::Value & out) const YAE_OVERRIDE;

      yae::iso14496::AVCDecoderConfigurationRecord cfg_;
    };

    //----------------------------------------------------------------
    // ESDSBox
    //
    // ISO/IEC 14496-14:2018(E), 6.7.2
    //
    struct YAE_API ESDSBox : public FullBox
    {
      void load(Mp4Context & mp4, IBitstream & bin) YAE_OVERRIDE;
      void to_json(Json::Value & out) const YAE_OVERRIDE;

      yae::iso14496::ES_Descriptor es_;
    };

    //----------------------------------------------------------------
    // AC3SpecificBox
    //
    // ETSI TS 102 366 V1.3.1 (2014-08), F.4.1
    //
    struct YAE_API AC3SpecificBox : public Box
    {
      void load(Mp4Context & mp4, IBitstream & bin) YAE_OVERRIDE;
      void to_json(Json::Value & out) const YAE_OVERRIDE;

      Bit<2> fscod_;
      Bit<5> bsid_;
      Bit<3> bsmod_;
      Bit<3> acmod_;
      Bit<1> lfeon_;
      Bit<5> bit_rate_code_;
      Bit<5, 0> reserved_;
    };

    //----------------------------------------------------------------
    // EC3IndependentSubstream
    //
    // ETSI TS 102 366 V1.3.1 (2014-08), F.6.1
    //
    struct YAE_API EC3IndependentSubstream : public IPayload
    {
      Bit<2> fscod_;
      Bit<5> bsid_;
      Bit<1, 0> reserved1_;
      Bit<1> asvc_;
      Bit<3> bsmod_;
      Bit<3> acmod_;
      Bit<1> lfeon_;
      Bit<3, 0> reserved2_;
      Bit<4> num_dep_sub_;

      Bit<9> chan_loc_; // if (num_dep_sub > 0)
      Bit<1, 0> reserved_; // if (!num_dep_sub)

      virtual void save(IBitstream & bin) const;
      virtual bool load(IBitstream & bin);

      void save(Json::Value & json) const;
    };

    //----------------------------------------------------------------
    // EC3SpecificBox
    //
    // ETSI TS 102 366 V1.3.1 (2014-08), F.6.1
    //
    struct YAE_API EC3SpecificBox : public Box
    {
      void load(Mp4Context & mp4, IBitstream & bin) YAE_OVERRIDE;
      void to_json(Json::Value & out) const YAE_OVERRIDE;

      // the data rate of the Enhanced AC-3 bit stream in kbit/s.
      // If the Enhanced AC-3 bit stream has a variable bit rate,
      // then this field shall indicate the maximum data rate
      // of the bit stream measured over the complete duration
      // of the bit stream.
      Bit<13> data_rate_;

      // the number of independent substreams that are present
      // in the Enhanced AC-3 bit stream. The value of this field
      // shall be equal to the substreamID value of the last
      // independent substream of the bit stream.
      Bit<3> num_ind_sub_;

      std::vector<EC3IndependentSubstream> ind_sub_;
    };


    //----------------------------------------------------------------
    // ChannelLayoutBox
    //
    // ISO/IEC 14496-12:2015(E), 12.2.4.1
    //
    struct YAE_API ChannelLayoutBox : public FullBox
    {
      ChannelLayoutBox();

      struct YAE_API Layout
      {
        Layout();

        bool load(IBitstream & bin, std::size_t end_pos);
        void to_json(Json::Value & out) const;

        uint8_t speaker_position_;

        // if speaker_position == 126) // explicit position
        int16_t azimuth_;
        int8_t elevation_;
      };

      void load(Mp4Context & mp4, IBitstream & bin) YAE_OVERRIDE;
      void to_json(Json::Value & out) const YAE_OVERRIDE;

      enum Structure
      {
        channelStructured = 1,
        objectStructured = 2
      };

      uint8_t stream_structure_;

      // if (stream_structure & channelStructured)
      uint8_t definedLayout_;

      // if definedLayout == 0:
      // (channelCount comes from the sample entry ...
      //  or we can just read until the end of the box)
      std::vector<Layout> layout_;

      // if definedLayout != 0:
      uint64_t omittedChannelsMap_;

      // if (stream_structure & objectStructured)
      uint8_t object_count_;
    };


    //----------------------------------------------------------------
    // DownMixInstructionsBox
    //
    // ISO/IEC 14496-12:2015(E), 12.2.5
    //
    struct YAE_API DownMixInstructionsBox : public FullBox
    {
      DownMixInstructionsBox();

      void load(Mp4Context & mp4, IBitstream & bin) YAE_OVERRIDE;
      void to_json(Json::Value & out) const YAE_OVERRIDE;

      uint8_t targetLayout_;

      uint8_t reserved_ : 1;
      uint8_t targetChannelCount_ : 7;

      uint8_t in_stream_ : 1;
      uint8_t downmix_ID_ : 7;

      // for (i = 1; i <= targetChannelCount; i++)
      //   for (j = 1; j <= baseChannelCount; j++)
      //     bit(4) bs_downmix_coefficient;
      //
      // If targetChannelCount*baseChannelCount is odd,
      // the box is padded with 4 bits set to 0xF
      //
      std::vector<uint8_t> bs_downmix_coefficient_;
    };


    //----------------------------------------------------------------
    // LoudnessBaseBox
    //
    // ISO/IEC 14496-12:2015(E), 12.2.7.2
    //
    struct YAE_API LoudnessBaseBox : public FullBox
    {
      LoudnessBaseBox();

      void load(Mp4Context & mp4, IBitstream & bin) YAE_OVERRIDE;
      void to_json(Json::Value & out) const YAE_OVERRIDE;

      uint16_t reserved_ : 3;
      uint16_t downmix_ID_ : 7; // matching downmix
      uint16_t DRC_set_ID_ : 6; // to match a DRC box

      int16_t bs_sample_peak_level_ : 12;
      int16_t bs_true_peak_level_ : 12;

      uint8_t measurement_system_for_TP_ : 4;
      uint8_t reliability_for_TP_ : 4;
      uint8_t measurement_count_;

      struct YAE_API Measurement
      {
        bool load(IBitstream & bin, std::size_t end_pos);
        void to_json(Json::Value & out) const;

        uint8_t method_definition_;
        uint8_t method_value_;
        uint8_t measurement_system_ : 4;
        uint8_t reliability_ : 4;
      };

      std::vector<Measurement> measurement_;
    };


    //----------------------------------------------------------------
    // XMLMetaDataSampleEntryBox
    //
    struct YAE_API XMLMetaDataSampleEntryBox : BoxWithChildren<SampleEntryBox>
    {
      void load(Mp4Context & mp4, IBitstream & bin) YAE_OVERRIDE;
      void to_json(Json::Value & out) const YAE_OVERRIDE;

      std::string content_encoding_;
      std::string namespace_;
      std::string schema_location_;
    };


    //----------------------------------------------------------------
    // TextMetaDataSampleEntryBox
    //
    struct YAE_API TextMetaDataSampleEntryBox : BoxWithChildren<SampleEntryBox>
    {
      void load(Mp4Context & mp4, IBitstream & bin) YAE_OVERRIDE;
      void to_json(Json::Value & out) const YAE_OVERRIDE;

      std::string content_encoding_;
      std::string mime_format_;
    };


    //----------------------------------------------------------------
    // URIMetaSampleEntryBox
    //
    struct YAE_API URIMetaSampleEntryBox : BoxWithChildren<SampleEntryBox> {};


    //----------------------------------------------------------------
    // HintMediaHeaderBox
    //
    struct YAE_API HintMediaHeaderBox : public FullBox
    {
      HintMediaHeaderBox();

      void load(Mp4Context & mp4, IBitstream & bin) YAE_OVERRIDE;
      void to_json(Json::Value & out) const YAE_OVERRIDE;

      uint16_t maxPDUsize_;
      uint16_t avgPDUsize_;
      uint32_t maxbitrate_;
      uint32_t avgbitrate_;
      uint32_t reserved_;
    };

    //----------------------------------------------------------------
    // XMLSubtitleSampleEntryBox
    //
    struct YAE_API XMLSubtitleSampleEntryBox : BoxWithChildren<SampleEntryBox>
    {
      void load(Mp4Context & mp4, IBitstream & bin) YAE_OVERRIDE;
      void to_json(Json::Value & out) const YAE_OVERRIDE;

      std::string namespace_;
      std::string schema_location_;
      std::string auxiliary_mime_types_;
    };

    //----------------------------------------------------------------
    // PlainTextSampleEntryBox
    //
    struct YAE_API PlainTextSampleEntryBox : BoxWithChildren<SampleEntryBox> {};

    //----------------------------------------------------------------
    // VTTCueBox
    //
    struct YAE_API VTTCueBox : BoxWithChildren<Box> {};

    //----------------------------------------------------------------
    // CueSourceIDBox
    //
    // ISO/IEC 14496-30:2018(E)
    //
    struct YAE_API CueSourceIDBox : Box
    {
      CueSourceIDBox(): Box(), source_id_(0) {}

      void load(Mp4Context & mp4, IBitstream & bin) YAE_OVERRIDE;
      void to_json(Json::Value & out) const YAE_OVERRIDE;

      int32_t source_id_;
    };

  }

  // some mp4 files contain quicktime atoms:
  namespace qtff
  {

    //----------------------------------------------------------------
    // WellKnownTypes
    //
    enum WellKnownTypes
    {
      Reserved = 0,
      UTF_8 = 1,
      UTF_16 = 2,
      S_JIS = 3,
      UTF_8_sort = 4,
      UTF_16_sort = 5,
      JPEG = 13,
      PNG = 14,
      BE_Signed_Int = 21, // 1, 2, 3, or 4 bytes
      BE_Unsigned_Int = 22, // 1, 2, 3, or 4 bytes
      BE_Float32 = 23, // IEEE754
      BE_Float64 = 24, // IEEE754
      BMP = 27, // Windows bitmap format graphics
      QuickTimeMetadataAtom = 28
    };

    //----------------------------------------------------------------
    // CountryList
    //
    struct YAE_API CountryList
    {
      CountryList(): count_(0) {}

      void load(IBitstream & bin);

      uint16_t count_;
      std::vector<TwoCC> countries_; // ISO 3166 country codes
    };

    //----------------------------------------------------------------
    // CountryListAtom
    //
    struct YAE_API CountryListAtom : public yae::mp4::FullBox
    {
      CountryListAtom(): entry_count_(0) {}

      void load(Mp4Context & mp4, IBitstream & bin) YAE_OVERRIDE;
      void to_json(Json::Value & out) const YAE_OVERRIDE;

      uint32_t entry_count_;
      std::vector<CountryList> entries_;
    };

    //----------------------------------------------------------------
    // LanguageList
    //
    struct YAE_API LanguageList
    {
      LanguageList(): count_(0) {}

      void load(IBitstream & bin);

      uint16_t count_;
      std::vector<iso_639_2t::PackedLang> languages_;
    };

    //----------------------------------------------------------------
    // LanguageListAtom
    //
    struct YAE_API LanguageListAtom : public yae::mp4::FullBox
    {
      LanguageListAtom(): entry_count_(0) {}

      void load(Mp4Context & mp4, IBitstream & bin) YAE_OVERRIDE;
      void to_json(Json::Value & out) const YAE_OVERRIDE;

      uint32_t entry_count_;
      std::vector<LanguageList> entries_;
    };

    //----------------------------------------------------------------
    // MetadataItemKeyAtom
    //
    struct YAE_API MetadataItemKeyAtom : public yae::mp4::TextBox {};

    //----------------------------------------------------------------
    // MetadataItemKeysAtom
    //
    struct YAE_API MetadataItemKeysAtom : public yae::mp4::ContainerList32
    {
      void load(Mp4Context & mp4, IBitstream & bin) YAE_OVERRIDE;
    };

    //----------------------------------------------------------------
    // NameAtom
    //
    struct YAE_API NameAtom : public yae::mp4::TextFullBox {};

    //----------------------------------------------------------------
    // ItemInfoAtom
    //
    struct YAE_API ItemInfoAtom : public yae::mp4::FullBox
    {
      ItemInfoAtom(): item_id_(0) {}

      void load(Mp4Context & mp4, IBitstream & bin) YAE_OVERRIDE;
      void to_json(Json::Value & out) const YAE_OVERRIDE;

      uint32_t item_id_;
    };

    //----------------------------------------------------------------
    // TypeIndicator
    //
    // The type indicator is formed of four bytes split between two fields.
    // The first byte indicates the set of types from which the type is drawn.
    // The second through fourth byte forms the second field
    // and its interpretation depends upon the value in the first field.
    //
    struct YAE_API TypeIndicator
    {
      TypeIndicator():
        indicator_byte_(0),
        well_known_type_(0)
      {}

      void load(IBitstream & bin);

      // The indicator byte must have a value of 0,
      // meaning the type is drawn from the well-known set of types.
      // All other values are reserved.
      uint32_t indicator_byte_ : 8;

      // If the type indicator byte is 0,
      // the following 24 bits hold the well-known type.
      // Please refer to the list of Well-Known Types...
      uint32_t well_known_type_ : 24;
    };

    //----------------------------------------------------------------
    // LocaleIndicator
    //
    // The locale indicator is formatted as a four-byte value.
    //
    // It is formed from two two-byte values: a country indicator,
    // and a language indicator. In each case, the two-byte field
    // has the possible values shown in Table 3-3.
    //
    // Table 3-3, Country and language indicators:
    //
    //   0:
    //     This atom provides the default value of this datum
    //     for any locale not explicitly listed.
    //
    //   1 to 255:
    //     The value is an index into the country or language list
    //     (the upper byte is 0).
    //
    //   otherwise:
    //     The value is an ISO 3166 code (for the country code)
    //     or a packed ISO 639-2/T code (for the language).
    //
    struct YAE_API LocaleIndicator
    {
      LocaleIndicator():
        country_(0),
        language_(0)
      {}

      void load(IBitstream & bin);

      uint16_t country_;
      uint16_t language_;
    };

    //----------------------------------------------------------------
    // MetadataValueAtom
    //
    struct YAE_API MetadataValueAtom : public yae::mp4::Box
    {
      void load(Mp4Context & mp4, IBitstream & bin) YAE_OVERRIDE;
      void to_json(Json::Value & out) const YAE_OVERRIDE;

      TypeIndicator type_indicator_;
      LocaleIndicator locale_indicator_;
      Data value_;
    };

    //----------------------------------------------------------------
    // MetadataItemAtom
    //
    struct YAE_API MetadataItemAtom : public yae::mp4::Container
    {
      void load(Mp4Context & mp4, IBitstream & bin) YAE_OVERRIDE;
    };

    //----------------------------------------------------------------
    // MetadataItemListAtom
    //
    struct YAE_API MetadataItemListAtom : public yae::mp4::Container
    {
      void load(Mp4Context & mp4, IBitstream & bin) YAE_OVERRIDE;
    };

    //----------------------------------------------------------------
    // ApertureDimensionsAtom
    //
    struct YAE_API ApertureDimensionsAtom : public yae::mp4::FullBox
    {
      ApertureDimensionsAtom():
        width_(0),
        height_(0)
      {}

      void load(Mp4Context & mp4, IBitstream & bin) YAE_OVERRIDE;
      void to_json(Json::Value & out) const YAE_OVERRIDE;

      // fixed point 16.16 aperture in pixels:
      uint32_t width_;
      uint32_t height_;
    };

    //----------------------------------------------------------------
    // BaseMediaInfoAtom
    //
    struct YAE_API BaseMediaInfoAtom : public yae::mp4::FullBox
    {
      BaseMediaInfoAtom();

      void load(Mp4Context & mp4, IBitstream & bin) YAE_OVERRIDE;
      void to_json(Json::Value & out) const YAE_OVERRIDE;

      uint16_t graphicsmode_;
      uint16_t opcolor_[3];
      uint16_t balance_;
      uint16_t reserved_;
    };

    //----------------------------------------------------------------
    // SampleDescriptionAtom
    //
    struct YAE_API SampleDescriptionAtom : public yae::mp4::Container
    {
      SampleDescriptionAtom();

      // helper, returns box start position:
      std::size_t load_base(Mp4Context & mp4, IBitstream & bin);

      void load(Mp4Context & mp4, IBitstream & bin) YAE_OVERRIDE;
      void to_json(Json::Value & out) const YAE_OVERRIDE;

      uint8_t reserved_[6];
      uint16_t data_reference_index_;
    };

    //----------------------------------------------------------------
    // SoundSampleDescription
    //
    // version 0, and version 1:
    //
    struct YAE_API SoundSampleDescription : SampleDescriptionAtom
    {
      SoundSampleDescription();

      void load(Mp4Context & mp4, IBitstream & bin) YAE_OVERRIDE;
      void to_json(Json::Value & out) const YAE_OVERRIDE;

      // version 0:
      uint16_t version_;
      uint16_t revision_level_;
      uint32_t vendor_;
      uint16_t num_channels_;
      uint16_t sample_size_; // bits
      int16_t compression_id_;
      uint16_t packet_size_;
      uint32_t sample_rate_; // 16.16 fixed point

      // version 1:
      uint32_t samples_per_packet_;
      uint32_t bytes_per_packet_;
      uint32_t bytes_per_frame_;
      uint32_t bytes_per_sample_;
    };

    //----------------------------------------------------------------
    // SoundSampleDescriptionV2
    //
    struct YAE_API SoundSampleDescriptionV2 : SampleDescriptionAtom
    {
      SoundSampleDescriptionV2();

      void load(Mp4Context & mp4, IBitstream & bin) YAE_OVERRIDE;
      void to_json(Json::Value & out) const YAE_OVERRIDE;

      enum FormatSpecificFlags {
        kAudioFormatFlagIsFloat = (1 << 0),
        kAudioFormatFlagIsBigEndian = (1 << 1),
        kAudioFormatFlagIsSignedInteger = (1 << 2),
        kAudioFormatFlagIsPacked = (1 << 3),
        kAudioFormatFlagIsAlignedHigh = (1 << 4),
        kAudioFormatFlagIsNonInterleaved = (1 << 5),
        kAudioFormatFlagIsNonMixable = (1 << 6),
        kAudioFormatFlagsAreAllClear = (1 << 31),

        kLinearPCMFormatFlagsSampleFractionShift = 7,
        kLinearPCMFormatFlagsSampleFractionMask = (0x3F << 7),

        kAppleLosslessFormatFlag_16BitSourceData = 1,
        kAppleLosslessFormatFlag_20BitSourceData = 2,
        kAppleLosslessFormatFlag_24BitSourceData = 3,
        kAppleLosslessFormatFlag_32BitSourceData = 4
      };

      // version 0:
      uint16_t version_;
      uint16_t revision_level_;
      uint32_t vendor_;
      uint16_t always3_; // former num channels
      uint16_t always16_; // former sample size in bits
      int16_t alwaysNegative2_; // former compression id
      uint16_t always0_; // former packet size
      uint32_t always65536_; // former sample rate

      // version 1:
      uint32_t sizeOfStructOnly_; // former samples per packet
      double audioSampleRate_; // bytes per packet
      uint32_t numAudioChannels_; // former bytes per frame
      uint32_t always7F000000_; // former bytes per sample

      // version 2:
      uint32_t constBitsPerChannel_;
      uint32_t formatSpecificFlags_; // LPCM flag values
      uint32_t constBytesPerAudioPacket_;
      uint32_t constLPCMFramesPerAudioPacket_;
    };

    //----------------------------------------------------------------
    // TimecodeSampleDescAtom
    //
    struct YAE_API TimecodeSampleDescAtom : SampleDescriptionAtom
    {
      TimecodeSampleDescAtom();

      void load(Mp4Context & mp4, IBitstream & bin) YAE_OVERRIDE;
      void to_json(Json::Value & out) const YAE_OVERRIDE;

      enum Flags {
        kDropFrame = 0x0001,
        k24HourMax = 0x0002,
        kNegativeTimesOK = 0x0004,
        kCounter = 0x0008,
      };

      uint32_t reserved1_;
      uint32_t flags_;
      uint32_t timescale_;
      uint32_t frame_duration_;
      uint32_t frames_per_second_ : 8;
      uint32_t reserved2_ : 24;
    };

    //----------------------------------------------------------------
    // TimecodeMediaInfoAtom
    //
    struct YAE_API TimecodeMediaInfoAtom : public yae::mp4::FullBox
    {
      TimecodeMediaInfoAtom();

      void load(Mp4Context & mp4, IBitstream & bin) YAE_OVERRIDE;
      void to_json(Json::Value & out) const YAE_OVERRIDE;

      enum TextFace {
        kBold = 0x0001,
        kItalic = 0x0002,
        kUnderline = 0x0004,
        kOutline = 0x0008,
        kShadow = 0x0010,
        kCondense = 0x0020,
        kExtend = 0x0040,
      };

      uint16_t text_font_;
      uint16_t text_face_;
      uint16_t text_size_;
      uint16_t reserved_;

      // RGB:
      uint16_t text_color_[3];
      uint16_t background_color_[3];

      std::string font_name_;
    };

    //----------------------------------------------------------------
    // siDecompressionParamAtom
    //
    struct YAE_API siDecompressionParamAtom :
      public yae::mp4::BoxWithChildren<yae::mp4::Box>
    {};

    //----------------------------------------------------------------
    // FormatAtom
    //
    struct YAE_API FormatAtom : public yae::mp4::Box
    {
      void load(Mp4Context & mp4, IBitstream & bin) YAE_OVERRIDE;
      void to_json(Json::Value & out) const YAE_OVERRIDE;

      FourCC data_format_;
    };

    //----------------------------------------------------------------
    // ESDSAtom
    //
    struct YAE_API ESDSAtom : public yae::mp4::Box
    {
      ESDSAtom():
        version_(0)
      {}

      void load(Mp4Context & mp4, IBitstream & bin) YAE_OVERRIDE;
      void to_json(Json::Value & out) const YAE_OVERRIDE;

      uint32_t version_;
      yae::iso14496::ES_Descriptor es_;
    };

    //----------------------------------------------------------------
    // AudioChannelLayoutAtom
    //
    struct YAE_API AudioChannelLayoutAtom : public yae::mp4::FullBox
    {
      void load(Mp4Context & mp4, IBitstream & bin) YAE_OVERRIDE;
      void to_json(Json::Value & out) const YAE_OVERRIDE;

      // AudioChannelLayout as defined in CoreAudioTypes.h
      Data data_;
    };
  }

  template <>
  inline void save(Json::Value & json, const yae::qtff::CountryList & v)
  {
    yae::save(json["count"], v.count_);
    yae::save(json["countries"], v.countries_);
  }

  template <>
  inline void load(const Json::Value & json, yae::qtff::CountryList & v)
  {
    yae::load(json["count"], v.count_);
    yae::load(json["countries"], v.countries_);
  }

  template <>
  inline void save(Json::Value & json, const yae::qtff::LanguageList & v)
  {
    yae::save(json["count"], v.count_);
    yae::save(json["languages"], v.languages_);
  }

  template <>
  inline void load(const Json::Value & json, yae::qtff::LanguageList & v)
  {
    yae::load(json["count"], v.count_);
    yae::load(json["languages"], v.languages_);
  }

  template <>
  inline void save(Json::Value & json, const yae::qtff::TypeIndicator & v)
  {
    yae::save(json["indicator_byte"], uint32_t(v.indicator_byte_));
    yae::save(json["well_known_type"], uint32_t(v.well_known_type_));
  }

  template <>
  inline void load(const Json::Value & json, yae::qtff::TypeIndicator & v)
  {
    uint32_t temp = 0;
    yae::load(json["indicator_byte"], temp);
    v.indicator_byte_ = temp;

    yae::load(json["well_known_type"], temp);
    v.well_known_type_ = temp;
  }

  template <>
  inline void save(Json::Value & json, const yae::qtff::LocaleIndicator & v)
  {
    yae::save(json["country"], v.country_);
    yae::save(json["language"], v.language_);
  }

  template <>
  inline void load(const Json::Value & json, yae::qtff::LocaleIndicator & v)
  {
    yae::load(json["country"], v.country_);
    yae::load(json["language"], v.language_);
  }

  //----------------------------------------------------------------
  // get_timeline
  //
  YAE_API void
  get_timeline(const mp4::TBoxPtrVec & top_level_boxes,
               yae::Timeline & timeline);

}


#endif // YAE_MP4_H_
