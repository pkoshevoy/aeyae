// -*- Mode: c++; tab-width: 8; c-basic-offset: 2; indent-tabs-mode: nil -*-
// NOTE: the first line of this file sets up source code indentation rules
// for Emacs; it is also a hint to anyone modifying this file.

// Created   : Sun Mar 30 12:09:04 PM MDT 2025
// Copyright : Pavel Koshevoy
// License   : MIT -- http://www.opensource.org/licenses/mit-license.php

// aeyae:
#include "yae/mp4/yae_mp4.h"
#include "yae/utils/yae_json.h"


// namespace access:
using namespace yae;
using namespace yae::iso_14496_12;


//----------------------------------------------------------------
// create
//
template <typename TBox>
struct create
{
  static TBox *
  please(const char * fourcc)
  {
    TBox * box = new TBox();
    box->type_.set(fourcc);
    box->size_ = 0;
    return box;
  }
};


//----------------------------------------------------------------
// Box::load
//
void
Box::load(Mp4Context & mp4, IBitstream & bin)
{
  (void)mp4;
  uint32_t size32 = bin.read<uint32_t>();
  bin.read_bytes(type_.str_, 4);

  if (size32 == 1)
  {
    // load largesize:
    size_ = bin.read<uint64_t>();
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
// Box::find
//
const Box *
Box::find(const TBoxPtrVec & boxes, const char * fourcc)
{
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
  return Box::find(boxes, fourcc);
}


//----------------------------------------------------------------
// FullBox::load
//
void
FullBox::load(Mp4Context & mp4, IBitstream & bin)
{
  Box::load(mp4, bin);
  version_ = bin.read<uint8_t>();
  flags_ = bin.read<uint32_t>(24);
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
// create<Container>::please
//
template Container *
create<Container>::please(const char * fourcc);

//----------------------------------------------------------------
// create<ContainerEx>::please
//
template ContainerEx *
create<ContainerEx>::please(const char * fourcc);

//----------------------------------------------------------------
// create<ContainerList>::please
//
template ContainerList *
create<ContainerList>::please(const char * fourcc);

//----------------------------------------------------------------
// ContainerList::load
//
void
ContainerList::load(Mp4Context & mp4, IBitstream & bin)
{
  const std::size_t box_pos = bin.position();
  TBase::load(mp4, bin);

  const std::size_t end_pos = box_pos + Box::size_ * 8;
  const uint32_t entry_count = bin.read<uint32_t>();
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
// create<FileTypeBox>::please
//
template FileTypeBox *
create<FileTypeBox>::please(const char * fourcc);

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
  minor_ = bin.read<uint32_t>();

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
// create<FreeSpaceBox>::please
//
template FreeSpaceBox *
create<FreeSpaceBox>::please(const char * fourcc);

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
// create<MediaDataBox>::please
//
template MediaDataBox *
create<MediaDataBox>::please(const char * fourcc);

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
    data_ = bin.read_bytes_until(box_end);
  }
  else
  {
    bin.seek(box_end);
  }
}


//----------------------------------------------------------------
// create<ProgressiveDownloadInfoBox>::please
//
template ProgressiveDownloadInfoBox *
create<ProgressiveDownloadInfoBox>::please(const char * fourcc);

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
    uint32_t rate = bin.read<uint32_t>();
    uint32_t initial_delay = bin.read<uint32_t>();
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
// create<MovieHeaderBox>::please
//
template MovieHeaderBox *
create<MovieHeaderBox>::please(const char * fourcc);

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
    creation_time_ = bin.read<uint64_t>();
    modification_time_ = bin.read<uint64_t>();
    timescale_ = bin.read<uint32_t>();
    duration_ = bin.read<uint64_t>();
  }
  else
  {
    creation_time_ = bin.read<uint32_t>();
    modification_time_ = bin.read<uint32_t>();
    timescale_ = bin.read<uint32_t>();
    duration_ = bin.read<uint32_t>();
  }

  rate_ = bin.read<int32_t>();
  volume_ = bin.read<int16_t>();
  reserved_16_ = bin.read<uint16_t>();
  reserved_[0] = bin.read<uint32_t>();
  reserved_[1] = bin.read<uint32_t>();

  matrix_[0] = bin.read<int32_t>();
  matrix_[1] = bin.read<int32_t>();
  matrix_[2] = bin.read<int32_t>();
  matrix_[3] = bin.read<int32_t>();
  matrix_[4] = bin.read<int32_t>();
  matrix_[5] = bin.read<int32_t>();
  matrix_[6] = bin.read<int32_t>();
  matrix_[7] = bin.read<int32_t>();
  matrix_[8] = bin.read<int32_t>();

  pre_defined_[0] = bin.read<uint32_t>();
  pre_defined_[1] = bin.read<uint32_t>();
  pre_defined_[2] = bin.read<uint32_t>();
  pre_defined_[3] = bin.read<uint32_t>();
  pre_defined_[4] = bin.read<uint32_t>();
  pre_defined_[5] = bin.read<uint32_t>();

  next_track_ID_ = bin.read<uint32_t>();
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
// create<TrackHeaderBox>::please
//
template TrackHeaderBox *
create<TrackHeaderBox>::please(const char * fourcc);

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
    creation_time_ = bin.read<uint64_t>();
    modification_time_ = bin.read<uint64_t>();
    track_ID_ = bin.read<uint32_t>();
    reserved1_ = bin.read<uint32_t>();
    duration_ = bin.read<uint64_t>();
  }
  else
  {
    creation_time_ = bin.read<uint32_t>();
    modification_time_ = bin.read<uint32_t>();
    track_ID_ = bin.read<uint32_t>();
    reserved1_ = bin.read<uint32_t>();
    duration_ = bin.read<uint32_t>();
  }

  reserved2_[0] = bin.read<uint32_t>();
  reserved2_[1] = bin.read<uint32_t>();

  layer_ = bin.read<int16_t>();
  alternate_group_ = bin.read<int16_t>();
  volume_ = bin.read<int16_t>();
  reserved3_ = bin.read<uint16_t>();

  matrix_[0] = bin.read<int32_t>();
  matrix_[1] = bin.read<int32_t>();
  matrix_[2] = bin.read<int32_t>();
  matrix_[3] = bin.read<int32_t>();
  matrix_[4] = bin.read<int32_t>();
  matrix_[5] = bin.read<int32_t>();
  matrix_[6] = bin.read<int32_t>();
  matrix_[7] = bin.read<int32_t>();
  matrix_[8] = bin.read<int32_t>();

  width_ = bin.read<uint32_t>();
  height_ = bin.read<uint32_t>();
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
// create<TrackReferenceTypeBox>::please
//
template TrackReferenceTypeBox *
create<TrackReferenceTypeBox>::please(const char * fourcc);

