// -*- Mode: c++; tab-width: 8; c-basic-offset: 2; indent-tabs-mode: nil -*-
// NOTE: the first line of this file sets up source code indentation rules
// for Emacs; it is also a hint to anyone modifying this file.

// Created   : Sun Mar 30 12:09:04 PM MDT 2025
// Copyright : Pavel Koshevoy
// License   : MIT -- http://www.opensource.org/licenses/mit-license.php

// aeyae:
#include "yae_mp4.h"

// namespace access:
using namespace yae;
using namespace yae::iso_14496_12;


//----------------------------------------------------------------
// Box::load
//
void
Box::load(Mp4Context & mp4, IBitstream & bin)
{
  (void)mp4;
  uint32_t size32 = bin.read_bits(32);
  bin.read_bytes(type_.str_, 4);

  if (size32 == 1)
  {
    // load largesize:
    size_ = bin.read_bits(64);
  }
  else if (size32 == 0)
  {
    // box extends to the end of file:
    size_ = 8 + bin.bytes_left();
  }
  else
  {
    size_ = size32;
  }

  if (type_.same_as("uuid"))
  {
    // extended type:
    uuid_ = bin.read_bytes(16);
  }
}

//----------------------------------------------------------------
// Box::to_json
//
void
Box::to_json(Json::Value & out) const
{
  out["Box.size"] = Json::UInt64(size_);
  out["Box.type"] = type_.str_;

  if (uuid_)
  {
    std::size_t uuid_sz = uuid_.size();
    out["box.uuid"] = yae::to_hex(uuid_.get(), uuid_sz, uuid_sz);
  }
}

//----------------------------------------------------------------
// Box::find_child
//
// breadth-first search through children, does not check 'this'
//
const Box *
Box::find_child(const char * fourcc) const
{
  if (!this->has_children())
  {
    return NULL;
  }

  const TBoxPtrVec & boxes = *(this->has_children());
  const std::size_t num_boxes = boxes.size();

  for (std::size_t i = 0; i < num_boxes; ++i)
  {
    const TBoxPtr & box = boxes[i];
    if (box->type_.same_as(fourcc))
    {
      return box.get();
    }
  }

  for (std::size_t i = 0; i < num_boxes; ++i)
  {
    const TBoxPtr & box = boxes[i];
    const Box * found = box->find_child(fourcc);
    if (found)
    {
      return found;
    }
  }

  return NULL;
}


//----------------------------------------------------------------
// FullBox::load
//
void
FullBox::load(Mp4Context & mp4, IBitstream & bin)
{
  Box::load(mp4, bin);
  version_ = bin.read_bits(8);
  flags_ = bin.read_bits(24);
}

//----------------------------------------------------------------
// FullBox::to_json
//
void
FullBox::to_json(Json::Value & out) const
{
  Box::to_json(out);
  out["FullBox.version"] = uint32_t(version_);
  out["FullBox.flags"] = flags_;
}


//----------------------------------------------------------------
// ContainerList::load
//
void
ContainerList::load(Mp4Context & mp4, IBitstream & bin)
{
  const std::size_t box_pos = bin.position();
  TBase::load(mp4, bin);

  const std::size_t end_pos = box_pos + Box::size_ * 8;
  const uint32_t entry_count = bin.read_bits(32);
  for (uint32_t i = 0; i < entry_count; ++i)
  {
    TBoxPtr box = mp4.parse(bin, end_pos);
    YAE_ASSERT(box);
    if (box)
    {
      children_.push_back(box);
    }
  }
}

//----------------------------------------------------------------
// ContainerList::to_json
//
void
ContainerList::to_json(Json::Value & out) const
{
  TBase::to_json(out);

  const uint64_t entry_count = children_.size();
  out["entry_count"] = Json::UInt64(entry_count);

  Json::Value & children = out["children"];
  for (uint64_t i = 0; i < entry_count; ++i)
  {
    const Box & box = *(children_[i]);
    Json::Value child;
    box.to_json(child);
    children.append(child);
  }
}


//----------------------------------------------------------------
// FileTypeBox::load
//
void
FileTypeBox::load(Mp4Context & mp4, IBitstream & bin)
{
  const std::size_t box_pos = bin.position();
  Box::load(mp4, bin);

  const std::size_t box_end = box_pos + Box::size_ * 8;
  bin.read_bytes(major_.str_, 4);
  minor_ = bin.read_bits(32);

  while (bin.position() < box_end)
  {
    FourCC fourcc;
    bin.read_bytes(fourcc.str_, 4);
    compatible_.push_back(fourcc);
  }
}