//----------------------------------------------------------------
// TrackReferenceTypeBox::load
//
void
TrackReferenceTypeBox::load(Mp4Context & mp4, IBitstream & bin)
{
    const std::size_t box_pos = bin.position();
    Box::load(mp4, bin);

    const std::size_t box_end = box_pos + Box::size_ * 8;
    while (bin.position() < box_end)
    {
      uint32_t track_ID = bin.read<uint32_t>();
      track_IDs_.push_back(track_ID);
    }
}

//----------------------------------------------------------------
// TrackReferenceTypeBox::to_json
//
void
TrackReferenceTypeBox::to_json(Json::Value & out) const
{
  Box::to_json(out);

  Json::Value & track_IDs = out["track_IDs"];
  for (std::size_t i = 0, n = track_IDs_.size(); i < n; ++i)
  {
    uint32_t track_ID = track_IDs_[i];
    track_IDs.append(track_ID);
  }
}


//----------------------------------------------------------------
// create<TrackGroupTypeBox>::please
//
template TrackGroupTypeBox *
create<TrackGroupTypeBox>::please(const char * fourcc);

//----------------------------------------------------------------
// TrackGroupTypeBox::load
//
void
TrackGroupTypeBox::load(Mp4Context & mp4, IBitstream & bin)
{
  const std::size_t box_pos = bin.position();
  FullBox::load(mp4, bin);

  const std::size_t box_end = box_pos + Box::size_ * 8;
  track_group_id_ = bin.read<uint32_t>();
  data_ = bin.read_bytes_until(box_end);
}

//----------------------------------------------------------------
// TrackGroupTypeBox::to_json
//
void
TrackGroupTypeBox::to_json(Json::Value & out) const
{
  FullBox::to_json(out);
  out["track_group_id"] = track_group_id_;
}


//----------------------------------------------------------------
// create<MediaHeaderBox>::please
//
template MediaHeaderBox *
create<MediaHeaderBox>::please(const char * fourcc);

//----------------------------------------------------------------
// MediaHeaderBox::MediaHeaderBox
//
MediaHeaderBox::MediaHeaderBox():
  creation_time_(0),
  modification_time_(0),
  timescale_(0),
  duration_(0),
  pre_defined_(0)
{
  language_[0] = 0;
  language_[1] = 0;
  language_[2] = 0;
  language_[3] = 0;
}

//----------------------------------------------------------------
// MediaHeaderBox::load
//
void
MediaHeaderBox::load(Mp4Context & mp4, IBitstream & bin)
{
  FullBox::load(mp4, bin);

  if (version_ == 1)
  {
    creation_time_ = bin.read<uint64_t>();
    modification_time_ = bin.read<uint64_t>();
    timescale_ = bin.read<uint32_t>();
    duration_ = bin.read<uint64_t>();
  }
  else
  {
    creation_time_ = bin.read<uint32_t>();
    modification_time_ = bin.read<uint32_t>();
    timescale_ = bin.read<uint32_t>();
    duration_ = bin.read<uint32_t>();
  }

  bin.skip(1); // padding 0 bit
  language_[0] = 0x60 + bin.read<char>(5);
  language_[1] = 0x60 + bin.read<char>(5);
  language_[2] = 0x60 + bin.read<char>(5);
  pre_defined_ = bin.read<uint16_t>();
}

//----------------------------------------------------------------
// MediaHeaderBox::to_json
//
void
MediaHeaderBox::to_json(Json::Value & out) const
{
  FullBox::to_json(out);

  out["creation_time"] = Json::UInt64(creation_time_);
  out["modification_time"] = Json::UInt64(modification_time_);
  out["timescale"] = timescale_;
  out["duration"] = Json::UInt64(duration_);
  out["language"] = language_;
}


//----------------------------------------------------------------
// create<HandlerBox>::please
//
template HandlerBox *
create<HandlerBox>::please(const char * fourcc);

//----------------------------------------------------------------
// HandlerBox::HandlerBox
//
HandlerBox::HandlerBox():
  pre_defined_(0)
{
  reserved_[0] = 0;
  reserved_[1] = 0;
  reserved_[2] = 0;
}

//----------------------------------------------------------------
// HandlerBox::load
//
void
HandlerBox::load(Mp4Context & mp4, IBitstream & bin)
{
  const std::size_t box_pos = bin.position();
  FullBox::load(mp4, bin);

  const std::size_t box_end = box_pos + Box::size_ * 8;
  pre_defined_ = bin.read<uint32_t>();
  bin.read_bytes(handler_type_.str_, 4);
  reserved_[0] = bin.read<uint32_t>();
  reserved_[1] = bin.read<uint32_t>();
  reserved_[2] = bin.read<uint32_t>();

  name_.clear();
  bin.read_string_until_null(name_, box_end);
  bin.seek(box_end);
}

//----------------------------------------------------------------
// HandlerBox::to_json
//
void
HandlerBox::to_json(Json::Value & out) const
{
  FullBox::to_json(out);
  out["handler_type"] = handler_type_.str_;
  out["name"] = name_;
}


//----------------------------------------------------------------
// create<NullMediaHeaderBox>::please
//
template NullMediaHeaderBox *
create<NullMediaHeaderBox>::please(const char * fourcc);


//----------------------------------------------------------------
// create<ExtendedLanguageBox>::please
//
template ExtendedLanguageBox *
create<ExtendedLanguageBox>::please(const char * fourcc);

//----------------------------------------------------------------
// ExtendedLanguageBox::load
//
void
ExtendedLanguageBox::load(Mp4Context & mp4, IBitstream & bin)
{
  const std::size_t box_pos = bin.position();
  FullBox::load(mp4, bin);

  const std::size_t box_end = box_pos + Box::size_ * 8;
  extended_language_.clear();
  bin.read_string_until_null(extended_language_, box_end);
  bin.seek(box_end);
}

//----------------------------------------------------------------
// ExtendedLanguageBox::to_json
//
void
ExtendedLanguageBox::to_json(Json::Value & out) const
{
  FullBox::to_json(out);
  out["extended_language"] = extended_language_;
}


//----------------------------------------------------------------
// SampleEntryBox::SampleEntryBox
//
SampleEntryBox::SampleEntryBox():
  data_reference_index_(0)
{
  reserved_[0] = 0;
  reserved_[1] = 0;
}

//----------------------------------------------------------------
// SampleEntryBox::load
//
void
SampleEntryBox::load(Mp4Context & mp4, IBitstream & bin)
{
  Box::load(mp4, bin);
  reserved_[0] = bin.read<uint8_t>();
  reserved_[1] = bin.read<uint8_t>();
  data_reference_index_ = bin.read<uint16_t>();
}

//----------------------------------------------------------------
// SampleEntryBox::to_json
//
void
SampleEntryBox::to_json(Json::Value & out) const
{
  Box::to_json(out);
  out["data_reference_index"] = Json::UInt(data_reference_index_);
}


//----------------------------------------------------------------
// create<BitRateBox>::please
//
template BitRateBox *
create<BitRateBox>::please(const char * fourcc);

//----------------------------------------------------------------
// BitRateBox::BitRateBox
//
BitRateBox::BitRateBox():
  bufferSizeDB_(0),
  maxBitrate_(0),
  avgBitrate_(0)
{}

//----------------------------------------------------------------
// BitRateBox::load
//
void
BitRateBox::load(Mp4Context & mp4, IBitstream & bin)
{
  Box::load(mp4, bin);
  bufferSizeDB_ = bin.read<uint32_t>();
  maxBitrate_ = bin.read<uint32_t>();
  avgBitrate_ = bin.read<uint32_t>();
}

//----------------------------------------------------------------
// BitRateBox::to_json
//
void
BitRateBox::to_json(Json::Value & out) const
{
  Box::to_json(out);
  out["bufferSizeDB"] = bufferSizeDB_;
  out["maxBitrate"] = maxBitrate_;
  out["avgBitrate"] = avgBitrate_;
}


//----------------------------------------------------------------
// create<DegradationPriorityBox>::please
//
template DegradationPriorityBox *
create<DegradationPriorityBox>::please(const char * fourcc);

//----------------------------------------------------------------
// DegradationPriorityBox::load
//
void
DegradationPriorityBox::load(Mp4Context & mp4, IBitstream & bin)
{
  const std::size_t box_pos = bin.position();
  FullBox::load(mp4, bin);

  const std::size_t box_end = box_pos + Box::size_ * 8;
  priority_.clear();

  while (bin.position() + 16 <= box_end)
  {
    uint16_t priority = bin.read<uint16_t>();
    priority_.push_back(priority);
  }
}

//----------------------------------------------------------------
// DegradationPriorityBox::to_json
//
void
DegradationPriorityBox::to_json(Json::Value & out) const
{
  FullBox::to_json(out);
  yae::save(out["priority"], priority_);
}


//----------------------------------------------------------------
// create<TimeToSampleBox>::please
//
template TimeToSampleBox *
create<TimeToSampleBox>::please(const char * fourcc);

//----------------------------------------------------------------
// TimeToSampleBox::load
//
void
TimeToSampleBox::load(Mp4Context & mp4, IBitstream & bin)
{
  FullBox::load(mp4, bin);

  sample_count_.clear();
  sample_delta_.clear();

  uint32_t entry_count = bin.read<uint32_t>();
  for (uint32_t i = 0; i < entry_count; ++i)
  {
    uint32_t sample_count = bin.read<uint32_t>();
    uint32_t sample_delta = bin.read<uint32_t>();
    sample_count_.push_back(sample_count);
    sample_delta_.push_back(sample_delta);
  }
}

//----------------------------------------------------------------
// TimeToSampleBox::to_json
//
void
TimeToSampleBox::to_json(Json::Value & out) const
{
  FullBox::to_json(out);
  out["entry_count"] = Json::UInt(sample_count_.size());
  yae::save(out["sample_count"], sample_count_);
  yae::save(out["sample_delta"], sample_delta_);
}


//----------------------------------------------------------------
// create<CompositionOffsetBox>::please
//
template CompositionOffsetBox *
create<CompositionOffsetBox>::please(const char * fourcc);

//----------------------------------------------------------------
// CompositionOffsetBox::load
//
void
CompositionOffsetBox::load(Mp4Context & mp4, IBitstream & bin)
{
  FullBox::load(mp4, bin);

  sample_count_.clear();
  sample_offset_.clear();

  uint32_t entry_count = bin.read<uint32_t>();
  for (uint32_t i = 0; i < entry_count; ++i)
  {
    uint32_t sample_count = bin.read<uint32_t>();
    int32_t sample_offset = bin.read<int32_t>();
    sample_count_.push_back(sample_count);
    sample_offset_.push_back(sample_offset);
  }
}

//----------------------------------------------------------------
// CompositionOffsetBox::to_json
//
void
CompositionOffsetBox::to_json(Json::Value & out) const
{
  FullBox::to_json(out);
  out["entry_count"] = Json::UInt(sample_count_.size());
  yae::save(out["sample_count"], sample_count_);
  yae::save(out["sample_offset"], sample_offset_);
}


//----------------------------------------------------------------
// create<CompositionToDecodeBox>::please
//
template CompositionToDecodeBox *
create<CompositionToDecodeBox>::please(const char * fourcc);

//----------------------------------------------------------------
// CompositionToDecodeBox::CompositionToDecodeBox
//
CompositionToDecodeBox::CompositionToDecodeBox():
  composition_to_dts_shift_(0),
  least_decode_to_display_delta_(0),
  greatest_decode_to_display_delta_(0),
  composition_start_time_(0),
  composition_end_time_(0)
{}