//----------------------------------------------------------------
// FileTypeBox::to_json
//
void
FileTypeBox::to_json(Json::Value & out) const
{
  Box::to_json(out);
  out["major"] = major_.str_;
  out["minor"] = minor_;

  Json::Value & compatible = out["compatible"];
  for (std::size_t i = 0, n = compatible_.size(); i < n; ++i)
  {
    const FourCC & fourcc = compatible_[i];
    compatible.append(fourcc.str_);
  }
}


//----------------------------------------------------------------
// FreeSpaceBox::load
//
void
FreeSpaceBox::load(Mp4Context & mp4, IBitstream & bin)
{
  const std::size_t box_pos = bin.position();
  Box::load(mp4, bin);

  const std::size_t box_end = box_pos + Box::size_ * 8;
  bin.seek(box_end);
}


//----------------------------------------------------------------
// MediaDataBox::load
//
void
MediaDataBox::load(Mp4Context & mp4, IBitstream & bin)
{
  const std::size_t box_pos = bin.position();
  Box::load(mp4, bin);

  const std::size_t box_end = box_pos + Box::size_ * 8;
  if (mp4.load_mdat_data_)
  {
    std::size_t cur_pos = bin.position();
    std::size_t num_bytes = (box_end - cur_pos) / 8;
    data_ = bin.read_bytes(num_bytes);
  }
  else
  {
    bin.seek(box_end);
  }
}


//----------------------------------------------------------------
// ProgressiveDownloadInfoBox::load
//
void
ProgressiveDownloadInfoBox::load(Mp4Context & mp4, IBitstream & bin)
{
  const std::size_t box_pos = bin.position();
  FullBox::load(mp4, bin);

  const std::size_t box_end = box_pos + Box::size_ * 8;
  while (bin.position() < box_end)
  {
    uint32_t rate = bin.read_bits(32);
    uint32_t initial_delay = bin.read_bits(32);
    rate_.push_back(rate);
    initial_delay_.push_back(initial_delay);
  }
}


//----------------------------------------------------------------
// ProgressiveDownloadInfoBox::to_json
//
void
ProgressiveDownloadInfoBox::to_json(Json::Value & out) const
{
  FullBox::to_json(out);

  Json::Value & rate = out["rate"];
  Json::Value & initial_delay = out["initial_delay"];
  for (std::size_t i = 0, n = rate_.size(); i < n; ++i)
  {
    rate.append(rate_[i]);
    initial_delay.append(initial_delay_[i]);
  }
}


//----------------------------------------------------------------
// MovieHeaderBox::MovieHeaderBox
//
MovieHeaderBox::MovieHeaderBox():
  creation_time_(0),
  modification_time_(0),
  timescale_(0),
  duration_(0),
  rate_(0x00010000), // 1.0
  volume_(0x0100), // 1.0 full volume
  reserved_16_(0)
{
  reserved_[0] = 0;
  reserved_[1] = 0;
  matrix_[0] = 0x00010000;
  matrix_[1] = 0x00000000;
  matrix_[2] = 0x00000000;
  matrix_[3] = 0x00000000;
  matrix_[4] = 0x00010000;
  matrix_[5] = 0x00000000;
  matrix_[6] = 0x00000000;
  matrix_[7] = 0x00000000;
  matrix_[8] = 0x40000000;
  pre_defined_[0] = 0;
  pre_defined_[1] = 0;
  pre_defined_[2] = 0;
  pre_defined_[3] = 0;
  pre_defined_[4] = 0;
  pre_defined_[5] = 0;
  next_track_ID_ = 0; // 0 is not a valid track ID value
}