//----------------------------------------------------------------
// CompositionToDecodeBox::load
//
void
CompositionToDecodeBox::load(Mp4Context & mp4, IBitstream & bin)
{
  FullBox::load(mp4, bin);

  if (FullBox::version_ == 0)
  {
    composition_to_dts_shift_ = bin.read<int32_t>();
    least_decode_to_display_delta_ = bin.read<int32_t>();
    greatest_decode_to_display_delta_ = bin.read<int32_t>();
    composition_start_time_ = bin.read<int32_t>();
    composition_end_time_ = bin.read<int32_t>();
  }
  else
  {
    composition_to_dts_shift_ = bin.read<int64_t>();
    least_decode_to_display_delta_ = bin.read<int64_t>();
    greatest_decode_to_display_delta_ = bin.read<int64_t>();
    composition_start_time_ = bin.read<int64_t>();
    composition_end_time_ = bin.read<int64_t>();
  }
}

//----------------------------------------------------------------
// CompositionToDecodeBox::to_json
//
void
CompositionToDecodeBox::to_json(Json::Value & out) const
{
  FullBox::to_json(out);

  out["composition_to_dts_shift"] =
    Json::Int64(composition_to_dts_shift_);

  out["least_decode_to_display_delta"] =
    Json::Int64(least_decode_to_display_delta_);

  out["greatest_decode_to_display_delta"] =
    Json::Int64(greatest_decode_to_display_delta_);

  out["composition_start_time"] =
    Json::Int64(composition_start_time_);

  out["composition_end_time"] =
    Json::Int64(composition_end_time_);
}


//----------------------------------------------------------------
// create<SyncSampleBox>::please
//
template SyncSampleBox *
create<SyncSampleBox>::please(const char * fourcc);

//----------------------------------------------------------------
// SyncSampleBox::load
//
void
SyncSampleBox::load(Mp4Context & mp4, IBitstream & bin)
{
  FullBox::load(mp4, bin);

  sample_number_.clear();

  uint32_t entry_count = bin.read<uint32_t>();
  for (uint32_t i = 0; i < entry_count; ++i)
  {
    uint32_t sample_number = bin.read<uint32_t>();
    sample_number_.push_back(sample_number);
  }
}

//----------------------------------------------------------------
// SyncSampleBox::to_json
//
void
SyncSampleBox::to_json(Json::Value & out) const
{
  FullBox::to_json(out);
  out["entry_count"] = Json::UInt(sample_number_.size());
  yae::save(out["sample_number"], sample_number_);
}


//----------------------------------------------------------------
// create<ShadowSyncSampleBox>::please
//
template ShadowSyncSampleBox *
create<ShadowSyncSampleBox>::please(const char * fourcc);

//----------------------------------------------------------------
// ShadowSyncSampleBox::load
//
void
ShadowSyncSampleBox::load(Mp4Context & mp4, IBitstream & bin)
{
  FullBox::load(mp4, bin);

  shadowed_sample_number_.clear();
  sync_sample_number_.clear();

  uint32_t entry_count = bin.read<uint32_t>();
  for (uint32_t i = 0; i < entry_count; ++i)
  {
    uint32_t shadowed_sample_number = bin.read<uint32_t>();
    uint32_t sync_sample_number = bin.read<uint32_t>();
    shadowed_sample_number_.push_back(shadowed_sample_number);
    sync_sample_number_.push_back(sync_sample_number);
  }
}

//----------------------------------------------------------------
// ShadowSyncSampleBox::to_json
//
void
ShadowSyncSampleBox::to_json(Json::Value & out) const
{
  FullBox::to_json(out);
  out["entry_count"] = Json::UInt(shadowed_sample_number_.size());
  yae::save(out["shadowed_sample_number"], shadowed_sample_number_);
  yae::save(out["sync_sample_number"], sync_sample_number_);
}


//----------------------------------------------------------------
// create<SampleDependencyTypeBox>::please
//
template SampleDependencyTypeBox *
create<SampleDependencyTypeBox>::please(const char * fourcc);

//----------------------------------------------------------------
// SampleDependencyTypeBox::load
//
void
SampleDependencyTypeBox::load(Mp4Context & mp4, IBitstream & bin)
{
  const std::size_t box_pos = bin.position();
  FullBox::load(mp4, bin);

  const std::size_t box_end = box_pos + Box::size_ * 8;
  samples_.clear();

  while (bin.position() + 8 <= box_end)
  {
    Sample sample;
    sample.is_leading_ = bin.read<uint8_t>(2);
    sample.depends_on_ = bin.read<uint8_t>(2);
    sample.is_depended_on_ = bin.read<uint8_t>(2);
    sample.has_redundancy_ = bin.read<uint8_t>(2);
    samples_.push_back(sample);
  }
}

//----------------------------------------------------------------
// SampleDependencyTypeBox::to_json
//
void
SampleDependencyTypeBox::to_json(Json::Value & out) const
{
  FullBox::to_json(out);

  const std::size_t num_samples = samples_.size();
  out["sample_count"] = Json::UInt(num_samples);

  Json::Value & is_leading = out["is_leading"];
  Json::Value & depends_on = out["depends_on"];
  Json::Value & is_depended_on = out["is_depended_on"];
  Json::Value & has_redundancy = out["has_redundancy"];

  is_leading = Json::arrayValue;
  depends_on = Json::arrayValue;
  is_depended_on = Json::arrayValue;
  has_redundancy = Json::arrayValue;

  for (std::size_t i = 0; i < num_samples; ++i)
  {
    const Sample & sample = samples_[i];
    is_leading.append(Json::UInt(sample.is_leading_));
    depends_on.append(Json::UInt(sample.depends_on_));
    is_depended_on.append(Json::UInt(sample.is_depended_on_));
    has_redundancy.append(Json::UInt(sample.has_redundancy_));
  }
}


//----------------------------------------------------------------
// create<EditListBox>::please
//
template EditListBox *
create<EditListBox>::please(const char * fourcc);

//----------------------------------------------------------------
// EditListBox::load
//
void
EditListBox::load(Mp4Context & mp4, IBitstream & bin)
{
  FullBox::load(mp4, bin);

  segment_duration_.clear();
  media_time_.clear();
  media_rate_integer_.clear();
  media_rate_fraction_.clear();

  uint32_t entry_count = bin.read<uint32_t>();
  for (uint32_t i = 0; i < entry_count; ++i)
  {
    uint64_t segment_duration =
      (FullBox::version_ == 1) ? bin.read<uint64_t>() : bin.read<uint32_t>();

    uint64_t media_time =
      (FullBox::version_ == 1) ? bin.read<int64_t>() : bin.read<int32_t>();

    int16_t media_rate_integer = bin.read<int16_t>();
    int16_t media_rate_fraction = bin.read<int16_t>();

    segment_duration_.push_back(segment_duration);
    media_time_.push_back(media_time);
    media_rate_integer_.push_back(media_rate_integer);
    media_rate_fraction_.push_back(media_rate_fraction);
  }
}

//----------------------------------------------------------------
// EditListBox::to_json
//
void
EditListBox::to_json(Json::Value & out) const
{
  FullBox::to_json(out);
  out["entry_count"] = Json::UInt(this->get_entry_count());
  yae::save(out["segment_duration"], segment_duration_);
  yae::save(out["media_time"], media_time_);
  yae::save(out["media_rate_integer"], media_rate_integer_);
  yae::save(out["media_rate_fraction"], media_rate_fraction_);
}


//----------------------------------------------------------------
// create<DataEntryUrlBox>::please
//
template DataEntryUrlBox *
create<DataEntryUrlBox>::please(const char * fourcc);

//----------------------------------------------------------------
// DataEntryUrlBox::load
//
void
DataEntryUrlBox::load(Mp4Context & mp4, IBitstream & bin)
{
  const std::size_t box_pos = bin.position();
  FullBox::load(mp4, bin);

  const std::size_t box_end = box_pos + Box::size_ * 8;
  location_.clear();
  bin.read_string_until_null(location_, box_end);
  bin.seek(box_end);
}

//----------------------------------------------------------------
// DataEntryUrlBox::to_json
//
void
DataEntryUrlBox::to_json(Json::Value & out) const
{
  FullBox::to_json(out);
  out["location"] = location_;
}


//----------------------------------------------------------------
// create<DataEntryUrnBox>::please
//
template DataEntryUrnBox *
create<DataEntryUrnBox>::please(const char * fourcc);

//----------------------------------------------------------------
// DataEntryUrnBox::load
//
void
DataEntryUrnBox::load(Mp4Context & mp4, IBitstream & bin)
{
  const std::size_t box_pos = bin.position();
  FullBox::load(mp4, bin);

  const std::size_t box_end = box_pos + Box::size_ * 8;
  name_.clear();
  location_.clear();
  bin.read_string_until_null(name_, box_end);
  bin.read_string_until_null(location_, box_end);
  bin.seek(box_end);
}

//----------------------------------------------------------------
// DataEntryUrnBox::to_json
//
void
DataEntryUrnBox::to_json(Json::Value & out) const
{
  FullBox::to_json(out);
  out["name"] = name_;
  out["location"] = location_;
}


//----------------------------------------------------------------
// create<SampleSizeBox>::please
//
template SampleSizeBox *
create<SampleSizeBox>::please(const char * fourcc);

//----------------------------------------------------------------
// SampleSizeBox::SampleSizeBox
//
SampleSizeBox::SampleSizeBox():
  sample_size_(0),
  sample_count_(0)
{}

//----------------------------------------------------------------
// SampleSizeBox::load
//
void
SampleSizeBox::load(Mp4Context & mp4, IBitstream & bin)
{
  FullBox::load(mp4, bin);

  sample_size_ = bin.read<uint32_t>();
  sample_count_ = bin.read<uint32_t>();
  entry_size_.clear();

  if (sample_size_ == 0)
  {
    for (uint32_t i = 0; i < sample_count_; ++i)
    {
      uint32_t entry_size = bin.read<uint32_t>();
      entry_size_.push_back(entry_size);
    }
  }
}

//----------------------------------------------------------------
// SampleSizeBox::to_json
//
void
SampleSizeBox::to_json(Json::Value & out) const
{
  FullBox::to_json(out);
  out["sample_size"] = sample_size_;
  out["sample_count"] = sample_count_;

  if (sample_size_ == 0)
  {
    yae::save(out["entry_size"], entry_size_);
  }
}


//----------------------------------------------------------------
// create<CompactSampleSizeBox>::please
//
template CompactSampleSizeBox *
create<CompactSampleSizeBox>::please(const char * fourcc);

//----------------------------------------------------------------
// CompactSampleSizeBox::CompactSampleSizeBox
//
CompactSampleSizeBox::CompactSampleSizeBox():
  reserved_(0),
  field_size_(0),
  sample_count_(0)
{}

//----------------------------------------------------------------
// CompactSampleSizeBox::load
//
void
CompactSampleSizeBox::load(Mp4Context & mp4, IBitstream & bin)
{
  FullBox::load(mp4, bin);

  reserved_ = bin.read<uint32_t>(24);
  field_size_ = bin.read<uint8_t>();
  sample_count_ = bin.read<uint32_t>();
  entry_size_.clear();

  for (uint32_t i = 0; i < sample_count_; ++i)
  {
    uint16_t entry_size = bin.read<uint16_t>(field_size_);
    entry_size_.push_back(entry_size);
  }
}

//----------------------------------------------------------------
// CompactSampleSizeBox::to_json
//
void
CompactSampleSizeBox::to_json(Json::Value & out) const
{
  FullBox::to_json(out);
  out["field_size"] = field_size_;
  out["sample_count"] = sample_count_;
  yae::save(out["entry_size"], entry_size_);
}


//----------------------------------------------------------------
// create<SampleToChunkBox>::please
//
template SampleToChunkBox *
create<SampleToChunkBox>::please(const char * fourcc);