//----------------------------------------------------------------
// MovieHeaderBox::load
//
void
MovieHeaderBox::load(Mp4Context & mp4, IBitstream & bin)
{
  FullBox::load(mp4, bin);

  if (version_ == 1)
  {
    creation_time_ = bin.read_bits(64);
    modification_time_ = bin.read_bits(64);
    timescale_ = bin.read_bits(32);
    duration_ = bin.read_bits(64);
  }
  else
  {
    creation_time_ = bin.read_bits(32);
    modification_time_ = bin.read_bits(32);
    timescale_ = bin.read_bits(32);
    duration_ = bin.read_bits(32);
  }

  rate_ = bin.read<int32_t>(32);
  volume_ = bin.read<int16_t>(16);
  reserved_16_ = bin.read_bits(16);
  reserved_[0] = bin.read_bits(32);
  reserved_[1] = bin.read_bits(32);

  matrix_[0] = bin.read<int32_t>(32);
  matrix_[1] = bin.read<int32_t>(32);
  matrix_[2] = bin.read<int32_t>(32);
  matrix_[3] = bin.read<int32_t>(32);
  matrix_[4] = bin.read<int32_t>(32);
  matrix_[5] = bin.read<int32_t>(32);
  matrix_[6] = bin.read<int32_t>(32);
  matrix_[7] = bin.read<int32_t>(32);
  matrix_[8] = bin.read<int32_t>(32);

  pre_defined_[0] = bin.read_bits(32);
  pre_defined_[1] = bin.read_bits(32);
  pre_defined_[2] = bin.read_bits(32);
  pre_defined_[3] = bin.read_bits(32);
  pre_defined_[4] = bin.read_bits(32);
  pre_defined_[5] = bin.read_bits(32);

  next_track_ID_ = bin.read_bits(32);
}

//----------------------------------------------------------------
// MovieHeaderBox::to_json
//
void
MovieHeaderBox::to_json(Json::Value & out) const
{
  FullBox::to_json(out);

  out["creation_time"] = Json::UInt64(creation_time_);
  out["modification_time"] = Json::UInt64(modification_time_);
  out["timescale"] = timescale_;
  out["duration"] = Json::UInt64(duration_);

  out["rate"] = double(rate_) / double(0x00010000);
  out["volume"] = double(volume_) / double(0x0100);

  Json::Value & matrix = out["matrix"];
  for (int i = 0; i < 9; ++i)
  {
    double v = double(matrix_[i]) / double(0x00010000);
    matrix.append(v);
  }

  out["next_track_ID"] = next_track_ID_;
}


//----------------------------------------------------------------
// TrackHeaderBox::TrackHeaderBox
//
TrackHeaderBox::TrackHeaderBox():
  creation_time_(0),
  modification_time_(0),
  track_ID_(0),
  reserved1_(0),
  duration_(0),
  layer_(0),
  alternate_group_(0),
  volume_(0x0100), // fixed point 8.8, set to 0 for non-audio tracks
  reserved3_(0),
  width_(0), // fixed point 16.16
  height_(0) // fixed point 16.16
{
  reserved2_[0] = 0;
  reserved2_[1] = 0;
  matrix_[0] = 0x00010000;
  matrix_[1] = 0x00000000;
  matrix_[2] = 0x00000000;
  matrix_[3] = 0x00000000;
  matrix_[4] = 0x00010000;
  matrix_[5] = 0x00000000;
  matrix_[6] = 0x00000000;
  matrix_[7] = 0x00000000;
  matrix_[8] = 0x40000000;
}

//----------------------------------------------------------------
// TrackHeaderBox::load
//
void
TrackHeaderBox::load(Mp4Context & mp4, IBitstream & bin)
{
  FullBox::load(mp4, bin);

  if (version_ == 1)
  {
    creation_time_ = bin.read_bits(64);
    modification_time_ = bin.read_bits(64);
    track_ID_ = bin.read_bits(32);
    reserved1_ = bin.read_bits(32);
    duration_ = bin.read_bits(64);
  }
  else
  {
    creation_time_ = bin.read_bits(32);
    modification_time_ = bin.read_bits(32);
    track_ID_ = bin.read_bits(32);
    reserved1_ = bin.read_bits(32);
    duration_ = bin.read_bits(32);
  }

  reserved2_[0] = bin.read_bits(32);
  reserved2_[1] = bin.read_bits(32);

  layer_ = bin.read<int16_t>(16);
  alternate_group_ = bin.read<int16_t>(16);
  volume_ = bin.read<int16_t>(16);
  reserved3_ = bin.read_bits(16);

  matrix_[0] = bin.read<int32_t>(32);
  matrix_[1] = bin.read<int32_t>(32);
  matrix_[2] = bin.read<int32_t>(32);
  matrix_[3] = bin.read<int32_t>(32);
  matrix_[4] = bin.read<int32_t>(32);
  matrix_[5] = bin.read<int32_t>(32);
  matrix_[6] = bin.read<int32_t>(32);
  matrix_[7] = bin.read<int32_t>(32);
  matrix_[8] = bin.read<int32_t>(32);

  width_ = bin.read_bits(32);
  height_ = bin.read_bits(32);
}