//----------------------------------------------------------------
// SampleToChunkBox::load
//
void
SampleToChunkBox::load(Mp4Context & mp4, IBitstream & bin)
{
  FullBox::load(mp4, bin);

  first_chunk_.clear();
  samples_per_chunk_.clear();
  sample_description_index_.clear();

  uint32_t entry_count = bin.read<uint32_t>();
  for (uint32_t i = 0; i < entry_count; ++i)
  {
    uint32_t first_chunk = bin.read<uint32_t>();
    uint32_t samples_per_chunk = bin.read<uint32_t>();
    uint32_t sample_description_index = bin.read<uint32_t>();

    first_chunk_.push_back(first_chunk);
    samples_per_chunk_.push_back(samples_per_chunk);
    sample_description_index_.push_back(sample_description_index);
  }
}

//----------------------------------------------------------------
// SampleToChunkBox::to_json
//
void
SampleToChunkBox::to_json(Json::Value & out) const
{
  FullBox::to_json(out);
  out["entry_count"] = Json::UInt(this->get_entry_count());
  yae::save(out["first_chunk"], first_chunk_);
  yae::save(out["samples_per_chunk"], samples_per_chunk_);
  yae::save(out["sample_description_index"], sample_description_index_);
}


//----------------------------------------------------------------
// create<ChunkOffsetBox>::please
//
template ChunkOffsetBox *
create<ChunkOffsetBox>::please(const char * fourcc);

//----------------------------------------------------------------
// ChunkOffsetBox::load
//
void
ChunkOffsetBox::load(Mp4Context & mp4, IBitstream & bin)
{
  FullBox::load(mp4, bin);

  chunk_offset_.clear();
  uint32_t entry_count = bin.read<uint32_t>();
  for (uint32_t i = 0; i < entry_count; ++i)
  {
    uint32_t chunk_offset = bin.read<uint32_t>();
    chunk_offset_.push_back(chunk_offset);
  }
}

//----------------------------------------------------------------
// ChunkOffsetBox::to_json
//
void
ChunkOffsetBox::to_json(Json::Value & out) const
{
  FullBox::to_json(out);
  out["entry_count"] = Json::UInt(this->get_entry_count());
  yae::save(out["chunk_offset"], chunk_offset_);
}


//----------------------------------------------------------------
// create<ChunkLargeOffsetBox>::please
//
template ChunkLargeOffsetBox *
create<ChunkLargeOffsetBox>::please(const char * fourcc);

//----------------------------------------------------------------
// ChunkLargeOffsetBox::load
//
void
ChunkLargeOffsetBox::load(Mp4Context & mp4, IBitstream & bin)
{
  FullBox::load(mp4, bin);

  chunk_offset_.clear();
  uint32_t entry_count = bin.read<uint32_t>();
  for (uint32_t i = 0; i < entry_count; ++i)
  {
    uint32_t chunk_offset = bin.read<uint64_t>();
    chunk_offset_.push_back(chunk_offset);
  }
}

//----------------------------------------------------------------
// ChunkLargeOffsetBox::to_json
//
void
ChunkLargeOffsetBox::to_json(Json::Value & out) const
{
  FullBox::to_json(out);
  out["entry_count"] = Json::UInt(this->get_entry_count());
  yae::save(out["chunk_offset"], chunk_offset_);
}


//----------------------------------------------------------------
// create<PaddingBitsBox>::please
//
template PaddingBitsBox *
create<PaddingBitsBox>::please(const char * fourcc);

//----------------------------------------------------------------
// PaddingBitsBox::load
//
void
PaddingBitsBox::load(Mp4Context & mp4, IBitstream & bin)
{
  FullBox::load(mp4, bin);

  sample_count_ = bin.read<uint32_t>();
  samples_.clear();

  for (uint32_t i = 0, n = (sample_count_ + 1) >> 1; i < n; ++i)
  {
    Sample sample;
    sample.reserved1_ = bin.read<uint8_t>(1);
    sample.pad1_ = bin.read<uint8_t>(3);
    sample.reserved2_ = bin.read<uint8_t>(1);
    sample.pad2_ = bin.read<uint8_t>(3);
    samples_.push_back(sample);
  }
}

//----------------------------------------------------------------
// PaddingBitsBox::to_json
//
void
PaddingBitsBox::to_json(Json::Value & out) const
{
  FullBox::to_json(out);

  out["sample_count"] = Json::UInt(sample_count_);

  Json::Value & pad1 = out["pad1"];
  Json::Value & pad2 = out["pad2"];

  pad1 = Json::arrayValue;
  pad2 = Json::arrayValue;

  for (std::size_t i = 0, n = samples_.size(); i < n; ++i)
  {
    const Sample & sample = samples_[i];
    pad1.append(Json::UInt(sample.pad1_));
    pad2.append(Json::UInt(sample.pad2_));
  }
}


//----------------------------------------------------------------
// create<SubSampleInformationBox>::please
//
template SubSampleInformationBox *
create<SubSampleInformationBox>::please(const char * fourcc);

//----------------------------------------------------------------
// SubSampleInformationBox::load
//
void
SubSampleInformationBox::load(Mp4Context & mp4, IBitstream & bin)
{
  FullBox::load(mp4, bin);

  uint32_t entry_count = bin.read<uint32_t>();
  entries_.clear();

  for (uint32_t i = 0; i < entry_count; ++i)
  {
    Entry entry;
    entry.sample_delta_ = bin.read<uint32_t>();

    uint16_t subsample_count = bin.read<uint16_t>();
    for (uint16_t j = 0; j < subsample_count; ++j)
    {
      uint32_t subsample_size =
        (FullBox::version_ == 1) ?
        bin.read<uint32_t>() :
        bin.read<uint16_t>();

      uint8_t subsample_priority = bin.read<uint8_t>();
      uint8_t discardable = bin.read<uint8_t>();
      uint32_t codec_specific_parameters = bin.read<uint32_t>();

      entry.subsample_size_.push_back(subsample_size);
      entry.subsample_priority_.push_back(subsample_priority);
      entry.discardable_.push_back(discardable);
      entry.codec_specific_parameters_.push_back(codec_specific_parameters);
    }

    entries_.push_back(entry);
  }
}

//----------------------------------------------------------------
// save
//
static void
save(Json::Value & entry, const SubSampleInformationBox::Entry & x)
{
  entry["sample_delta"] = Json::UInt(x.sample_delta_);
  entry["subsample_count"] = Json::UInt(x.subsample_size_.size());
  yae::save(entry["subsample_size"], x.subsample_size_);
  yae::save(entry["subsample_priority"], x.subsample_priority_);
  yae::save(entry["discardable"], x.discardable_);
  yae::save(entry["codec_specific_parameters"], x.codec_specific_parameters_);
}