//----------------------------------------------------------------
// TrackHeaderBox::to_json
//
void
TrackHeaderBox::to_json(Json::Value & out) const
{
  FullBox::to_json(out);

  out["creation_time"] = Json::UInt64(creation_time_);
  out["modification_time"] = Json::UInt64(modification_time_);
  out["track_ID"] = track_ID_;
  out["duration"] = Json::UInt64(duration_);

  out["layer"] = Json::Int(layer_);
  out["alternate_group"] = Json::Int(alternate_group_);
  out["volume"] = double(volume_) / double(0x0100);

  Json::Value & matrix = out["matrix"];
  for (int i = 0; i < 9; ++i)
  {
    double v = double(matrix_[i]) / double(0x00010000);
    matrix.append(v);
  }

  out["width"] = double(width_) / double(0x00010000);
  out["height"] = double(height_) / double(0x00010000);
}


//----------------------------------------------------------------
// create
//
template <typename TBox>
struct create
{
  static TBox *
  box(const char * fourcc)
  {
    TBox * box = new TBox();
    box->type_.set(fourcc);
    box->size_ = 0;
    return box;
  }
};

// explicitly instantiate box constructor functions:
template Box * create<Box>::box(const char * fourcc);
template Container * create<Container>::box(const char * fourcc);
template ContainerEx * create<ContainerEx>::box(const char * fourcc);
template ContainerList * create<ContainerList>::box(const char * fourcc);
template FileTypeBox * create<FileTypeBox>::box(const char * fourcc);
template FreeSpaceBox * create<FreeSpaceBox>::box(const char * fourcc);
template MediaDataBox * create<MediaDataBox>::box(const char * fourcc);

template ProgressiveDownloadInfoBox *
create<ProgressiveDownloadInfoBox>::box(const char * fourcc);

template MovieHeaderBox * create<MovieHeaderBox>::box(const char * fourcc);
template TrackHeaderBox * create<TrackHeaderBox>::box(const char * fourcc);


//----------------------------------------------------------------
// BoxFactory
//
struct BoxFactory : public std::map<FourCC, TBoxConstructor>
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
  { this->operator[](FourCC(fourcc)) = box_constructor; }

  template <typename TBox>
  inline void add(const char * fourcc, TBox *(*box_constructor)(const char *))
  { this->add(fourcc, (TBoxConstructor)box_constructor); }

  // populate the LUT in the constructor:
  BoxFactory()
  {
    TBoxConstructor create_container =
      (TBoxConstructor)(create<Container>::box);
    this->add("moov", create_container);
    this->add("mvex", create_container);
    this->add("trak", create_container);
    this->add("moof", create_container);
    this->add("traf", create_container);
    this->add("edts", create_container);
    this->add("mdia", create_container);
    this->add("minf", create_container);
    this->add("dinf", create_container);
    this->add("stbl", create_container);
    this->add("sinf", create_container);
    this->add("schi", create_container);
    this->add("udta", create_container);

    this->add("meta", create<ContainerEx>::box);

    this->add("stsd", create<ContainerList>::box);

    this->add("ftyp", create<FileTypeBox>::box);
    this->add("free", create<FreeSpaceBox>::box);
    this->add("skip", create<FreeSpaceBox>::box);
    this->add("mdat", create<MediaDataBox>::box);

    this->add("pdin", create<ProgressiveDownloadInfoBox>::box);
    this->add("mvhd", create<MovieHeaderBox>::box);
    this->add("tkhd", create<TrackHeaderBox>::box);
  }

  // helper:
  static const BoxFactory & singleton()
  {
    static const BoxFactory singleton_;
    return singleton_;
  }
};


//----------------------------------------------------------------
// Mp4Context::parse
//
TBoxPtr
Mp4Context::parse(IBitstream & bin, std::size_t end_pos)
{
  const std::size_t box_pos = bin.position();
  TBoxPtr box(new Box());
  box->load(*this, bin);

  const std::size_t box_end = box_pos + box->size_ * 8;
  YAE_ASSERT(!end_pos || box_end <= end_pos);

  // instantiate appropriate box type:
  TBoxConstructor box_constructor = NULL;
  if (parse_mdat_data_ && box->type_.same_as("mdat"))
  {
    box_constructor = (TBoxConstructor)(create<Container>::box);
  }
  else
  {
    box_constructor = BoxFactory::singleton().get(box->type_);
  }

  if (box_constructor)
  {
    box.reset(box_constructor(box->type_.str_));
    bin.seek(box_pos);
    box->load(*this, bin);
  }

  return box;
}