//----------------------------------------------------------------
// SubSampleInformationBox::to_json
//
void
SubSampleInformationBox::to_json(Json::Value & out) const
{
  FullBox::to_json(out);

  out["entry_count"] = Json::UInt(entries_.size());
  Json::Value & entries = out["entries"];
  entries = Json::arrayValue;

  for (uint32_t i = 0, entry_count = entries_.size(); i < entry_count; ++i)
  {
    const Entry & x = entries_[i];
    Json::Value entry;
    ::save(entry, x);
    entries.append(entry);
  }
}


//----------------------------------------------------------------
// create<SampleAuxiliaryInformationSizesBox>::please
//
template SampleAuxiliaryInformationSizesBox *
create<SampleAuxiliaryInformationSizesBox>::please(const char * fourcc);

//----------------------------------------------------------------
// SampleAuxiliaryInformationSizesBox::SampleAuxiliaryInformationSizesBox
//
SampleAuxiliaryInformationSizesBox::SampleAuxiliaryInformationSizesBox():
  aux_info_type_(0),
  aux_info_type_parameters_(0),
  default_sample_info_size_(0)
{}

//----------------------------------------------------------------
// SampleAuxiliaryInformationSizesBox::load
//
void
SampleAuxiliaryInformationSizesBox::load(Mp4Context & mp4, IBitstream & bin)
{
  FullBox::load(mp4, bin);

  if ((FullBox::flags_ & 1) == 1)
  {
    aux_info_type_ = bin.read<uint32_t>();
    aux_info_type_parameters_ = bin.read<uint32_t>();
  }

  default_sample_info_size_ = bin.read<uint8_t>();

  sample_info_sizes_.clear();
  uint32_t sample_count = bin.read<uint32_t>();
  if (default_sample_info_size_ == 0)
  {
    for (uint32_t i = 0; i < sample_count; ++i)
    {
      uint32_t sample_info_size = bin.read<uint32_t>();
      sample_info_sizes_.push_back(sample_info_size);
    }
  }
}

//----------------------------------------------------------------
// SampleAuxiliaryInformationSizesBox::to_json
//
void
SampleAuxiliaryInformationSizesBox::to_json(Json::Value & out) const
{
  FullBox::to_json(out);

  if ((FullBox::flags_ & 1) == 1)
  {
    out["aux_info_type"] = aux_info_type_;
    out["aux_info_type_parameters"] = aux_info_type_parameters_;
  }

  out["default_sample_info_size"] = default_sample_info_size_;
  out["sample_count"] = Json::UInt(sample_info_sizes_.size());

  if (default_sample_info_size_ == 0)
  {
    yae::save(out["sample_info_sizes"], sample_info_sizes_);
  }
}


//----------------------------------------------------------------
// create<SampleAuxiliaryInformationOffsetsBox>::please
//
template SampleAuxiliaryInformationOffsetsBox *
create<SampleAuxiliaryInformationOffsetsBox>::please(const char * fourcc);

//----------------------------------------------------------------
// SampleAuxiliaryInformationOffsetsBox::SampleAuxiliaryInformationOffsetsBox
//
SampleAuxiliaryInformationOffsetsBox::SampleAuxiliaryInformationOffsetsBox():
  aux_info_type_(0),
  aux_info_type_parameters_(0)
{}

//----------------------------------------------------------------
// SampleAuxiliaryInformationOffsetsBox::load
//
void
SampleAuxiliaryInformationOffsetsBox::load(Mp4Context & mp4, IBitstream & bin)
{
  FullBox::load(mp4, bin);

  if ((FullBox::flags_ & 1) == 1)
  {
    aux_info_type_ = bin.read<uint32_t>();
    aux_info_type_parameters_ = bin.read<uint32_t>();
  }

  offsets_.clear();
  uint32_t sample_count = bin.read<uint32_t>();

  if (FullBox::version_ == 0)
  {
    for (uint32_t i = 0; i < sample_count; ++i)
    {
      uint32_t offset = bin.read<uint32_t>();
      offsets_.push_back(offset);
    }
  }
  else
  {
    for (uint32_t i = 0; i < sample_count; ++i)
    {
      uint64_t offset = bin.read<uint64_t>();
      offsets_.push_back(offset);
    }
  }
}

//----------------------------------------------------------------
// SampleAuxiliaryInformationOffsetsBox::to_json
//
void
SampleAuxiliaryInformationOffsetsBox::to_json(Json::Value & out) const
{
  FullBox::to_json(out);

  if ((FullBox::flags_ & 1) == 1)
  {
    out["aux_info_type"] = aux_info_type_;
    out["aux_info_type_parameters"] = aux_info_type_parameters_;
  }

  out["entry_count"] = Json::UInt(offsets_.size());
  yae::save(out["offsets"], offsets_);
}


//----------------------------------------------------------------
// create<MovieExtendsHeaderBox>::please
//
template MovieExtendsHeaderBox *
create<MovieExtendsHeaderBox>::please(const char * fourcc);

//----------------------------------------------------------------
// MovieExtendsHeaderBox::load
//
void
MovieExtendsHeaderBox::load(Mp4Context & mp4, IBitstream & bin)
{
  FullBox::load(mp4, bin);

  if (FullBox::version_ == 1)
  {
    fragment_duration_ = bin.read<uint64_t>();
  }
  else
  {
    fragment_duration_ = bin.read<uint32_t>();
  }
}

//----------------------------------------------------------------
// MovieExtendsHeaderBox::to_json
//
void
MovieExtendsHeaderBox::to_json(Json::Value & out) const
{
  FullBox::to_json(out);
  out["fragment_duration"] = Json::UInt64(fragment_duration_);
}


//----------------------------------------------------------------
// DefaulSampleFlags::load
//
void
DefaulSampleFlags::load(Mp4Context & mp4, IBitstream & bin)
{
  (void)mp4;
  reserved_ = bin.read<uint8_t>(4);
  is_leading_ = bin.read<uint8_t>(2);
  depends_on_ = bin.read<uint8_t>(2);
  is_depended_on_ = bin.read<uint8_t>(2);
  has_redundancy_ = bin.read<uint8_t>(2);
  sample_padding_value_ = bin.read<uint8_t>(3);
  sample_is_non_sync_sample_ = bin.read<uint8_t>(1);
  sample_degradation_priority_ = bin.read<uint16_t>();
}

//----------------------------------------------------------------
// DefaulSampleFlags::to_json
//
void
DefaulSampleFlags::to_json(Json::Value & out) const
{
  out["is_leading"] = is_leading_;
  out["depends_on"] = depends_on_;
  out["is_depended_on"] = is_depended_on_;
  out["has_redundancy"] = has_redundancy_;
  out["sample_padding_value"] = sample_padding_value_;
  out["sample_is_non_sync_sample"] = sample_is_non_sync_sample_;
  out["sample_degradation_priority"] = sample_degradation_priority_;
}


//----------------------------------------------------------------
// create<MovieFragmentHeaderBox>::please
//
template MovieFragmentHeaderBox *
create<MovieFragmentHeaderBox>::please(const char * fourcc);

//----------------------------------------------------------------
// MovieFragmentHeaderBox::load
//
void
MovieFragmentHeaderBox::load(Mp4Context & mp4, IBitstream & bin)
{
  FullBox::load(mp4, bin);
  sequence_number_ = bin.read<uint32_t>();
}

//----------------------------------------------------------------
// MovieFragmentHeaderBox::to_json
//
void
MovieFragmentHeaderBox::to_json(Json::Value & out) const
{
  FullBox::to_json(out);
  out["sequence_number"] = sequence_number_;
}


//----------------------------------------------------------------
// create<TrackExtendsBox>::please
//
template TrackExtendsBox *
create<TrackExtendsBox>::please(const char * fourcc);

//----------------------------------------------------------------
// TrackExtendsBox::load
//
void
TrackExtendsBox::load(Mp4Context & mp4, IBitstream & bin)
{
  FullBox::load(mp4, bin);

  track_ID_ = bin.read<uint32_t>();
  default_sample_description_index_ = bin.read<uint32_t>();
  default_sample_duration_ = bin.read<uint32_t>();
  default_sample_size_ = bin.read<uint32_t>();
  default_sample_flags_.load(mp4, bin);
}

//----------------------------------------------------------------
// TrackExtendsBox::to_json
//
void
TrackExtendsBox::to_json(Json::Value & out) const
{
  FullBox::to_json(out);
  out["track_ID"] = track_ID_;
  out["default_sample_description_index"] = default_sample_description_index_;
  out["default_sample_duration"] = default_sample_duration_;
  out["default_sample_size"] = default_sample_size_;
  default_sample_flags_.to_json(out["default_sample_flags"]);
}


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
      (TBoxConstructor)(create<Container>::please);
    this->add("moov", create_container);
    this->add("mvex", create_container);
    this->add("trak", create_container);
    this->add("tref", create_container);
    this->add("trgr", create_container);
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

    this->add("meta", create<ContainerEx>::please);

    this->add("stsd", create<ContainerList>::please);
    this->add("dref", create<ContainerList>::please);

    this->add("ftyp", create<FileTypeBox>::please);
    this->add("free", create<FreeSpaceBox>::please);
    this->add("skip", create<FreeSpaceBox>::please);
    this->add("mdat", create<MediaDataBox>::please);

    this->add("pdin", create<ProgressiveDownloadInfoBox>::please);
    this->add("mvhd", create<MovieHeaderBox>::please);
    this->add("tkhd", create<TrackHeaderBox>::please);
    this->add("mdhd", create<MediaHeaderBox>::please);
    this->add("hdlr", create<HandlerBox>::please);
    this->add("nmhd", create<NullMediaHeaderBox>::please);
    this->add("elng", create<ExtendedLanguageBox>::please);
    this->add("btrt", create<BitRateBox>::please);
    this->add("stdp", create<DegradationPriorityBox>::please);
    this->add("stts", create<TimeToSampleBox>::please);
    this->add("ctts", create<CompositionOffsetBox>::please);
    this->add("cslg", create<CompositionToDecodeBox>::please);
    this->add("stss", create<SyncSampleBox>::please);
    this->add("stsh", create<ShadowSyncSampleBox>::please);
    this->add("sdtp", create<SampleDependencyTypeBox>::please);
    this->add("elst", create<EditListBox>::please);
    this->add("url ", create<DataEntryUrlBox>::please);
    this->add("urn ", create<DataEntryUrnBox>::please);
    this->add("stsz", create<SampleSizeBox>::please);
    this->add("stz2", create<CompactSampleSizeBox>::please);
    this->add("stsc", create<SampleToChunkBox>::please);
    this->add("stco", create<ChunkOffsetBox>::please);
    this->add("co64", create<ChunkLargeOffsetBox>::please);
    this->add("padb", create<PaddingBitsBox>::please);
    this->add("subs", create<SubSampleInformationBox>::please);
    this->add("saiz", create<SampleAuxiliaryInformationSizesBox>::please);
    this->add("saio", create<SampleAuxiliaryInformationOffsetsBox>::please);
    this->add("mehd", create<MovieExtendsHeaderBox>::please);
    this->add("trex", create<TrackExtendsBox>::please);
    this->add("mfhd", create<MovieFragmentHeaderBox>::please);

    this->add("hint", create<TrackReferenceTypeBox>::please);
    this->add("cdsc", create<TrackReferenceTypeBox>::please);
    this->add("font", create<TrackReferenceTypeBox>::please);
    this->add("hind", create<TrackReferenceTypeBox>::please);
    this->add("vdep", create<TrackReferenceTypeBox>::please);
    this->add("vplx", create<TrackReferenceTypeBox>::please);
    this->add("subt", create<TrackReferenceTypeBox>::please);

    this->add("msrc", create<TrackGroupTypeBox>::please);
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
    box_constructor = (TBoxConstructor)(create<Container>::please);
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

  // sanity check:
  YAE_ASSERT(bin.position() == box_end);
  bin.seek(box_end);

  return box;
}
