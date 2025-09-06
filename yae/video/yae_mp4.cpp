// -*- Mode: c++; tab-width: 8; c-basic-offset: 2; indent-tabs-mode: nil -*-
// NOTE: the first line of this file sets up source code indentation rules
// for Emacs; it is also a hint to anyone modifying this file.

// Created   : Sun Mar 30 12:09:04 PM MDT 2025
// Copyright : Pavel Koshevoy
// License   : MIT -- http://www.opensource.org/licenses/mit-license.php

// aeyae:
#include "yae/video/yae_mp4.h"
#include "yae/utils/yae_json.h"


// namespace access:
using namespace yae;
using namespace yae::mp4;


//----------------------------------------------------------------
// yae::read_mp4_box_size
//
bool
yae::read_mp4_box_size(FILE * file,
                       uint64_t & box_size,
                       FourCC & box_type)
{
  uint64_t box_start = yae::ftell64(file);
  uint8_t buffer[8];
  bool success = false;

  do {
    // load 32-bit box size:
    if (yae::read(file, buffer, 4) != 4)
    {
      break;
    }

    box_size = yae::load_be32(buffer);

    // load box type fourcc code:
    if (yae::read(file, box_type.str_, 4) != 4)
    {
      break;
    }

    if (box_size == 1)
    {
      // load largesize:
      if (yae::read(file, buffer, 8) != 8)
      {
        break;
      }

      box_size = yae::load_be64(buffer);
    }
    else if (box_size == 0)
    {
      // box extends to the end of file:
      if (yae::fseek64(file, 0, SEEK_END) != 0)
      {
        break;
      }

      uint64_t file_size = yae::ftell64(file);
      box_size = file_size - box_start;
    }

    success = true;
  } while (false);

  // restore file position:
  if (yae::fseek64(file, box_start, SEEK_SET) != 0)
  {
    success = false;
  }

  return success;
}


//----------------------------------------------------------------
// yae::iso_639_2t::PackedLang::set
//
void
yae::iso_639_2t::PackedLang::set(uint16_t code)
{
  char str[4];
  str[0] = 0x60 + ((code >> 10) & 0x1F);
  str[1] = 0x60 + ((code >> 5) & 0x1F);
  str[2] = 0x60 + (code & 0x1F);
  str[3] = 0;
  str_ = str;
}

//----------------------------------------------------------------
// yae::iso_639_2t::PackedLang::load
//
void
yae::iso_639_2t::PackedLang::load(IBitstream & bin)
{
  char str[4];
  bin.skip(1); // padding 0 bit
  str[0] = 0x60 + bin.read<uint8_t>(5);
  str[1] = 0x60 + bin.read<uint8_t>(5);
  str[2] = 0x60 + bin.read<uint8_t>(5);
  str[3] = 0;
  str_ = str;
}

//----------------------------------------------------------------
// yae::iso_639_2t::PackedLang::save
//
void
yae::iso_639_2t::PackedLang::save(IBitstream & bin) const
{
  bin.write_bits(1, 0); // padding 0 bit
  for (size_t i = 0, n = str_.size(); i < 3; ++i)
  {
    uint8_t c = (i < n) ? str_[i] : 0x60;
    YAE_ASSERT(c >= 0x60);
    bin.write_bits(5, c - 0x60);
  }
}


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
// Box::peek_box_type
//
void
Box::peek_box_type(Mp4Context & mp4, IBitstream & bin)
{
  const std::size_t box_pos = bin.position();
  Box::load(mp4, bin);
  bin.seek(box_pos);
}

//----------------------------------------------------------------
// Box::load
//
void
Box::load(Mp4Context & mp4, IBitstream & bin)
{
  (void)mp4;
  uint32_t size32 = bin.read<uint32_t>();
  type_.load(bin);

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
// create<ContainerList16>::please
//
template ContainerList16 *
create<ContainerList16>::please(const char * fourcc);

//----------------------------------------------------------------
// create<ContainerList32>::please
//
template ContainerList32 *
create<ContainerList32>::please(const char * fourcc);


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
  major_.load(bin);
  minor_ = bin.read<uint32_t>();

  while (bin.position() < box_end)
  {
    FourCC fourcc;
    fourcc.load(bin);
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
  track_IDs_.clear();
  yae::LoadVec<uint32_t>(track_IDs_, bin, box_end);
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
{}

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

  language_.load(bin);
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
  yae::save(out["language"], language_);
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
  reserved_manufacturer_(0),
  reserved_flags_(0),
  reserved_flags_mask_(0)
{}

//----------------------------------------------------------------
// HandlerBox::load
//
void
HandlerBox::load(Mp4Context & mp4, IBitstream & bin)
{
  const std::size_t box_pos = bin.position();
  FullBox::load(mp4, bin);

  const std::size_t box_end = box_pos + Box::size_ * 8;
  pre_defined_.load(bin);
  handler_type_.load(bin);
  reserved_manufacturer_ = bin.read<uint32_t>();
  reserved_flags_ = bin.read<uint32_t>();
  reserved_flags_mask_ = bin.read<uint32_t>();

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
  yae::save(out["pre_defined"], pre_defined_);
  yae::save(out["handler_type"], handler_type_);
  yae::save(out["reserved_manufacturer"], reserved_manufacturer_);
  yae::save(out["reserved_flags"], reserved_flags_);
  yae::save(out["reserved_flags_mask"], reserved_flags_mask_);
  yae::save(out["name"], name_);
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
  reserved_[2] = 0;
  reserved_[3] = 0;
  reserved_[4] = 0;
  reserved_[5] = 0;
}

//----------------------------------------------------------------
// SampleEntryBox::load
//
void
SampleEntryBox::load(Mp4Context & mp4, IBitstream & bin)
{
  Box::load(mp4, bin);
  bin.read_bytes(reserved_, 6);
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
// create<AudioSampleEntryBox>::please
//
template AudioSampleEntryBox *
create<AudioSampleEntryBox>::please(const char * fourcc);

//----------------------------------------------------------------
// AudioSampleEntryBox::AudioSampleEntryBox
//
AudioSampleEntryBox::AudioSampleEntryBox():
  entry_version_(0),
  channel_count_(2),
  sample_size_(16),
  pre_defined_(0),
  reserved2_(0),
  sample_rate_(0)
{
  reserved1_[0] = 0;
  reserved1_[1] = 0;
  reserved1_[2] = 0;
}

//----------------------------------------------------------------
// AudioSampleEntryBox::load
//
void
AudioSampleEntryBox::load(Mp4Context & mp4, IBitstream & bin)
{
  const std::size_t box_pos = bin.position();
  SampleEntryBox::load(mp4, bin);
  const std::size_t box_end = box_pos + Box::size_ * 8;

  entry_version_ = bin.read<uint16_t>();

  reserved1_[0] = bin.read<uint16_t>();
  reserved1_[1] = bin.read<uint16_t>();
  reserved1_[2] = bin.read<uint16_t>();

  channel_count_ = bin.read<uint16_t>();
  sample_size_ = bin.read<uint16_t>();
  pre_defined_ = bin.read<int16_t>();
  reserved2_ = bin.read<uint16_t>();
  sample_rate_ = bin.read<uint32_t>();

  TSelf::load_children_until(mp4, bin, box_end);
}

//----------------------------------------------------------------
// AudioSampleEntryBox::to_json
//
void
AudioSampleEntryBox::to_json(Json::Value & out) const
{
  SampleEntryBox::to_json(out);

  out["entry_version"] = entry_version_;
  out["channel_count"] = channel_count_;
  out["sample_size"] = sample_size_;
  out["sample_rate"] = double(sample_rate_) / double(0x00010000);

  Json::Value & children = out["children"];
  TSelf::children_to_json(children);
}


//----------------------------------------------------------------
// create<VisualSampleEntryBox>::please
//
template VisualSampleEntryBox *
create<VisualSampleEntryBox>::please(const char * fourcc);

//----------------------------------------------------------------
// VisualSampleEntryBox::VisualSampleEntryBox
//
VisualSampleEntryBox::VisualSampleEntryBox():
  pre_defined1_(0),
  reserved1_(0),
  width_(0),
  height_(0),
  horizresolution_(0x00480000),
  vertresolution_(0x00480000),
  reserved2_(0),
  frame_count_(1),
  depth_(24),
  pre_defined3_(-1)
{
  pre_defined2_[0] = 0;
  pre_defined2_[1] = 0;
  pre_defined2_[2] = 0;
}

//----------------------------------------------------------------
// VisualSampleEntryBox::load
//
void
VisualSampleEntryBox::load(Mp4Context & mp4, IBitstream & bin)
{
  const std::size_t box_pos = bin.position();
  SampleEntryBox::load(mp4, bin);
  const std::size_t box_end = box_pos + Box::size_ * 8;

  pre_defined1_ = bin.read<uint16_t>();
  reserved1_ = bin.read<uint16_t>();
  pre_defined2_[0] = bin.read<uint32_t>();
  pre_defined2_[1] = bin.read<uint32_t>();
  pre_defined2_[2] = bin.read<uint32_t>();
  width_ = bin.read<uint16_t>();
  height_ = bin.read<uint16_t>();
  horizresolution_ = bin.read<uint32_t>();
  vertresolution_ = bin.read<uint32_t>();
  reserved2_ = bin.read<uint32_t>();
  frame_count_ = bin.read<uint16_t>();
  bin.read_string(compressorname_, 32);

  depth_ = bin.read<uint16_t>();
  pre_defined3_ = bin.read<int16_t>();

  TSelf::load_children_until(mp4, bin, box_end);
}

//----------------------------------------------------------------
// VisualSampleEntryBox::to_json
//
void
VisualSampleEntryBox::to_json(Json::Value & out) const
{
  SampleEntryBox::to_json(out);

  out["width"] = width_;
  out["height"] = height_;
  out["horizresolution"] = double(horizresolution_) / double(0x00010000);
  out["vertresolution"] = double(vertresolution_) / double(0x00010000);
  out["frame_count"] = frame_count_;

  // compressorname is supposed to be a Pascal string,
  // with string size stored in the 1st byte...
  // however, sometimes that is not the case:
  int n = compressorname_[0];
  if (n < 32)
  {
    out["compressorname"] = compressorname_.substr(1, n).c_str();
  }
  else
  {
    out["compressorname"] = compressorname_.c_str();
  }

  out["depth"] = depth_;

  Json::Value & children = out["children"];
  TSelf::children_to_json(children);
}

//----------------------------------------------------------------
// create<MpegSampleEntryBox>::please
//
template MpegSampleEntryBox *
create<MpegSampleEntryBox>::please(const char * fourcc);

//----------------------------------------------------------------
// create<CleanApertureBox>::please
//
template CleanApertureBox *
create<CleanApertureBox>::please(const char * fourcc);

//----------------------------------------------------------------
// CleanApertureBox::load
//
void
CleanApertureBox::load(Mp4Context & mp4, IBitstream & bin)
{
  Box::load(mp4, bin);

  cleanApertureWidthN_ = bin.read<uint32_t>();
  cleanApertureWidthD_ = bin.read<uint32_t>();
  cleanApertureHeightN_ = bin.read<uint32_t>();
  cleanApertureHeightD_ = bin.read<uint32_t>();
  horizOffN_ = bin.read<uint32_t>();
  horizOffD_ = bin.read<uint32_t>();
  vertOffN_ = bin.read<uint32_t>();
  vertOffD_ = bin.read<uint32_t>();
}

//----------------------------------------------------------------
// CleanApertureBox::to_json
//
void
CleanApertureBox::to_json(Json::Value & out) const
{
  Box::to_json(out);

  out["cleanApertureWidthN"] = cleanApertureWidthN_;
  out["cleanApertureWidthD"] = cleanApertureWidthD_;
  out["cleanApertureHeightN"] = cleanApertureHeightN_;
  out["cleanApertureHeightD"] = cleanApertureHeightD_;
  out["horizOffN"] = horizOffN_;
  out["horizOffD"] = horizOffD_;
  out["vertOffN"] = vertOffN_;
  out["vertOffD"] = vertOffD_;
}


//----------------------------------------------------------------
// create<PixelAspectRatioBox>::please
//
template PixelAspectRatioBox *
create<PixelAspectRatioBox>::please(const char * fourcc);

//----------------------------------------------------------------
// PixelAspectRatioBox::load
//
void
PixelAspectRatioBox::load(Mp4Context & mp4, IBitstream & bin)
{
  Box::load(mp4, bin);

  hSpacing_ = bin.read<uint32_t>();
  vSpacing_ = bin.read<uint32_t>();
}

//----------------------------------------------------------------
// PixelAspectRatioBox::to_json
//
void
PixelAspectRatioBox::to_json(Json::Value & out) const
{
  Box::to_json(out);

  out["hSpacing"] = hSpacing_;
  out["vSpacing"] = vSpacing_;
}


//----------------------------------------------------------------
// create<ColourInformationBox>::please
//
template ColourInformationBox *
create<ColourInformationBox>::please(const char * fourcc);

//----------------------------------------------------------------
// ColourInformationBox::load
//
void
ColourInformationBox::load(Mp4Context & mp4, IBitstream & bin)
{
  const std::size_t box_pos = bin.position();
  Box::load(mp4, bin);
  const std::size_t box_end = box_pos + Box::size_ * 8;

  colour_type_.load(bin);

  bool is_nclc = colour_type_.same_as("nclc");
  bool is_nclx = colour_type_.same_as("nclx");

  if (is_nclc || is_nclx)
  {
    colour_primaries_ = bin.read<uint16_t>();
    transfer_characteristics_ = bin.read<uint16_t>();
    matrix_coefficients_ = bin.read<uint16_t>();

    if (is_nclx)
    {
      full_range_flag_ = bin.read<uint8_t>(1);
      reserved_ = bin.read<uint8_t>(7);
    }
  }
  else if (colour_type_.same_as("rICC") ||
           colour_type_.same_as("prof"))
  {
    ICC_profile_ = bin.read_bytes_until(box_end);
  }
}

//----------------------------------------------------------------
// ColourInformationBox::to_json
//
void
ColourInformationBox::to_json(Json::Value & out) const
{
  Box::to_json(out);

  out["colour_type"] = colour_type_.str_;

  bool is_nclc = colour_type_.same_as("nclc");
  bool is_nclx = colour_type_.same_as("nclx");

  if (is_nclc || is_nclx)
  {
    out["colour_primaries"] = colour_primaries_;
    out["transfer_characteristics"] = transfer_characteristics_;
    out["matrix_coefficients"] = matrix_coefficients_;

    if (is_nclx)
    {
      out["full_range_flag"] = Json::UInt(full_range_flag_);
    }
  }
  else if (colour_type_.same_as("rICC") ||
           colour_type_.same_as("prof"))
  {
    std::string v = yae::to_hex(ICC_profile_.get(), ICC_profile_.size());
    out["ICC_profile"] = v;
  }
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
  yae::LoadVec<uint16_t>(priority_, bin, box_end);
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

  entry_count_ = bin.read<uint32_t>();
  for (uint32_t i = 0; i < entry_count_; ++i)
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
  out["entry_count"] = entry_count_;
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

  entry_count_ = bin.read<uint32_t>();
  for (uint32_t i = 0; i < entry_count_; ++i)
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
  out["entry_count"] = entry_count_;
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
  const std::size_t box_pos = bin.position();
  FullBox::load(mp4, bin);
  const std::size_t box_end = box_pos + Box::size_ * 8;

  entry_count_ = bin.read<uint32_t>();
  sample_number_.clear();
  yae::LoadVec<uint32_t>(entry_count_, sample_number_, bin, box_end);
}

//----------------------------------------------------------------
// SyncSampleBox::to_json
//
void
SyncSampleBox::to_json(Json::Value & out) const
{
  FullBox::to_json(out);
  out["entry_count"] = entry_count_;
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

  entry_count_ = bin.read<uint32_t>();
  for (uint32_t i = 0; i < entry_count_; ++i)
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
  out["entry_count"] = entry_count_;
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
  media_rate_.clear();

  entry_count_ = bin.read<uint32_t>();
  for (uint32_t i = 0; i < entry_count_; ++i)
  {
    uint64_t segment_duration =
      (FullBox::version_ == 1) ? bin.read<uint64_t>() : bin.read<uint32_t>();

    int64_t media_time =
      (FullBox::version_ == 1) ? bin.read<int64_t>() : bin.read<int32_t>();

    int32_t media_rate = bin.read<uint32_t>();

    segment_duration_.push_back(segment_duration);
    media_time_.push_back(media_time);
    media_rate_.push_back(media_rate);
  }
}

//----------------------------------------------------------------
// save_fixedpoint
//
template <typename TData, uint8_t frac_bits>
static void
save_fixedpoint(Json::Value & out, const std::vector<TData> & v)
{
  typedef std::vector<TData> TContainer;
  const double scale = double(1ull << frac_bits);

  Json::Value array(Json::arrayValue);
  for (typename TContainer::const_iterator i = v.begin(); i != v.end(); ++i)
  {
    double t = double(*i) / scale;
    array.append(t);
  }

  out = array;
}

//----------------------------------------------------------------
// EditListBox::to_json
//
void
EditListBox::to_json(Json::Value & out) const
{
  FullBox::to_json(out);
  out["entry_count"] = entry_count_;
  yae::save(out["segment_duration"], segment_duration_);
  yae::save(out["media_time"], media_time_);
  save_fixedpoint<uint32_t, 16>(out["media_rate"], media_rate_);
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
  const std::size_t box_pos = bin.position();
  FullBox::load(mp4, bin);
  const std::size_t box_end = box_pos + Box::size_ * 8;

  sample_size_ = bin.read<uint32_t>();
  sample_count_ = bin.read<uint32_t>();
  entry_size_.clear();

  if (sample_size_ == 0)
  {
    yae::LoadVec<uint32_t>(sample_count_, entry_size_, bin, box_end);
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
  const std::size_t box_pos = bin.position();
  FullBox::load(mp4, bin);
  const std::size_t box_end = box_pos + Box::size_ * 8;

  reserved_ = bin.read<uint32_t>(24);
  field_size_ = bin.read<uint8_t>();
  sample_count_ = bin.read<uint32_t>();
  entry_size_.clear();
  yae::LoadVec<uint16_t>(sample_count_, entry_size_, bin, box_end);
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

  entry_count_ = bin.read<uint32_t>();
  for (uint32_t i = 0; i < entry_count_; ++i)
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
  out["entry_count"] = entry_count_;
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
  const std::size_t box_pos = bin.position();
  FullBox::load(mp4, bin);
  const std::size_t box_end = box_pos + Box::size_ * 8;

  entry_count_ = bin.read<uint32_t>();
  chunk_offset_.clear();
  yae::LoadVec<uint32_t>(entry_count_, chunk_offset_, bin, box_end);
}

//----------------------------------------------------------------
// ChunkOffsetBox::to_json
//
void
ChunkOffsetBox::to_json(Json::Value & out) const
{
  FullBox::to_json(out);
  out["entry_count"] = entry_count_;
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
  const std::size_t box_pos = bin.position();
  FullBox::load(mp4, bin);
  const std::size_t box_end = box_pos + Box::size_ * 8;

  entry_count_ = bin.read<uint32_t>();
  chunk_offset_.clear();
  yae::LoadVec<uint64_t>(entry_count_, chunk_offset_, bin, box_end);
}

//----------------------------------------------------------------
// ChunkLargeOffsetBox::to_json
//
void
ChunkLargeOffsetBox::to_json(Json::Value & out) const
{
  FullBox::to_json(out);
  out["entry_count"] = entry_count_;
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

  entry_count_ = bin.read<uint32_t>();
  entries_.clear();

  for (uint32_t i = 0; i < entry_count_; ++i)
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

  out["entry_count"] = entry_count_;
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
// SampleAuxiliaryInformationSizesBox::load
//
void
SampleAuxiliaryInformationSizesBox::load(Mp4Context & mp4, IBitstream & bin)
{
  const std::size_t box_pos = bin.position();
  FullBox::load(mp4, bin);
  const std::size_t box_end = box_pos + Box::size_ * 8;

  if ((FullBox::flags_ & 1) == 1)
  {
    aux_info_type_ = bin.read<uint32_t>();
    aux_info_type_parameters_ = bin.read<uint32_t>();
  }

  default_sample_info_size_ = bin.read<uint8_t>();

  sample_info_sizes_.clear();
  sample_count_ = bin.read<uint32_t>();

  if (default_sample_info_size_ == 0)
  {
    yae::LoadVec<uint32_t>(sample_count_, sample_info_sizes_, bin, box_end);
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
  out["sample_count"] = sample_count_;

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
// SampleAuxiliaryInformationOffsetsBox::load
//
void
SampleAuxiliaryInformationOffsetsBox::load(Mp4Context & mp4, IBitstream & bin)
{
  const std::size_t box_pos = bin.position();
  FullBox::load(mp4, bin);
  const std::size_t box_end = box_pos + Box::size_ * 8;

  if ((FullBox::flags_ & 1) == 1)
  {
    aux_info_type_ = bin.read<uint32_t>();
    aux_info_type_parameters_ = bin.read<uint32_t>();
  }

  offsets_.clear();
  entry_count_ = bin.read<uint32_t>();

  if (FullBox::version_ == 0)
  {
    yae::LoadVec<uint32_t>(entry_count_, offsets_, bin, box_end);
  }
  else
  {
    yae::LoadVec<uint64_t>(entry_count_, offsets_, bin, box_end);
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
// SampleFlags::SampleFlags
//
SampleFlags::SampleFlags()
{
  memset(this, 0, sizeof(*this));
}

//----------------------------------------------------------------
// SampleFlags::load
//
void
SampleFlags::load(IBitstream & bin)
{
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
// SampleFlags::to_json
//
void
SampleFlags::to_json(Json::Value & out) const
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
  default_sample_flags_.load(bin);
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
// create<TrackFragmentHeaderBox>::please
//
template TrackFragmentHeaderBox *
create<TrackFragmentHeaderBox>::please(const char * fourcc);

//----------------------------------------------------------------
// TrackFragmentHeaderBox::load
//
void
TrackFragmentHeaderBox::load(Mp4Context & mp4, IBitstream & bin)
{
  FullBox::load(mp4, bin);
  track_ID_ = bin.read<uint32_t>();

  if ((FullBox::flags_ & kBaseDataOffsetPresent) != 0)
  {
    base_data_offset_ = bin.read<uint32_t>();
  }

  if ((FullBox::flags_ & kSampleDescriptionIndexPresent) != 0)
  {
    sample_description_index_ = bin.read<uint32_t>();
  }

  if ((FullBox::flags_ & kDefaultSampleDurationPresent) != 0)
  {
    default_sample_duration_ = bin.read<uint32_t>();
  }

  if ((FullBox::flags_ & kDefaultSampleSizePresent) != 0)
  {
    default_sample_size_ = bin.read<uint32_t>();
  }

  if ((FullBox::flags_ & kDefaultSampleFlagsPresent) != 0)
  {
    default_sample_flags_.load(bin);
  }
}

//----------------------------------------------------------------
// TrackFragmentHeaderBox::to_json
//
void
TrackFragmentHeaderBox::to_json(Json::Value & out) const
{
  FullBox::to_json(out);
  out["track_ID"] = track_ID_;

  if ((FullBox::flags_ & kBaseDataOffsetPresent) != 0)
  {
    out["base_data_offset"] = base_data_offset_;
  }

  if ((FullBox::flags_ & kSampleDescriptionIndexPresent) != 0)
  {
    out["sample_description_index"] = sample_description_index_;
  }

  if ((FullBox::flags_ & kDefaultSampleDurationPresent) != 0)
  {
    out["default_sample_duration"] = default_sample_duration_;
  }

  if ((FullBox::flags_ & kDefaultSampleSizePresent) != 0)
  {
    out["default_sample_size"] = default_sample_size_;
  }

  if ((FullBox::flags_ & kDefaultSampleFlagsPresent) != 0)
  {
    default_sample_flags_.to_json(out["default_sample_flags"]);
  }
}


//----------------------------------------------------------------
// create<TrackRunBox>::please
//
template TrackRunBox *
create<TrackRunBox>::please(const char * fourcc);

//----------------------------------------------------------------
// TrackRunBox::load
//
void
TrackRunBox::load(Mp4Context & mp4, IBitstream & bin)
{
  FullBox::load(mp4, bin);
  sample_count_ = bin.read<uint32_t>();

  if ((FullBox::flags_ & kDataOffsetPresent) != 0)
  {
    data_offset_ = bin.read<int32_t>();
  }

  if ((FullBox::flags_ & kFirstSampleFlagsPresent) != 0)
  {
    // if this flag and field are used, sample_flags shall not be present:
    YAE_ASSERT((FullBox::flags_ & kSampleFlagsPresent) == 0);

    first_sample_flags_.load(bin);
  }

  sample_duration_.clear();
  sample_size_.clear();
  sample_flags_.clear();
  sample_composition_time_offset_.clear();

  for (uint32_t i = 0; i < sample_count_; ++i)
  {
    if ((FullBox::flags_ & kSampleDurationPresent) != 0)
    {
      uint32_t sample_duration = bin.read<uint32_t>();
      sample_duration_.push_back(sample_duration);
    }

    if ((FullBox::flags_ & kSampleSizePresent) != 0)
    {
      uint32_t sample_size = bin.read<uint32_t>();
      sample_size_.push_back(sample_size);
    }

    if ((FullBox::flags_ & kSampleFlagsPresent) != 0)
    {
      SampleFlags sample_flags;
      sample_flags.load(bin);
      sample_flags_.push_back(sample_flags);
    }

    if ((FullBox::flags_ & kSampleCompositionTimeOffsetsPresent) != 0)
    {
      if (FullBox::version_ == 1)
      {
        int32_t v = bin.read<int32_t>();
        sample_composition_time_offset_.push_back(v);
      }
      else
      {
        uint32_t v = bin.read<uint32_t>();
        sample_composition_time_offset_.push_back(v);
      }
    }
  }
}

//----------------------------------------------------------------
// TrackRunBox::to_json
//
void
TrackRunBox::to_json(Json::Value & out) const
{
  FullBox::to_json(out);
  out["sample_count"] = sample_count_;

  if ((FullBox::flags_ & kDataOffsetPresent) != 0)
  {
    out["data_offset"] = data_offset_;
  }

  if ((FullBox::flags_ & kFirstSampleFlagsPresent) != 0)
  {
    first_sample_flags_.to_json(out["first_sample_flags"]);
  }

  if ((FullBox::flags_ & kSampleDurationPresent) != 0)
  {
    yae::save(out["sample_duration"], sample_duration_);
  }

  if ((FullBox::flags_ & kSampleSizePresent) != 0)
  {
    yae::save(out["sample_size"], sample_size_);
  }

  if ((FullBox::flags_ & kSampleFlagsPresent) != 0)
  {
    Json::Value & sample_flags = out["sample_flags"];
    sample_flags = Json::arrayValue;

    for (std::size_t i = 0, n = sample_flags_.size(); i < n; ++i)
    {
      const SampleFlags & x = sample_flags_[i];
      Json::Value v;
      x.to_json(v);
      sample_flags.append(v);
    }
  }

  if ((FullBox::flags_ & kSampleCompositionTimeOffsetsPresent) != 0)
  {
    yae::save(out["sample_composition_time_offset"],
              sample_composition_time_offset_);
  }
}


//----------------------------------------------------------------
// create<TrackFragmentRandomAccessBox>::please
//
template TrackFragmentRandomAccessBox *
create<TrackFragmentRandomAccessBox>::please(const char * fourcc);

//----------------------------------------------------------------
// TrackFragmentRandomAccessBox::load
//
void
TrackFragmentRandomAccessBox::load(Mp4Context & mp4, IBitstream & bin)
{
  FullBox::load(mp4, bin);
  track_ID_ = bin.read<uint32_t>();

  reserved_ = bin.read<uint32_t>(26);
  length_size_of_traf_num_ = bin.read<uint8_t>(2);
  length_size_of_trun_num_ = bin.read<uint8_t>(2);
  length_size_of_sample_num_ = bin.read<uint8_t>(2);

  uint8_t traf_number_bits = (length_size_of_traf_num_ + 1) * 8;
  uint8_t trun_number_bits = (length_size_of_trun_num_ + 1) * 8;
  uint8_t sample_number_bits = (length_size_of_sample_num_ + 1) * 8;

  time_.clear();
  moof_offset_.clear();
  traf_number_.clear();
  trun_number_.clear();
  sample_number_.clear();

  entry_count_ = bin.read<uint32_t>();
  for (uint32_t i = 0; i < entry_count_; ++i)
  {
    uint64_t time =
      (FullBox::version_ == 1) ?
      bin.read<uint64_t>() :
      bin.read<uint32_t>();

    uint64_t moof_offset =
      (FullBox::version_ == 1) ?
      bin.read<uint64_t>() :
      bin.read<uint32_t>();

    uint32_t traf_number = bin.read<uint32_t>(traf_number_bits);
    uint32_t trun_number = bin.read<uint32_t>(trun_number_bits);
    uint32_t sample_number = bin.read<uint32_t>(sample_number_bits);

    time_.push_back(time);
    moof_offset_.push_back(moof_offset);
    traf_number_.push_back(traf_number);
    trun_number_.push_back(trun_number);
    sample_number_.push_back(sample_number);
  }
}

//----------------------------------------------------------------
// TrackFragmentRandomAccessBox::to_json
//
void
TrackFragmentRandomAccessBox::to_json(Json::Value & out) const
{
  FullBox::to_json(out);
  out["track_ID"] = track_ID_;
  out["length_size_of_traf_num"] = length_size_of_traf_num_;
  out["length_size_of_trun_num"] = length_size_of_trun_num_;
  out["length_size_of_sample_num"] = length_size_of_sample_num_;
  out["entry_count"] = entry_count_;

  yae::save(out["time"], time_);
  yae::save(out["moof_offset"], moof_offset_);
  yae::save(out["traf_number"], traf_number_);
  yae::save(out["trun_number"], trun_number_);
  yae::save(out["sample_number"], sample_number_);
}


//----------------------------------------------------------------
// create<MovieFragmentRandomAccessOffsetBox>::please
//
template MovieFragmentRandomAccessOffsetBox *
create<MovieFragmentRandomAccessOffsetBox>::please(const char * fourcc);

//----------------------------------------------------------------
// MovieFragmentRandomAccessOffsetBox::load
//
void
MovieFragmentRandomAccessOffsetBox::load(Mp4Context & mp4, IBitstream & bin)
{
  FullBox::load(mp4, bin);
  size_ = bin.read<uint32_t>();
}

//----------------------------------------------------------------
// MovieFragmentRandomAccessOffsetBox::to_json
//
void
MovieFragmentRandomAccessOffsetBox::to_json(Json::Value & out) const
{
  FullBox::to_json(out);
  out["size"] = size_;
}


//----------------------------------------------------------------
// create<TrackFragmentBaseMediaDecodeTimeBox>::please
//
template TrackFragmentBaseMediaDecodeTimeBox *
create<TrackFragmentBaseMediaDecodeTimeBox>::please(const char * fourcc);

//----------------------------------------------------------------
// TrackFragmentBaseMediaDecodeTimeBox::load
//
void
TrackFragmentBaseMediaDecodeTimeBox::load(Mp4Context & mp4, IBitstream & bin)
{
  FullBox::load(mp4, bin);
  baseMediaDecodeTime_ =
    (FullBox::version_ == 1) ?
    bin.read<uint64_t>() :
    bin.read<uint32_t>();
}

//----------------------------------------------------------------
// TrackFragmentBaseMediaDecodeTimeBox::to_json
//
void
TrackFragmentBaseMediaDecodeTimeBox::to_json(Json::Value & out) const
{
  FullBox::to_json(out);
  out["baseMediaDecodeTime"] = Json::UInt64(baseMediaDecodeTime_);
}


//----------------------------------------------------------------
// create<LevelAssignmentBox>::please
//
template LevelAssignmentBox *
create<LevelAssignmentBox>::please(const char * fourcc);

//----------------------------------------------------------------
// LevelAssignmentBox::Level::Level
//
LevelAssignmentBox::Level::Level()
{
  memset(this, 0, sizeof(*this));
}

//----------------------------------------------------------------
// LevelAssignmentBox::Level::load
//
void
LevelAssignmentBox::Level::load(IBitstream & bin)
{
  track_id_ = bin.read<uint32_t>();
  padding_flag_ = bin.read<uint8_t>(1);
  assignment_type_ = bin.read<uint8_t>(7);

  if (assignment_type_ == 0 ||
      assignment_type_ == 1)
  {
    grouping_type_.load(bin);
  }

  if (assignment_type_ == 1)
  {
    grouping_type_parameter_ = bin.read<uint32_t>();
  }

  if (assignment_type_ == 4)
  {
    sub_track_id_ = bin.read<uint32_t>();
  }
}

//----------------------------------------------------------------
// LevelAssignmentBox::Level::to_json
//
void
LevelAssignmentBox::Level::to_json(Json::Value & out) const
{
  out["track_id"] = track_id_;
  out["padding_flag"] = Json::UInt(padding_flag_);
  out["assignment_type"] = Json::UInt(assignment_type_);

  if (assignment_type_ == 0 ||
      assignment_type_ == 1)
  {
    out["grouping_type"] = grouping_type_.str_;
  }

  if (assignment_type_ == 1)
  {
    out["grouping_type_parameter"] = Json::UInt(grouping_type_parameter_);
  }

  if (assignment_type_ == 4)
  {
    out["sub_track_id"] = Json::UInt(sub_track_id_);
  }
}

//----------------------------------------------------------------
// LevelAssignmentBox::load
//
void
LevelAssignmentBox::load(Mp4Context & mp4, IBitstream & bin)
{
  FullBox::load(mp4, bin);
  levels_.clear();

  level_count_ = bin.read<uint8_t>();
  for (uint32_t i = 0; i < level_count_; ++i)
  {
    Level level;
    level.load(bin);
    levels_.push_back(level);
  }
}

//----------------------------------------------------------------
// LevelAssignmentBox::to_json
//
void
LevelAssignmentBox::to_json(Json::Value & out) const
{
  FullBox::to_json(out);

  out["level_count"] = level_count_;

  Json::Value & levels = out["levels"];
  levels = Json::arrayValue;

  for (std::size_t i = 0, n = levels_.size(); i < n; ++i)
  {
    const Level & level = levels_[i];
    Json::Value v;
    level.to_json(v);
    levels.append(v);
  }
}


//----------------------------------------------------------------
// create<TrackExtensionPropertiesBox>::please
//
template TrackExtensionPropertiesBox *
create<TrackExtensionPropertiesBox>::please(const char * fourcc);

//----------------------------------------------------------------
// TrackExtensionPropertiesBox::load
//
void
TrackExtensionPropertiesBox::load(Mp4Context & mp4, IBitstream & bin)
{
  const std::size_t box_pos = bin.position();
  FullBox::load(mp4, bin);

  const std::size_t end_pos = box_pos + Box::size_ * 8;
  track_id_ = bin.read<uint32_t>();

  ContainerEx::load_children_until(mp4, bin, end_pos);
}

//----------------------------------------------------------------
// TrackExtensionPropertiesBox::to_json
//
void
TrackExtensionPropertiesBox::to_json(Json::Value & out) const
{
  FullBox::to_json(out);
  out["track_id"] = track_id_;

  Json::Value & children = out["children"];
  ContainerEx::children_to_json(children);
}


//----------------------------------------------------------------
// create<AlternativeStartupSequencePropertiesBox>::please
//
template AlternativeStartupSequencePropertiesBox *
create<AlternativeStartupSequencePropertiesBox>::please(const char * fourcc);

//----------------------------------------------------------------
// AlternativeStartupSequencePropertiesBox::load
//
void
AlternativeStartupSequencePropertiesBox::load(Mp4Context & mp4,
                                              IBitstream & bin)
{
  FullBox::load(mp4, bin);

  grouping_type_parameters_.clear();
  min_initial_alt_startup_offsets_.clear();

  if (FullBox::version_ == 0)
  {
    min_initial_alt_startup_offset_ = bin.read<int32_t>();
  }
  else if (FullBox::version_ == 1)
  {
    num_entries_ = bin.read<uint32_t>();
    for (uint32_t i = 0; i < num_entries_; ++i)
    {
      uint32_t grouping_type_parameter = bin.read<uint32_t>();
      int32_t min_initial_alt_startup_offset = bin.read<int32_t>();

      grouping_type_parameters_.
        push_back(grouping_type_parameter);

      min_initial_alt_startup_offsets_.
        push_back(min_initial_alt_startup_offset);
    }
  }
}

//----------------------------------------------------------------
// AlternativeStartupSequencePropertiesBox::to_json
//
void
AlternativeStartupSequencePropertiesBox::to_json(Json::Value & out) const
{
  FullBox::to_json(out);
  if (FullBox::version_ == 0)
  {
    out["min_initial_alt_startup_offset"] = min_initial_alt_startup_offset_;
  }
  else if (FullBox::version_ == 1)
  {
    out["num_entries"] = num_entries_;

    yae::save(out["grouping_type_parameters"],
              grouping_type_parameters_);

    yae::save(out["min_initial_alt_startup_offsets"],
              min_initial_alt_startup_offsets_);
  }
}


//----------------------------------------------------------------
// create<SampleToGroupBox>::please
//
template SampleToGroupBox *
create<SampleToGroupBox>::please(const char * fourcc);

//----------------------------------------------------------------
// SampleToGroupBox::load
//
void
SampleToGroupBox::load(Mp4Context & mp4, IBitstream & bin)
{
  FullBox::load(mp4, bin);

  grouping_type_.load(bin);

  if (FullBox::version_ == 1)
  {
    grouping_type_parameter_ = bin.read<uint32_t>();
  }

  sample_count_.clear();
  group_description_index_.clear();

  entry_count_ = bin.read<uint32_t>();
  for (uint32_t i = 0; i < entry_count_; ++i)
  {
    uint32_t sample_count = bin.read<uint32_t>();
    uint32_t group_description_index = bin.read<uint32_t>();

    sample_count_.push_back(sample_count);
    group_description_index_.push_back(group_description_index);
  }
}

//----------------------------------------------------------------
// SampleToGroupBox::to_json
//
void
SampleToGroupBox::to_json(Json::Value & out) const
{
  FullBox::to_json(out);

  out["grouping_type"] = grouping_type_.str_;

  if (FullBox::version_ == 1)
  {
    out["grouping_type_parameter"] = grouping_type_parameter_;
  }

  out["entry_count"] = entry_count_;
  yae::save(out["sample_count"], sample_count_);
  yae::save(out["group_description_index"], group_description_index_);
}


//----------------------------------------------------------------
// create<SampleGroupDescriptionBox>::please
//
template SampleGroupDescriptionBox *
create<SampleGroupDescriptionBox>::please(const char * fourcc);

//----------------------------------------------------------------
// SampleGroupDescriptionBox::load
//
void
SampleGroupDescriptionBox::load(Mp4Context & mp4, IBitstream & bin)
{
  const std::size_t box_pos = bin.position();
  FullBox::load(mp4, bin);

  const std::size_t end_pos = box_pos + Box::size_ * 8;
  grouping_type_.load(bin);

  if (FullBox::version_ > 0)
  {
    default_length_ = bin.read<uint32_t>();
  }

  if (FullBox::version_ > 1)
  {
    default_sample_description_index_ = bin.read<uint32_t>();
  }

  entry_count_ = bin.read<uint32_t>();
  description_length_.clear();
  sample_group_entries_.clear();

  if (FullBox::version_ == 0)
  {
    v0_sample_group_entries_ = bin.read_bytes_until(end_pos);
  }
  else
  {
    for (uint32_t i = 0; i < entry_count_; ++i)
    {
      uint32_t description_length =
        default_length_ ? default_length_ : bin.read<uint32_t>();

      Data entry = bin.read_bytes(description_length);

      if (!default_length_)
      {
        description_length_.push_back(description_length);
      }

      sample_group_entries_.push_back(entry);
    }
  }
}

//----------------------------------------------------------------
// SampleGroupDescriptionBox::to_json
//
void
SampleGroupDescriptionBox::to_json(Json::Value & out) const
{
  FullBox::to_json(out);

  out["grouping_type"] = grouping_type_.str_;

  if (FullBox::version_ > 0)
  {
    out["default_length"] = default_length_;
  }

  if (FullBox::version_ > 1)
  {
    out["default_sample_description_index"] = default_sample_description_index_;
  }

  out["entry_count"] = entry_count_;

  if (FullBox::version_ == 0)
  {
    out["v0_sample_group_entries"] =
      yae::to_hex(v0_sample_group_entries_.get(),
                  v0_sample_group_entries_.size());
  }
  else
  {
    if (!description_length_.empty())
    {
      yae::save(out["description_length"], description_length_);
    }

    Json::Value & sample_group_entries = out["sample_group_entries"];
    sample_group_entries = Json::arrayValue;

    for (uint32_t i = 0, n = sample_group_entries_.size(); i < n; ++i)
    {
      const Data & entry = sample_group_entries_[i];
      std::string v = yae::to_hex(entry.get(), entry.size());
      sample_group_entries.append(v);
    }
  }
}


//----------------------------------------------------------------
// create<CopyrightBox>::please
//
template CopyrightBox *
create<CopyrightBox>::please(const char * fourcc);

//----------------------------------------------------------------
// CopyrightBox::load
//
void
CopyrightBox::load(Mp4Context & mp4, IBitstream & bin)
{
  const std::size_t box_pos = bin.position();
  FullBox::load(mp4, bin);

  const std::size_t box_end = box_pos + Box::size_ * 8;
  language_.load(bin);
  notice_.clear();
  load_as_utf8(notice_, bin, box_end);
}

//----------------------------------------------------------------
// CopyrightBox::to_json
//
void
CopyrightBox::to_json(Json::Value & out) const
{
  FullBox::to_json(out);
  yae::save(out["language"], language_);
  yae::save(out["notice"], notice_);
}


//----------------------------------------------------------------
// create<TrackSelectionBox>::please
//
template TrackSelectionBox *
create<TrackSelectionBox>::please(const char * fourcc);

//----------------------------------------------------------------
// TrackSelectionBox::load
//
void
TrackSelectionBox::load(Mp4Context & mp4, IBitstream & bin)
{
  const std::size_t box_pos = bin.position();
  FullBox::load(mp4, bin);

  const std::size_t box_end = box_pos + Box::size_ * 8;
  switch_group_ = bin.read<int32_t>();

  attribute_list_.clear();
  while (bin.position() + 32 <= box_end)
  {
    FourCC attr;
    attr.load(bin);
    attribute_list_.push_back(attr);
  }
}

//----------------------------------------------------------------
// TrackSelectionBox::to_json
//
void
TrackSelectionBox::to_json(Json::Value & out) const
{
  FullBox::to_json(out);
  out["switch_group"] = switch_group_;
  yae::save(out["attribute_list"], attribute_list_);
}


//----------------------------------------------------------------
// create<KindBox>::please
//
template KindBox *
create<KindBox>::please(const char * fourcc);

//----------------------------------------------------------------
// KindBox::load
//
void
KindBox::load(Mp4Context & mp4, IBitstream & bin)
{
  const std::size_t box_pos = bin.position();
  FullBox::load(mp4, bin);

  const std::size_t box_end = box_pos + Box::size_ * 8;
  schemeURI_.clear();
  value_.clear();
  bin.read_string_until_null(schemeURI_, box_end);
  bin.read_string_until_null(value_, box_end);
}

//----------------------------------------------------------------
// KindBox::to_json
//
void
KindBox::to_json(Json::Value & out) const
{
  FullBox::to_json(out);
  out["schemeURI"] = schemeURI_;
  if (!value_.empty())
  {
    out["value"] = value_;
  }
}


//----------------------------------------------------------------
// create<XMLBox>::please
//
template XMLBox *
create<XMLBox>::please(const char * fourcc);

//----------------------------------------------------------------
// XMLBox::load
//
void
XMLBox::load(Mp4Context & mp4, IBitstream & bin)
{
  const std::size_t box_pos = bin.position();
  FullBox::load(mp4, bin);

  const std::size_t box_end = box_pos + Box::size_ * 8;
  xml_.clear();
  load_as_utf8(xml_, bin, box_end);
}

//----------------------------------------------------------------
// XMLBox::to_json
//
void
XMLBox::to_json(Json::Value & out) const
{
  FullBox::to_json(out);
  out["xml"] = xml_;
}


//----------------------------------------------------------------
// create<BinaryXMLBox>::please
//
template BinaryXMLBox *
create<BinaryXMLBox>::please(const char * fourcc);

//----------------------------------------------------------------
// BinaryXMLBox::load
//
void
BinaryXMLBox::load(Mp4Context & mp4, IBitstream & bin)
{
  const std::size_t box_pos = bin.position();
  FullBox::load(mp4, bin);

  const std::size_t box_end = box_pos + Box::size_ * 8;
  data_ = bin.read_bytes_until(box_end);
}

//----------------------------------------------------------------
// BinaryXMLBox::to_json
//
void
BinaryXMLBox::to_json(Json::Value & out) const
{
  FullBox::to_json(out);
  out["data"] = yae::to_hex(data_.get(), data_.size());
}


//----------------------------------------------------------------
// create<ItemLocationBox>::please
//
template ItemLocationBox *
create<ItemLocationBox>::please(const char * fourcc);

//----------------------------------------------------------------
// ItemLocationBox::load
//
void
ItemLocationBox::load(Mp4Context & mp4, IBitstream & bin)
{
  FullBox::load(mp4, bin);

  offset_size_ = bin.read<uint16_t>(4);
  length_size_ = bin.read<uint16_t>(4);
  base_offset_size_ = bin.read<uint16_t>(4);
  index_size_ = bin.read<uint16_t>(4);
  bool load_extent_index =  (FullBox::version_ > 0 && index_size_ > 0);

  item_count_ =
    (FullBox::version_ < 2) ? bin.read<uint16_t>() : bin.read<uint32_t>();

  items_.clear();
  for (uint32_t i = 0; i < item_count_; ++i)
  {
    Item item;

    item.item_ID_ =
      (FullBox::version_ < 2) ? bin.read<uint16_t>() : bin.read<uint32_t>();

    item.reserved_ =
      (FullBox::version_ > 0) ? bin.read<uint16_t>(12) : 0;

    item.construction_method_ =
      (FullBox::version_ > 0) ? bin.read<uint16_t>(4) : 0;

    item.data_reference_index_ = bin.read<uint16_t>();
    item.base_offset_ = bin.read<uint64_t>(base_offset_size_ * 8);

    item.extent_count_ = bin.read<uint16_t>();
    for (uint16_t j = 0; j < item.extent_count_; ++j)
    {
      uint64_t extent_index =
        load_extent_index ? bin.read<uint64_t>(index_size_ * 8) : 0;

      uint64_t extent_offset = bin.read<uint64_t>(offset_size_ * 8);
      uint64_t extent_length = bin.read<uint64_t>(length_size_ * 8);

      if (load_extent_index)
      {
        item.extent_index_.push_back(extent_index);
      }

      item.extent_offset_.push_back(extent_offset);
      item.extent_length_.push_back(extent_length);
    }

    items_.push_back(item);
  }
}

//----------------------------------------------------------------
// ItemLocationBox::Item::to_json
//
void
ItemLocationBox::Item::to_json(Json::Value & out, uint32_t box_version) const
{
  out["item_ID"] = item_ID_;

  if (box_version > 0)
  {
    out["construction_method"] = construction_method_;
  }

  out["data_reference_index"] = data_reference_index_;
  out["base_offset"] = Json::UInt64(base_offset_);

  out["extent_count"] = extent_count_;
  if (!extent_index_.empty())
  {
    yae::save(out["extent_index"], extent_index_);
  }

  yae::save(out["extent_offset"], extent_offset_);
  yae::save(out["extent_length"], extent_length_);
}

//----------------------------------------------------------------
// ItemLocationBox::to_json
//
void
ItemLocationBox::to_json(Json::Value & out) const
{
  FullBox::to_json(out);
  out["offset_size"] = Json::UInt(offset_size_);
  out["length_size"] = Json::UInt(length_size_);
  out["base_offset_size"] = Json::UInt(base_offset_size_);
  out["index_size"] = Json::UInt(index_size_);
  out["item_count"] = item_count_;

  Json::Value & items = out["items"];
  for (std::size_t i = 0, n = items_.size(); i < n; ++i)
  {
    const Item & item = items_[i];
    Json::Value v;
    item.to_json(v, FullBox::version_);
    items.append(v);
  }
}


//----------------------------------------------------------------
// create<PrimaryItemBox>::please
//
template PrimaryItemBox *
create<PrimaryItemBox>::please(const char * fourcc);

//----------------------------------------------------------------
// PrimaryItemBox::load
//
void
PrimaryItemBox::load(Mp4Context & mp4, IBitstream & bin)
{
  FullBox::load(mp4, bin);

  item_ID_ =
    (FullBox::version_ == 0) ?
    bin.read<uint16_t>() :
    bin.read<uint32_t>();
}

//----------------------------------------------------------------
// PrimaryItemBox::to_json
//
void
PrimaryItemBox::to_json(Json::Value & out) const
{
  FullBox::to_json(out);
  out["item_ID"] = item_ID_;
}


//----------------------------------------------------------------
// create<ItemInfoEntryBox>::please
//
template ItemInfoEntryBox *
create<ItemInfoEntryBox>::please(const char * fourcc);

//----------------------------------------------------------------
// ItemInfoEntryBox::FDItemInfo::load
//
void
ItemInfoEntryBox::FDItemInfo::load(IBitstream & bin, std::size_t end_pos)
{
  bin.read_string_until_null(content_location_, end_pos);
  bin.read_string_until_null(content_MD5_, end_pos);
  content_length_ = bin.read<uint64_t>();
  transfer_length_ = bin.read<uint64_t>();

  group_ids_.clear();
  entry_count_ = bin.read<uint8_t>();
  yae::LoadVec<uint32_t>(entry_count_, group_ids_, bin, end_pos);
}

//----------------------------------------------------------------
// ItemInfoEntryBox::FDItemInfo::to_json
//
void
ItemInfoEntryBox::FDItemInfo::to_json(Json::Value & out) const
{
  out["content_location"] = content_location_;
  out["content_MD5"] = content_MD5_;
  out["content_length"] = Json::UInt64(content_length_);
  out["transfer_length"] = Json::UInt64(transfer_length_);
  out["entry_count"] = Json::UInt(entry_count_);
  yae::save(out["group_ids"], group_ids_);
}

//----------------------------------------------------------------
// ItemInfoEntryBox::Unknown::load
//
void
ItemInfoEntryBox::Unknown::load(IBitstream & bin, std::size_t end_pos)
{
  data_ = bin.read_bytes_until(end_pos);
}

//----------------------------------------------------------------
// ItemInfoEntryBox::Unknown::to_json
//
void
ItemInfoEntryBox::Unknown::to_json(Json::Value & out) const
{
  out["data"] = yae::to_hex(data_.get(), data_.size());
}

//----------------------------------------------------------------
// ItemInfoEntryBox::load
//
void
ItemInfoEntryBox::load(Mp4Context & mp4, IBitstream & bin)
{
  const std::size_t box_pos = bin.position();
  FullBox::load(mp4, bin);

  const std::size_t box_end = box_pos + Box::size_ * 8;
  item_ID_ =
    (FullBox::version_ < 3) ?
    bin.read<uint16_t>() :
    bin.read<uint32_t>();

  item_protection_index_ = bin.read<uint16_t>();

  if (FullBox::version_ > 1)
  {
    item_type_.load(bin);
  }

  bin.read_string_until_null(item_name_, box_end);
  bin.read_string_until_null(content_type_, box_end);

  // content_encoding is optional, check if it's there:
  if (bin.position() < box_end)
  {
    bin.read_string_until_null(content_encoding_, box_end);
  }

  extension_type_.clear();
  extension_.reset();

  if (FullBox::version_ != 1 || box_end < bin.position() + 32)
  {
    // no extension:
    return;
  }

  extension_type_.load(bin);
  if (extension_type_.same_as("fdel"))
  {
    extension_.reset(new FDItemInfo());
    extension_->load(bin, box_end);
  }
  else
  {
    extension_.reset(new Unknown());
    extension_->load(bin, box_end);
  }
}

//----------------------------------------------------------------
// ItemInfoEntryBox::to_json
//
void
ItemInfoEntryBox::to_json(Json::Value & out) const
{
  FullBox::to_json(out);
  out["item_ID"] = item_ID_;
  out["item_protection_index"] = item_protection_index_;

  if (FullBox::version_ > 1)
  {
    yae::save(out["item_type"], item_type_);
  }

  out["item_name"] = item_name_;
  out["content_type"] = content_type_;

  if (!content_encoding_.empty())
  {
    out["content_encoding"] = content_encoding_;
  }

  if (!extension_type_.empty())
  {
    yae::save(out["extension_type"], extension_type_);
  }

  if (extension_)
  {
    extension_->to_json(out["extension"]);
  }
}


//----------------------------------------------------------------
// create<ItemInfoBox>::please
//
template ItemInfoBox *
create<ItemInfoBox>::please(const char * fourcc);

//----------------------------------------------------------------
// ItemInfoBox::load
//
void
ItemInfoBox::load(Mp4Context & mp4, IBitstream & bin)
{
  const std::size_t box_pos = bin.position();
  FullBox::load(mp4, bin);
  const std::size_t box_end = box_pos + Box::size_ * 8;

  entry_count_ =
    (FullBox::version_ == 0) ?
    bin.read<uint16_t>() :
    bin.read<uint32_t>();

  ContainerEx::load_children(mp4, bin, box_end, entry_count_);
}


//----------------------------------------------------------------
// create<MetaboxRelationBox>::please
//
template MetaboxRelationBox *
create<MetaboxRelationBox>::please(const char * fourcc);

//----------------------------------------------------------------
// MetaboxRelationBox::load
//
void
MetaboxRelationBox::load(Mp4Context & mp4, IBitstream & bin)
{
  FullBox::load(mp4, bin);
  first_metabox_handler_type_.load(bin);
  second_metabox_handler_type_.load(bin);
  metabox_relation_ = bin.read<uint8_t>();
}

//----------------------------------------------------------------
// MetaboxRelationBox::to_json
//
void
MetaboxRelationBox::to_json(Json::Value & out) const
{
  FullBox::to_json(out);
  yae::save(out["first_metabox_handler_type"], first_metabox_handler_type_);
  yae::save(out["second_metabox_handler_type"], second_metabox_handler_type_);
  out["metabox_relation"] = Json::UInt(metabox_relation_);
}


//----------------------------------------------------------------
// create<ItemDataBox>::please
//
template ItemDataBox *
create<ItemDataBox>::please(const char * fourcc);

//----------------------------------------------------------------
// ItemDataBox::load
//
void
ItemDataBox::load(Mp4Context & mp4, IBitstream & bin)
{
  const std::size_t box_pos = bin.position();
  Box::load(mp4, bin);

  const std::size_t box_end = box_pos + Box::size_ * 8;
  data_ = bin.read_bytes_until(box_end);
}

//----------------------------------------------------------------
// ItemDataBox::to_json
//
void
ItemDataBox::to_json(Json::Value & out) const
{
  Box::to_json(out);
  out["data"] = yae::to_hex(data_.get(), data_.size());
}


//----------------------------------------------------------------
// create<ItemReferenceBox>::please
//
template ItemReferenceBox *
create<ItemReferenceBox>::please(const char * fourcc);

//----------------------------------------------------------------
// ItemReferenceBox::load
//
void
ItemReferenceBox::load(Mp4Context & mp4, IBitstream & bin)
{
  const std::size_t box_pos = bin.position();
  FullBox::load(mp4, bin);

  const std::size_t box_end = box_pos + Box::size_ * 8;
  children_.clear();

  while (bin.position() < box_end)
  {
    TBoxPtr box;
    if (FullBox::version_ == 0)
    {
      box.reset(new SingleItemTypeReferenceBox<uint16_t>());
    }
    else
    {
      box.reset(new SingleItemTypeReferenceBox<uint32_t>());
    }

    box->load(mp4, bin);
    children_.push_back(box);
  }
}


//----------------------------------------------------------------
// create<OriginalFormatBox>::please
//
template OriginalFormatBox *
create<OriginalFormatBox>::please(const char * fourcc);

//----------------------------------------------------------------
// OriginalFormatBox::load
//
void
OriginalFormatBox::load(Mp4Context & mp4, IBitstream & bin)
{
  Box::load(mp4, bin);
  data_format_.load(bin);
}

//----------------------------------------------------------------
// OriginalFormatBox::to_json
//
void
OriginalFormatBox::to_json(Json::Value & out) const
{
  Box::to_json(out);
  yae::save(out["data_format"], data_format_);
}


//----------------------------------------------------------------
// create<SchemeTypeBox>::please
//
template SchemeTypeBox *
create<SchemeTypeBox>::please(const char * fourcc);

//----------------------------------------------------------------
// SchemeTypeBox::load
//
void
SchemeTypeBox::load(Mp4Context & mp4, IBitstream & bin)
{
  const std::size_t box_pos = bin.position();
  FullBox::load(mp4, bin);
  const std::size_t box_end = box_pos + Box::size_ * 8;

  scheme_type_.load(bin);
  scheme_version_ = bin.read<uint32_t>();
  scheme_uri_.clear();

  if ((FullBox::flags_ & kSchemeUriPreset) != 0)
  {
    bin.read_string_until_null(scheme_uri_, box_end);
  }
}

//----------------------------------------------------------------
// SchemeTypeBox::to_json
//
void
SchemeTypeBox::to_json(Json::Value & out) const
{
  FullBox::to_json(out);

  yae::save(out["scheme_type"], scheme_type_);
  out["scheme_version"] = scheme_version_;

  if ((FullBox::flags_ & kSchemeUriPreset) != 0)
  {
    out["scheme_uri"] = scheme_uri_;
  }
}


//----------------------------------------------------------------
// create<FDItemInformationBox>::please
//
template FDItemInformationBox *
create<FDItemInformationBox>::please(const char * fourcc);

//----------------------------------------------------------------
// FDItemInformationBox::load
//
void
FDItemInformationBox::load(Mp4Context & mp4, IBitstream & bin)
{
  const std::size_t box_pos = bin.position();
  FullBox::load(mp4, bin);

  const std::size_t box_end = box_pos + Box::size_ * 8;
  children_.clear();

  entry_count_ = bin.read<uint16_t>();
  ContainerEx::load_children_until(mp4, bin, box_end);
}

//----------------------------------------------------------------
// FDItemInformationBox::to_json
//
void
FDItemInformationBox::to_json(Json::Value & out) const
{
  ContainerEx::to_json(out);
  out["entry_count"] = entry_count_;
}


//----------------------------------------------------------------
// create<FilePartitionBox>::please
//
template FilePartitionBox *
create<FilePartitionBox>::please(const char * fourcc);

//----------------------------------------------------------------
// FilePartitionBox::load
//
void
FilePartitionBox::load(Mp4Context & mp4, IBitstream & bin)
{
  const std::size_t box_pos = bin.position();
  FullBox::load(mp4, bin);
  const std::size_t box_end = box_pos + Box::size_ * 8;

  item_ID_ =
    (FullBox::version_ == 0) ? bin.read<uint16_t>() : bin.read<uint32_t>();

  packet_payload_size_ = bin.read<uint16_t>();
  reserved_ = bin.read<uint8_t>();
  FEC_encoding_ID_ = bin.read<uint8_t>();
  FEC_instance_ID_ = bin.read<uint16_t>();
  max_source_block_length_ = bin.read<uint16_t>();
  encoding_symbol_length_ = bin.read<uint16_t>();
  max_number_of_encoding_symbols_ = bin.read<uint16_t>();
  bin.read_string_until_null(scheme_specific_info_, box_end);

  block_count_.clear();
  block_size_.clear();

  entry_count_ =
    (FullBox::version_ == 0) ? bin.read<uint16_t>() : bin.read<uint32_t>();

  for (uint32_t i = 0; i < entry_count_; ++i)
  {
    uint16_t block_count = bin.read<uint16_t>();
    uint32_t block_size = bin.read<uint32_t>();
    block_count_.push_back(block_count);
    block_size_.push_back(block_size);
  }
}

//----------------------------------------------------------------
// FilePartitionBox::to_json
//
void
FilePartitionBox::to_json(Json::Value & out) const
{
  FullBox::to_json(out);
  out["item_ID"] = item_ID_;
  out["packet_payload_size"] = packet_payload_size_;
  out["FEC_encoding_ID"] = Json::UInt(FEC_encoding_ID_);
  out["FEC_instance_ID"] = FEC_instance_ID_;
  out["max_source_block_length"] = max_source_block_length_;
  out["encoding_symbol_length"] = encoding_symbol_length_;
  out["max_number_of_encoding_symbols"] = max_number_of_encoding_symbols_;
  out["scheme_specific_info"] = scheme_specific_info_;
  out["entry_count"] = entry_count_;
  yae::save(out["block_count"], block_count_);
  yae::save(out["block_size"], block_size_);
}


//----------------------------------------------------------------
// create<FECReservoirBox>::please
//
template FECReservoirBox *
create<FECReservoirBox>::please(const char * fourcc);

//----------------------------------------------------------------
// FECReservoirBox::load
//
void
FECReservoirBox::load(Mp4Context & mp4, IBitstream & bin)
{
  FullBox::load(mp4, bin);

  item_ID_.clear();

  symbol_count_.clear();

  entry_count_ =
    (FullBox::version_ == 0) ?
    bin.read<uint16_t>() :
    bin.read<uint32_t>();

  for (uint32_t i = 0; i < entry_count_; ++i)
  {
    uint32_t item_ID =
      (FullBox::version_ == 0) ?
      bin.read<uint16_t>() :
      bin.read<uint32_t>();

    uint32_t symbol_count = bin.read<uint32_t>();
    item_ID_.push_back(item_ID);
    symbol_count_.push_back(symbol_count);
  }
}

//----------------------------------------------------------------
// FECReservoirBox::to_json
//
void
FECReservoirBox::to_json(Json::Value & out) const
{
  FullBox::to_json(out);
  out["entry_count"] = entry_count_;
  yae::save(out["item_ID"], item_ID_);
  yae::save(out["symbol_count"], symbol_count_);
}


//----------------------------------------------------------------
// create<FDSessionGroupBox>::please
//
template FDSessionGroupBox *
create<FDSessionGroupBox>::please(const char * fourcc);

//----------------------------------------------------------------
// FDSessionGroupBox::SessionGroup::load
//
void
FDSessionGroupBox::SessionGroup::load(IBitstream & bin, std::size_t end_pos)
{
  entry_count_ = bin.read<uint8_t>();
  yae::LoadVec<uint32_t>(entry_count_, group_ID_, bin, end_pos);

  num_channels_in_session_group_ = bin.read<uint16_t>();
  yae::LoadVec<uint32_t>(num_channels_in_session_group_,
                         hint_track_id_,
                         bin,
                         end_pos);
}

//----------------------------------------------------------------
// FDSessionGroupBox::SessionGroup::to_json
//
void
FDSessionGroupBox::SessionGroup::to_json(Json::Value & out) const
{
  out["entry_count"] = entry_count_;
  yae::save(out["group_ID"], group_ID_);

  out["num_channels_in_session_group"] = num_channels_in_session_group_;
  yae::save(out["hint_track_id"], hint_track_id_);
}

//----------------------------------------------------------------
// FDSessionGroupBox::load
//
void
FDSessionGroupBox::load(Mp4Context & mp4, IBitstream & bin)
{
  const std::size_t box_pos = bin.position();
  Box::load(mp4, bin);
  const std::size_t box_end = box_pos + Box::size_ * 8;

  session_groups_.clear();
  num_session_groups_ = bin.read<uint16_t>();
  for (uint16_t i = 0; i < num_session_groups_; ++i)
  {
    SessionGroup session_group;
    session_group.load(bin, box_end);
    session_groups_.push_back(session_group);
  }
}

//----------------------------------------------------------------
// FDSessionGroupBox::to_json
//
void
FDSessionGroupBox::to_json(Json::Value & out) const
{
  Box::to_json(out);
  out["num_session_groups"] = num_session_groups_;

  Json::Value & session_groups = out["session_groups"];
  session_groups = Json::arrayValue;

  for (std::size_t i = 0, n = session_groups_.size(); i < n; ++i)
  {
    const SessionGroup & session_group = session_groups_[i];
    Json::Value v;
    session_group.to_json(v);
    session_groups.append(v);
  }
}


//----------------------------------------------------------------
// create<GroupIdToNameBox>::please
//
template GroupIdToNameBox *
create<GroupIdToNameBox>::please(const char * fourcc);

//----------------------------------------------------------------
// GroupIdToNameBox::load
//
void
GroupIdToNameBox::load(Mp4Context & mp4, IBitstream & bin)
{
  const std::size_t box_pos = bin.position();
  FullBox::load(mp4, bin);
  const std::size_t box_end = box_pos + Box::size_ * 8;

  group_ID_.clear();
  group_name_.clear();

  entry_count_ = bin.read<uint16_t>();
  for (uint16_t i = 0; i < entry_count_; ++i)
  {
    uint32_t group_ID = bin.read<uint32_t>();
    std::string group_name;
    bin.read_string_until_null(group_name, box_end);
    group_ID_.push_back(group_ID);
    group_name_.push_back(group_name);
  }
}

//----------------------------------------------------------------
// GroupIdToNameBox::to_json
//
void
GroupIdToNameBox::to_json(Json::Value & out) const
{
  FullBox::to_json(out);
  out["entry_count"] = entry_count_;
  yae::save(out["group_ID"], group_ID_);
  yae::save(out["group_name"], group_name_);
}


//----------------------------------------------------------------
// create<SubTrackInformationBox>::please
//
template SubTrackInformationBox *
create<SubTrackInformationBox>::please(const char * fourcc);

//----------------------------------------------------------------
// SubTrackInformationBox::load
//
void
SubTrackInformationBox::load(Mp4Context & mp4, IBitstream & bin)
{
  const std::size_t box_pos = bin.position();
  FullBox::load(mp4, bin);
  const std::size_t box_end = box_pos + Box::size_ * 8;

  attribute_list_.clear();

  switch_group_ = bin.read<uint16_t>();
  alternate_group_ = bin.read<uint16_t>();
  sub_track_ID_ = bin.read<uint32_t>();

  while (bin.position() + 32 <= box_end)
  {
    FourCC attr;
    attr.load(bin);
    attribute_list_.push_back(attr);
  }
}

//----------------------------------------------------------------
// SubTrackInformationBox::to_json
//
void
SubTrackInformationBox::to_json(Json::Value & out) const
{
  FullBox::to_json(out);
  out["switch_group"] = switch_group_;
  out["alternate_group"] = alternate_group_;
  out["sub_track_ID"] = sub_track_ID_;
  yae::save(out["attribute_list"], attribute_list_);
}


//----------------------------------------------------------------
// create<SubTrackSampleGroupBox>::please
//
template SubTrackSampleGroupBox *
create<SubTrackSampleGroupBox>::please(const char * fourcc);

//----------------------------------------------------------------
// SubTrackSampleGroupBox::load
//
void
SubTrackSampleGroupBox::load(Mp4Context & mp4, IBitstream & bin)
{
  const std::size_t box_pos = bin.position();
  FullBox::load(mp4, bin);
  const std::size_t box_end = box_pos + Box::size_ * 8;

  grouping_type_.load(bin);
  group_description_index_.clear();

  entry_count_ = bin.read<uint16_t>();
  yae::LoadVec<uint32_t>(entry_count_, group_description_index_, bin, box_end);
}

//----------------------------------------------------------------
// SubTrackSampleGroupBox::to_json
//
void
SubTrackSampleGroupBox::to_json(Json::Value & out) const
{
  FullBox::to_json(out);
  yae::save(out["grouping_type"], grouping_type_);
  out["entry_count"] = entry_count_;
  yae::save(out["group_description_index"], group_description_index_);
}


//----------------------------------------------------------------
// create<StereoVideoBox>::please
//
template StereoVideoBox *
create<StereoVideoBox>::please(const char * fourcc);

//----------------------------------------------------------------
// StereoVideoBox::load
//
void
StereoVideoBox::load(Mp4Context & mp4, IBitstream & bin)
{
  const std::size_t box_pos = bin.position();
  FullBox::load(mp4, bin);
  const std::size_t box_end = box_pos + Box::size_ * 8;

  reserved_ = bin.read<uint32_t>(30);
  single_view_allowed_ = bin.read<uint32_t>(2);
  stereo_scheme_ = bin.read<uint32_t>();
  length_ = bin.read<uint32_t>();
  stereo_indication_type_.clear();
  yae::LoadVec<uint8_t>(length_, stereo_indication_type_, bin, box_end);

  ContainerEx::load_children_until(mp4, bin, box_end);
}

//----------------------------------------------------------------
// StereoVideoBox::to_json
//
void
StereoVideoBox::to_json(Json::Value & out) const
{
  FullBox::to_json(out);
  out["single_view_allowed"] = single_view_allowed_;
  out["stereo_scheme"] = stereo_scheme_;
  out["length"] = length_;
  yae::save(out["stereo_indication_type"], stereo_indication_type_);

  if (!ContainerEx::children_.empty())
  {
    Json::Value & children = out["children"];
    ContainerEx::children_to_json(children);
  }
}


//----------------------------------------------------------------
// create<SegmentIndexBox>::please
//
template SegmentIndexBox *
create<SegmentIndexBox>::please(const char * fourcc);

//----------------------------------------------------------------
// SegmentIndexBox::Reference::Reference
//
SegmentIndexBox::Reference::Reference()
{
  memset(this, 0, sizeof(*this));
}

//----------------------------------------------------------------
// SegmentIndexBox::Reference::load
//
bool
SegmentIndexBox::Reference::load(IBitstream & bin, std::size_t end_pos)
{
  if (end_pos < bin.position() + 96)
  {
    return false;
  }

  reference_type_ = bin.read<uint32_t>(1);
  referenced_size_ = bin.read<uint32_t>(31);
  subsegment_duration_ = bin.read<uint32_t>();

  starts_with_SAP_ = bin.read<uint32_t>(1);
  SAP_type_ = bin.read<uint32_t>(3);
  SAP_delta_time_ = bin.read<uint32_t>(28);
  return true;
}

//----------------------------------------------------------------
// SegmentIndexBox::Reference::to_json
//
void
SegmentIndexBox::Reference::to_json(Json::Value & out) const
{
  out["reference_type"] = Json::UInt(reference_type_);
  out["referenced_size"] = Json::UInt(referenced_size_);
  out["subsegment_duration"] = Json::UInt(subsegment_duration_);

  out["starts_with_SAP"] = Json::UInt(starts_with_SAP_);
  out["SAP_type"] = Json::UInt(SAP_type_);
  out["SAP_delta_time"] = Json::UInt(SAP_delta_time_);
}

//----------------------------------------------------------------
// SegmentIndexBox::load
//
void
SegmentIndexBox::load(Mp4Context & mp4, IBitstream & bin)
{
  const std::size_t box_pos = bin.position();
  FullBox::load(mp4, bin);
  const std::size_t box_end = box_pos + Box::size_ * 8;

  references_.clear();
  reference_ID_ = bin.read<uint32_t>();
  timescale_ = bin.read<uint32_t>();

  if (FullBox::version_ == 0)
  {
    earliest_presentation_time_ = bin.read<uint32_t>();
    first_offset_ = bin.read<uint32_t>();
  }
  else
  {
    earliest_presentation_time_ = bin.read<uint64_t>();
    first_offset_ = bin.read<uint64_t>();
  }

  reserved_ = bin.read<uint16_t>();
  reference_count_ = bin.read<uint16_t>();

  for (uint16_t i = 0; i < reference_count_; ++i)
  {
    Reference reference;
    if (!reference.load(bin, box_end))
    {
      break;
    }

    references_.push_back(reference);
  }
}

//----------------------------------------------------------------
// SegmentIndexBox::to_json
//
void
SegmentIndexBox::to_json(Json::Value & out) const
{
  FullBox::to_json(out);

  out["reference_ID"] = reference_ID_;
  out["timescale"] = timescale_;
  out["earliest_presentation_time"] = Json::UInt64(earliest_presentation_time_);
  out["first_offset"] = Json::UInt64(first_offset_);
  out["reference_count"] = reference_count_;

  Json::Value & references = out["references"];
  references = Json::arrayValue;

  for (std::size_t i = 0, n = references_.size(); i < n; ++i)
  {
    const Reference & reference = references_[i];
    Json::Value v;
    reference.to_json(v);
    references.append(v);
  }
}


//----------------------------------------------------------------
// create<SubsegmentIndexBox>::please
//
template SubsegmentIndexBox *
create<SubsegmentIndexBox>::please(const char * fourcc);

//----------------------------------------------------------------
// SubsegmentIndexBox::Subsegment::load
//
bool
SubsegmentIndexBox::Subsegment::load(IBitstream & bin, std::size_t end_pos)
{
  range_count_ = bin.read<uint32_t>();
  for (uint32_t i = 0; i < range_count_; ++i)
  {
    if (end_pos < bin.position() + 32)
    {
      return false;
    }

    uint8_t level = bin.read<uint8_t>();
    uint32_t range_size = bin.read<uint32_t>(24);
    levels_.push_back(level);
    range_sizes_.push_back(range_size);
  }

  return true;
}

//----------------------------------------------------------------
// SubsegmentIndexBox::Subsegment::to_json
//
void
SubsegmentIndexBox::Subsegment::to_json(Json::Value & out) const
{
  out["range_count"] = range_count_;
  yae::save(out["levels"], levels_);
  yae::save(out["range_sizes"], range_sizes_);
}

//----------------------------------------------------------------
// SubsegmentIndexBox::load
//
void
SubsegmentIndexBox::load(Mp4Context & mp4, IBitstream & bin)
{
  const std::size_t box_pos = bin.position();
  FullBox::load(mp4, bin);
  const std::size_t box_end = box_pos + Box::size_ * 8;

  subsegments_.clear();
  subsegment_count_ = bin.read<uint32_t>();

  for (uint32_t i = 0; i < subsegment_count_; ++i)
  {
    subsegments_.push_back(Subsegment());
    Subsegment & subsegment = subsegments_.back();
    if (!subsegment.load(bin, box_end))
    {
      break;
    }
  }
}

//----------------------------------------------------------------
// SubsegmentIndexBox::to_json
//
void
SubsegmentIndexBox::to_json(Json::Value & out) const
{
  FullBox::to_json(out);

  out["subsegment_count"] = subsegment_count_;

  Json::Value & subsegments = out["subsegments"];
  subsegments = Json::arrayValue;

  for (std::size_t i = 0, n = subsegments_.size(); i < n; ++i)
  {
    const Subsegment & subsegment = subsegments_[i];
    Json::Value v;
    subsegment.to_json(v);
    subsegments.append(v);
  }
}


//----------------------------------------------------------------
// create<ProducerReferenceTimeBox>::please
//
template ProducerReferenceTimeBox *
create<ProducerReferenceTimeBox>::please(const char * fourcc);

//----------------------------------------------------------------
// ProducerReferenceTimeBox::load
//
void
ProducerReferenceTimeBox::load(Mp4Context & mp4, IBitstream & bin)
{
  FullBox::load(mp4, bin);

  reference_track_ID_ = bin.read<uint32_t>();
  ntp_timestamp_ = bin.read<uint64_t>();

  if (FullBox::version_ == 0)
  {
    media_time_ = bin.read<uint32_t>();
  }
  else
  {
    media_time_ = bin.read<uint64_t>();
  }
}

//----------------------------------------------------------------
// ProducerReferenceTimeBox::to_json
//
void
ProducerReferenceTimeBox::to_json(Json::Value & out) const
{
  FullBox::to_json(out);
  out["reference_track_ID"] = reference_track_ID_;
  out["ntp_timestamp"] = Json::UInt64(ntp_timestamp_);
  out["media_time"] = Json::UInt64(media_time_);
}


//----------------------------------------------------------------
// create<RtpHintSampleEntryBox>::please
//
template RtpHintSampleEntryBox *
create<RtpHintSampleEntryBox>::please(const char * fourcc);

//----------------------------------------------------------------
// RtpHintSampleEntryBox::load
//
void
RtpHintSampleEntryBox::load(Mp4Context & mp4, IBitstream & bin)
{
  const std::size_t box_pos = bin.position();
  SampleEntryBox::load(mp4, bin);
  const std::size_t box_end = box_pos + Box::size_ * 8;

  hinttrackversion_ = bin.read<uint16_t>();
  highestcompatibleversion_ = bin.read<uint16_t>();
  maxpacketsize_ = bin.read<uint32_t>();

  TSelf::load_children_until(mp4, bin, box_end);
}

//----------------------------------------------------------------
// RtpHintSampleEntryBox::to_json
//
void
RtpHintSampleEntryBox::to_json(Json::Value & out) const
{
  SampleEntryBox::to_json(out);

  out["hinttrackversion"] = hinttrackversion_;
  out["highestcompatibleversion"] = highestcompatibleversion_;
  out["maxpacketsize"] = maxpacketsize_;

  Json::Value & children = out["children"];
  TSelf::children_to_json(children);
}


//----------------------------------------------------------------
// create<TimescaleEntryBox>::please
//
template TimescaleEntryBox *
create<TimescaleEntryBox>::please(const char * fourcc);

//----------------------------------------------------------------
// TimescaleEntryBox::load
//
void
TimescaleEntryBox::load(Mp4Context & mp4, IBitstream & bin)
{
  Box::load(mp4, bin);
  timescale_ = bin.read<uint32_t>();
}

//----------------------------------------------------------------
// TimescaleEntryBox::to_json
//
void
TimescaleEntryBox::to_json(Json::Value & out) const
{
  Box::to_json(out);
  out["timescale"] = timescale_;
}


//----------------------------------------------------------------
// create<TimeOffsetBox>::please
//
template TimeOffsetBox *
create<TimeOffsetBox>::please(const char * fourcc);

//----------------------------------------------------------------
// TimeOffsetBox::load
//
void
TimeOffsetBox::load(Mp4Context & mp4, IBitstream & bin)
{
  Box::load(mp4, bin);
  offset_ = bin.read<uint32_t>();
}

//----------------------------------------------------------------
// TimeOffsetBox::to_json
//
void
TimeOffsetBox::to_json(Json::Value & out) const
{
  Box::to_json(out);
  out["offset"] = offset_;
}


//----------------------------------------------------------------
// create<SRTPProcessBox>::please
//
template SRTPProcessBox *
create<SRTPProcessBox>::please(const char * fourcc);

//----------------------------------------------------------------
// SRTPProcessBox::load
//
void
SRTPProcessBox::load(Mp4Context & mp4, IBitstream & bin)
{
  const std::size_t box_pos = bin.position();
  FullBox::load(mp4, bin);
  const std::size_t box_end = box_pos + Box::size_ * 8;

  encryption_algorithm_rtp_.load(bin);
  encryption_algorithm_rtcp_.load(bin);
  integrity_algorithm_rtp_.load(bin);
  integrity_algorithm_rtcp_.load(bin);

  ContainerEx::load_children_until(mp4, bin, box_end);
}

//----------------------------------------------------------------
// SRTPProcessBox::to_json
//
void
SRTPProcessBox::to_json(Json::Value & out) const
{
  FullBox::to_json(out);

  out["encryption_algorithm_rtp"] = encryption_algorithm_rtp_.str_;
  out["encryption_algorithm_rtcp"] = encryption_algorithm_rtcp_.str_;
  out["integrity_algorithm_rtp"] = integrity_algorithm_rtp_.str_;
  out["integrity_algorithm_rtcp"] = integrity_algorithm_rtcp_.str_;

  Json::Value & children = out["children"];
  ContainerEx::children_to_json(children);
}


//----------------------------------------------------------------
// create<ObjectDescriptorBox>::please
//
template ObjectDescriptorBox *
create<ObjectDescriptorBox>::please(const char * fourcc);

//----------------------------------------------------------------
// ObjectDescriptorBox::load
//
void
ObjectDescriptorBox::load(Mp4Context & mp4, IBitstream & bin)
{
  const std::size_t box_pos = bin.position();
  FullBox::load(mp4, bin);

  const std::size_t box_end = box_pos + Box::size_ * 8;
  data_ = bin.read_bytes_until(box_end);
}

//----------------------------------------------------------------
// ObjectDescriptorBox::to_json
//
void
ObjectDescriptorBox::to_json(Json::Value & out) const
{
  FullBox::to_json(out);
  out["object_descriptor"] = yae::to_hex(data_.get(), data_.size());
}


//----------------------------------------------------------------
// create<VideoMediaHeaderBox>::please
//
template VideoMediaHeaderBox *
create<VideoMediaHeaderBox>::please(const char * fourcc);

//----------------------------------------------------------------
// VideoMediaHeaderBox::VideoMediaHeaderBox
//
VideoMediaHeaderBox::VideoMediaHeaderBox():
  graphicsmode_(0)
{
  opcolor_[0] = 0;
  opcolor_[1] = 0;
  opcolor_[2] = 0;
}

//----------------------------------------------------------------
// VideoMediaHeaderBox::load
//
void
VideoMediaHeaderBox::load(Mp4Context & mp4, IBitstream & bin)
{
  const std::size_t box_pos = bin.position();
  FullBox::load(mp4, bin);
  const std::size_t box_end = box_pos + Box::size_ * 8;

  graphicsmode_ = bin.read<uint16_t>();
  yae::LoadVec<uint16_t>(3, opcolor_, bin, box_end);
}

//----------------------------------------------------------------
// VideoMediaHeaderBox::to_json
//
void
VideoMediaHeaderBox::to_json(Json::Value & out) const
{
  FullBox::to_json(out);
  out["graphicsmode"] = graphicsmode_;
  yae::save(out["opcolor"], opcolor_, 3);
}


//----------------------------------------------------------------
// create<SoundMediaHeaderBox>::please
//
template SoundMediaHeaderBox *
create<SoundMediaHeaderBox>::please(const char * fourcc);

//----------------------------------------------------------------
// SoundMediaHeaderBox::load
//
void
SoundMediaHeaderBox::load(Mp4Context & mp4, IBitstream & bin)
{
  FullBox::load(mp4, bin);
  balance_ = bin.read<uint16_t>();
  reserved_ = bin.read<uint16_t>();
}

//----------------------------------------------------------------
// SoundMediaHeaderBox::to_json
//
void
SoundMediaHeaderBox::to_json(Json::Value & out) const
{
  FullBox::to_json(out);
  out["balance"] = balance_;
  out["reserved"] = reserved_;
}


//----------------------------------------------------------------
// create<TextBox>::please
//
template TextBox *
create<TextBox>::please(const char * fourcc);

//----------------------------------------------------------------
// TextBox::load
//
void
TextBox::load(Mp4Context & mp4, IBitstream & bin)
{
  const std::size_t box_pos = bin.position();
  Box::load(mp4, bin);

  const std::size_t box_end = box_pos + Box::size_ * 8;
  data_.clear();
  load_as_utf8(data_, bin, box_end);
}

//----------------------------------------------------------------
// TextBox::to_json
//
void
TextBox::to_json(Json::Value & out) const
{
  Box::to_json(out);
  out["data"] = data_;
}


//----------------------------------------------------------------
// create<TextFullBox>::please
//
template TextFullBox *
create<TextFullBox>::please(const char * fourcc);

//----------------------------------------------------------------
// TextFullBox::load
//
void
TextFullBox::load(Mp4Context & mp4, IBitstream & bin)
{
  const std::size_t box_pos = bin.position();
  FullBox::load(mp4, bin);

  const std::size_t box_end = box_pos + Box::size_ * 8;
  data_.clear();
  load_as_utf8(data_, bin, box_end);
}

//----------------------------------------------------------------
// TextFullBox::to_json
//
void
TextFullBox::to_json(Json::Value & out) const
{
  FullBox::to_json(out);
  out["data"] = data_;
}


//----------------------------------------------------------------
// create<DataFullBox>::please
//
template DataFullBox *
create<DataFullBox>::please(const char * fourcc);

//----------------------------------------------------------------
// DataFullBox::load
//
void
DataFullBox::load(Mp4Context & mp4, IBitstream & bin)
{
  const std::size_t box_pos = bin.position();
  FullBox::load(mp4, bin);

  const std::size_t box_end = box_pos + Box::size_ * 8;
  data_ = bin.read_bytes_until(box_end);
}

//----------------------------------------------------------------
// DataFullBox::to_json
//
void
DataFullBox::to_json(Json::Value & out) const
{
  FullBox::to_json(out);
  out["data"] = data_.to_hex();
}


//----------------------------------------------------------------
// create<DASHEventMessageBox>::please
//
template DASHEventMessageBox *
create<DASHEventMessageBox>::please(const char * fourcc);

//----------------------------------------------------------------
// DASHEventMessageBox::load
//
void
DASHEventMessageBox::load(Mp4Context & mp4, IBitstream & bin)
{
  const std::size_t box_pos = bin.position();
  FullBox::load(mp4, bin);

  const std::size_t box_end = box_pos + Box::size_ * 8;
  message_data_.clear();

  if (FullBox::version_ == 0)
  {
    bin.read_string_until_null(scheme_id_uri_, box_end);
    bin.read_string_until_null(value_, box_end);
    timescale_ = bin.read<uint32_t>();
    presentation_ = bin.read<uint32_t>(); // presentation_time_delta
    event_duration_ = bin.read<uint32_t>();
    id_ = bin.read<uint32_t>();
  }
  else
  {
    timescale_ = bin.read<uint32_t>();
    presentation_ = bin.read<uint64_t>(); // presentation_time
    event_duration_ = bin.read<uint32_t>();
    id_ = bin.read<uint32_t>();
    bin.read_string_until_null(scheme_id_uri_, box_end);
    bin.read_string_until_null(value_, box_end);
  }

  message_data_ = bin.read_bytes_until(box_end);
}

//----------------------------------------------------------------
// DASHEventMessageBox::to_json
//
void
DASHEventMessageBox::to_json(Json::Value & out) const
{
  FullBox::to_json(out);
  out["scheme_id_uri"] = scheme_id_uri_;
  out["value"] = value_;

  out["timescale"] = timescale_;

  if (FullBox::version_ == 0)
  {
    out["presentation_time_delta"] = Json::UInt64(presentation_);
  }
  else
  {
    out["presentation_time"] = Json::UInt64(presentation_);
  }

  out["event_duration"] = event_duration_;
  out["id"] = id_;

  out["message_data"] = yae::to_hex(message_data_.get(), message_data_.size());
}


//----------------------------------------------------------------
// create<AVCSampleEntryBox>::please
//
template AVCSampleEntryBox *
create<AVCSampleEntryBox>::please(const char * fourcc);


//----------------------------------------------------------------
// create<AVCConfigurationBox>::please
//
template AVCConfigurationBox *
create<AVCConfigurationBox>::please(const char * fourcc);

//----------------------------------------------------------------
// AVCConfigurationBox::load
//
void
AVCConfigurationBox::load(Mp4Context & mp4, IBitstream & bin)
{
  Box::load(mp4, bin);
  cfg_.load(bin);
}

//----------------------------------------------------------------
// AVCConfigurationBox::to_json
//
void
AVCConfigurationBox::to_json(Json::Value & out) const
{
  Box::to_json(out);
  cfg_.save(out["cfg"]);
}


//----------------------------------------------------------------
// create<ESDSBox>::please
//
template ESDSBox *
create<ESDSBox>::please(const char * fourcc);

//----------------------------------------------------------------
// ESDSBox::load
//
void
ESDSBox::load(Mp4Context & mp4, IBitstream & bin)
{
  FullBox::load(mp4, bin);
  es_.load(bin);
}

//----------------------------------------------------------------
// ESDSBox::to_json
//
void
ESDSBox::to_json(Json::Value & out) const
{
  FullBox::to_json(out);
  // es_.save(out["ES_Descriptor"]);
}


//----------------------------------------------------------------
// create<AC3SpecificBox>::please
//
template AC3SpecificBox *
create<AC3SpecificBox>::please(const char * fourcc);

//----------------------------------------------------------------
// AC3SpecificBox::load
//
void
AC3SpecificBox::load(Mp4Context & mp4, IBitstream & bin)
{
  Box::load(mp4, bin);
  fscod_.load(bin);
  bsid_.load(bin);
  bsmod_.load(bin);
  acmod_.load(bin);
  lfeon_.load(bin);
  bit_rate_code_.load(bin);
  reserved_.load(bin);
}

//----------------------------------------------------------------
// AC3SpecificBox::to_json
//
void
AC3SpecificBox::to_json(Json::Value & out) const
{
  Box::to_json(out);
  yae::save(out["fscod"], fscod_.data_);
  yae::save(out["bsid"], bsid_.data_);
  yae::save(out["bsmod"], bsmod_.data_);
  yae::save(out["acmod"], acmod_.data_);
  yae::save(out["lfeon"], lfeon_.data_);
  yae::save(out["bit_rate_code"], bit_rate_code_.data_);
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
// EC3IndependentSubstream::save
//
void
EC3IndependentSubstream::save(Json::Value & json) const
{
  yae::save(json["fscod"], fscod_.data_);
  yae::save(json["bsid"], bsid_.data_);
  yae::save(json["asvc"], asvc_.data_);
  yae::save(json["bsmod"], bsmod_.data_);
  yae::save(json["acmod"], acmod_.data_);
  yae::save(json["lfeon"], lfeon_.data_);
  yae::save(json["num_dep_sub"], num_dep_sub_.data_);

  if (num_dep_sub_.data_ > 0)
  {
    yae::save(json["chan_loc"], chan_loc_.data_);
  }
}


//----------------------------------------------------------------
// create<EC3SpecificBox>::please
//
template EC3SpecificBox *
create<EC3SpecificBox>::please(const char * fourcc);

//----------------------------------------------------------------
// EC3SpecificBox::load
//
void
EC3SpecificBox::load(Mp4Context & mp4, IBitstream & bin)
{
  Box::load(mp4, bin);
  data_rate_.load(bin);
  num_ind_sub_.load(bin);

  for (uint64_t i = 0; i < num_ind_sub_.data_; ++i)
  {
    ind_sub_.push_back(EC3IndependentSubstream());
    EC3IndependentSubstream & ind_sub = ind_sub_.back();
    if (!ind_sub.load(bin))
    {
      break;
    }
  }
}

//----------------------------------------------------------------
// EC3SpecificBox::to_json
//
void
EC3SpecificBox::to_json(Json::Value & out) const
{
  Box::to_json(out);
  yae::save(out["data_rate"], data_rate_.data_);
  yae::save(out["num_ind_sub"], num_ind_sub_.data_);

  Json::Value & ind_subs = out["ind_sub"];
  ind_subs = Json::arrayValue;

  for (std::size_t i = 0, n = ind_sub_.size(); i < n; ++i)
  {
    const EC3IndependentSubstream & ind_sub = ind_sub_[i];
    Json::Value v;
    ind_sub.save(v);
    ind_subs.append(v);
  }
}


//----------------------------------------------------------------
// create<ChannelLayoutBox>::please
//
template ChannelLayoutBox *
create<ChannelLayoutBox>::please(const char * fourcc);

//----------------------------------------------------------------
// ChannelLayoutBox::ChannelLayoutBox
//
ChannelLayoutBox::ChannelLayoutBox():
  stream_structure_(0),
  definedLayout_(0),
  omittedChannelsMap_(0),
  object_count_(0)
{}

//----------------------------------------------------------------
// ChannelLayoutBox::Layout::Layout
//
ChannelLayoutBox::Layout::Layout():
  speaker_position_(0),
  azimuth_(0),
  elevation_(0)
{}

//----------------------------------------------------------------
// ChannelLayoutBox::Layout::load
//
bool
ChannelLayoutBox::Layout::load(IBitstream & bin, std::size_t end_pos)
{
  azimuth_ = 0;
  elevation_ = 0;
  speaker_position_ = 0;

  std::size_t start_pos = bin.position();
  if (end_pos < start_pos + 8)
  {
    return false;
  }

  speaker_position_ = bin.read<uint8_t>();
  if (speaker_position_ != 126)
  {
    return true;
  }

  if (end_pos < start_pos + 32)
  {
    bin.seek(start_pos);
    return false;
  }

  azimuth_ = bin.read<int16_t>();
  elevation_ = bin.read<int8_t>();
  return true;
}

//----------------------------------------------------------------
// ChannelLayoutBox::Layout::to_json
//
void
ChannelLayoutBox::Layout::to_json(Json::Value & out) const
{
  out["speaker_position"] = Json::UInt(speaker_position_);

  if (speaker_position_ == 126)
  {
    out["azimuth"] = Json::Int(azimuth_);
    out["elevation"] = Json::Int(elevation_);
  }
}

//----------------------------------------------------------------
// ChannelLayoutBox::load
//
void
ChannelLayoutBox::load(Mp4Context & mp4, IBitstream & bin)
{
  const std::size_t box_pos = bin.position();
  FullBox::load(mp4, bin);

  const std::size_t box_end = box_pos + Box::size_ * 8;
  layout_.clear();

  stream_structure_ = bin.read<uint8_t>();

  bool channel_structured = !!(stream_structure_ & channelStructured);
  bool object_structured = !!(stream_structure_ & objectStructured);

  if (channel_structured)
  {
    definedLayout_ = bin.read<uint8_t>();

    if (definedLayout_ == 0)
    {
      std::size_t layout_end = box_end - (object_structured ? 8 : 0);
      while (true)
      {
        Layout layout;
        if (!layout.load(bin, layout_end))
        {
          break;
        }

        layout_.push_back(layout);
      }
    }
    else
    {
      omittedChannelsMap_ = bin.read<uint64_t>();
    }
  }

  if (object_structured)
  {
    object_count_ = bin.read<uint8_t>();
  }
}

//----------------------------------------------------------------
// ChannelLayoutBox::to_json
//
void
ChannelLayoutBox::to_json(Json::Value & out) const
{
  FullBox::to_json(out);

  out["stream_structure"] = Json::UInt(stream_structure_);

  bool channel_structured = !!(stream_structure_ & channelStructured);
  bool object_structured = !!(stream_structure_ & objectStructured);

  if (channel_structured)
  {
    out["definedLayout"] = Json::UInt(definedLayout_);

    if (definedLayout_ == 0)
    {
      Json::Value & layouts = out["layouts"];
      layouts = Json::arrayValue;

      for (std::size_t i = 0, n = layout_.size(); i < n; ++i)
      {
        const Layout & layout = layout_[i];
        Json::Value v;
        layout.to_json(v);
        layouts.append(v);
      }
    }
    else
    {
      out["omittedChannelsMap"] = Json::UInt64(omittedChannelsMap_);
    }
  }

  if (object_structured)
  {
    out["object_count"] = Json::UInt(object_count_);
  }
}


//----------------------------------------------------------------
// create<DownMixInstructionsBox>::please
//
template DownMixInstructionsBox *
create<DownMixInstructionsBox>::please(const char * fourcc);

//----------------------------------------------------------------
// DownMixInstructionsBox::DownMixInstructionsBox
//
DownMixInstructionsBox::DownMixInstructionsBox():
  targetLayout_(0),
  reserved_(0),
  targetChannelCount_(0),
  in_stream_(0),
  downmix_ID_(0)
{}

//----------------------------------------------------------------
// DownMixInstructionsBox::load
//
void
DownMixInstructionsBox::load(Mp4Context & mp4, IBitstream & bin)
{
  const std::size_t box_pos = bin.position();
  FullBox::load(mp4, bin);

  const std::size_t box_end = box_pos + Box::size_ * 8;
  bs_downmix_coefficient_.clear();

  targetLayout_ = bin.read<uint8_t>();
  reserved_ = bin.read<uint8_t>(1);
  targetChannelCount_ = bin.read<uint8_t>(7);
  in_stream_ = bin.read<uint8_t>(1);
  downmix_ID_ = bin.read<uint8_t>(7);

  if (!in_stream_ && targetChannelCount_ > 0)
  {
    std::size_t num_coefficients = bin.bits_left() / 4;
    std::size_t baseChannelCount = num_coefficients / targetChannelCount_;
    for (std::size_t i = 0; i < targetChannelCount_; ++i)
    {
      for (std::size_t j = 0; j < baseChannelCount; ++j)
      {
        uint8_t bs_downmix_coefficient = bin.read<uint8_t>(4);
        bs_downmix_coefficient_.push_back(bs_downmix_coefficient);
      }
    }
  }
}

//----------------------------------------------------------------
// DownMixInstructionsBox::to_json
//
void
DownMixInstructionsBox::to_json(Json::Value & out) const
{
  FullBox::to_json(out);

  out["targetLayout"] = Json::UInt(targetLayout_);
  out["reserved"] = Json::UInt(reserved_);
  out["targetChannelCount"] = Json::UInt(targetChannelCount_);
  out["in_stream"] = Json::UInt(in_stream_);
  out["downmix_ID"] = Json::UInt(downmix_ID_);

  yae::save(out["bs_downmix_coefficient"], bs_downmix_coefficient_);
}


//----------------------------------------------------------------
// create<LoudnessBaseBox>::please
//
template LoudnessBaseBox *
create<LoudnessBaseBox>::please(const char * fourcc);

//----------------------------------------------------------------
// LoudnessBaseBox::LoudnessBaseBox
//
LoudnessBaseBox::LoudnessBaseBox():
  reserved_(0),
  downmix_ID_(0),
  DRC_set_ID_(0),
  bs_sample_peak_level_(0),
  bs_true_peak_level_(0),
  measurement_system_for_TP_(0),
  reliability_for_TP_(0),
  measurement_count_(0)
{}

//----------------------------------------------------------------
// LoudnessBaseBox::Measurement::load
//
bool
LoudnessBaseBox::Measurement::load(IBitstream & bin, std::size_t end_pos)
{
  if (end_pos < bin.position() + 24)
  {
    return false;
  }

  method_definition_ = bin.read<uint8_t>();
  method_value_ = bin.read<uint8_t>();
  measurement_system_ = bin.read<uint8_t>(4);
  reliability_ = bin.read<uint8_t>(4);
  return true;
}

//----------------------------------------------------------------
// LoudnessBaseBox::Measurement::to_json
//
void
LoudnessBaseBox::Measurement::to_json(Json::Value & out) const
{
  out["method_definition"] = Json::UInt(method_definition_);
  out["method_value"] = Json::UInt(method_value_);
  out["measurement_system"] = Json::UInt(measurement_system_);
  out["reliability"] = Json::UInt(reliability_);
}

//----------------------------------------------------------------
// LoudnessBaseBox::load
//
void
LoudnessBaseBox::load(Mp4Context & mp4, IBitstream & bin)
{
  const std::size_t box_pos = bin.position();
  FullBox::load(mp4, bin);

  const std::size_t box_end = box_pos + Box::size_ * 8;
  measurement_.clear();

  reserved_ = bin.read<uint8_t>(3);
  downmix_ID_ = bin.read<uint8_t>(7);
  DRC_set_ID_ = bin.read<uint8_t>(6);

  bs_sample_peak_level_ = bin.read<int16_t>(12);
  bs_true_peak_level_ = bin.read<int16_t>(12);

  measurement_system_for_TP_ = bin.read<uint8_t>(4);
  reliability_for_TP_ = bin.read<uint8_t>(4);
  measurement_count_ = bin.read<uint8_t>();

  for (uint8_t i = 0; i < measurement_count_; ++i)
  {
    Measurement measurement;
    if (!measurement.load(bin, box_end))
    {
      break;
    }

    measurement_.push_back(measurement);
  }
}

//----------------------------------------------------------------
// LoudnessBaseBox::to_json
//
void
LoudnessBaseBox::to_json(Json::Value & out) const
{
  FullBox::to_json(out);

  out["reserved"] = Json::UInt(reserved_);
  out["downmix_ID"] = Json::UInt(downmix_ID_);
  out["DRC_set_ID"] = Json::UInt(DRC_set_ID_);

  out["bs_sample_peak_level"] = Json::Int(bs_sample_peak_level_);
  out["bs_true_peak_level"] = Json::Int(bs_true_peak_level_);

  out["measurement_system_for_TP"] = Json::UInt(measurement_system_for_TP_);
  out["reliability_for_TP"] = Json::UInt(reliability_for_TP_);
  out["measurement_count"] = Json::UInt(measurement_count_);

  Json::Value & measurements = out["measurements"];
  measurements = Json::arrayValue;

  for (std::size_t i = 0, n = measurement_.size(); i < n; ++i)
  {
    const Measurement & measurement = measurement_[i];
    Json::Value v;
    measurement.to_json(v);
    measurements.append(v);
  }
}

//----------------------------------------------------------------
// create<XMLMetaDataSampleEntryBox>::please
//
template XMLMetaDataSampleEntryBox *
create<XMLMetaDataSampleEntryBox>::please(const char * fourcc);

//----------------------------------------------------------------
// XMLMetaDataSampleEntryBox::load
//
void
XMLMetaDataSampleEntryBox::load(Mp4Context & mp4, IBitstream & bin)
{
  const std::size_t box_pos = bin.position();
  SampleEntryBox::load(mp4, bin);

  const std::size_t box_end = box_pos + Box::size_ * 8;

  content_encoding_.clear();
  bin.read_string_until_null(content_encoding_, box_end);

  namespace_.clear();
  bin.read_string_until_null(namespace_, box_end);

  schema_location_.clear();
  bin.read_string_until_null(schema_location_, box_end);

  TSelf::load_children_until(mp4, bin, box_end);
}

//----------------------------------------------------------------
// XMLMetaDataSampleEntryBox::to_json
//
void
XMLMetaDataSampleEntryBox::to_json(Json::Value & out) const
{
  SampleEntryBox::to_json(out);

  out["content_encoding"] = content_encoding_;
  out["namespace"] = namespace_;
  out["schema_location"] = schema_location_;

  Json::Value & children = out["children"];
  TSelf::children_to_json(children);
}


//----------------------------------------------------------------
// create<TextMetaDataSampleEntryBox>::please
//
template TextMetaDataSampleEntryBox *
create<TextMetaDataSampleEntryBox>::please(const char * fourcc);

//----------------------------------------------------------------
// TextMetaDataSampleEntryBox::load
//
void
TextMetaDataSampleEntryBox::load(Mp4Context & mp4, IBitstream & bin)
{
  const std::size_t box_pos = bin.position();
  SampleEntryBox::load(mp4, bin);

  const std::size_t box_end = box_pos + Box::size_ * 8;

  content_encoding_.clear();
  bin.read_string_until_null(content_encoding_, box_end);

  mime_format_.clear();
  bin.read_string_until_null(mime_format_, box_end);

  TSelf::load_children_until(mp4, bin, box_end);
}

//----------------------------------------------------------------
// TextMetaDataSampleEntryBox::to_json
//
void
TextMetaDataSampleEntryBox::to_json(Json::Value & out) const
{
  SampleEntryBox::to_json(out);

  out["content_encoding"] = content_encoding_;
  out["mime_format"] = mime_format_;

  Json::Value & children = out["children"];
  TSelf::children_to_json(children);
}


//----------------------------------------------------------------
// create<URIMetaSampleEntryBox>::please
//
template URIMetaSampleEntryBox *
create<URIMetaSampleEntryBox>::please(const char * fourcc);


//----------------------------------------------------------------
// create<HintMediaHeaderBox>::please
//
template HintMediaHeaderBox *
create<HintMediaHeaderBox>::please(const char * fourcc);

//----------------------------------------------------------------
// HintMediaHeaderBox::HintMediaHeaderBox
//
HintMediaHeaderBox::HintMediaHeaderBox():
  maxPDUsize_(0),
  avgPDUsize_(0),
  maxbitrate_(0),
  avgbitrate_(0),
  reserved_(0)
{}

//----------------------------------------------------------------
// HintMediaHeaderBox::load
//
void
HintMediaHeaderBox::load(Mp4Context & mp4, IBitstream & bin)
{
  const std::size_t box_pos = bin.position();
  FullBox::load(mp4, bin);

  maxPDUsize_ = bin.read<uint16_t>();
  avgPDUsize_ = bin.read<uint16_t>();
  maxbitrate_ = bin.read<uint32_t>();
  avgbitrate_ = bin.read<uint32_t>();
  reserved_ = bin.read<uint32_t>();
}

//----------------------------------------------------------------
// HintMediaHeaderBox::to_json
//
void
HintMediaHeaderBox::to_json(Json::Value & out) const
{
  FullBox::to_json(out);

  out["maxPDUsize"] = maxPDUsize_;
  out["avgPDUsize"] = avgPDUsize_;
  out["maxbitrate"] = maxbitrate_;
  out["avgbitrate"] = avgbitrate_;
  out["reserved"] = reserved_;
}


//----------------------------------------------------------------
// create<XMLSubtitleSampleEntryBox>::please
//
template XMLSubtitleSampleEntryBox *
create<XMLSubtitleSampleEntryBox>::please(const char * fourcc);

//----------------------------------------------------------------
// XMLSubtitleSampleEntryBox::load
//
void
XMLSubtitleSampleEntryBox::load(Mp4Context & mp4, IBitstream & bin)
{
  const std::size_t box_pos = bin.position();
  SampleEntryBox::load(mp4, bin);

  const std::size_t box_end = box_pos + Box::size_ * 8;

  namespace_.clear();
  bin.read_string_until_null(namespace_, box_end);

  schema_location_.clear();
  bin.read_string_until_null(schema_location_, box_end);

  auxiliary_mime_types_.clear();
  bin.read_string_until_null(auxiliary_mime_types_, box_end);

  TSelf::load_children_until(mp4, bin, box_end);
}

//----------------------------------------------------------------
// XMLSubtitleSampleEntryBox::to_json
//
void
XMLSubtitleSampleEntryBox::to_json(Json::Value & out) const
{
  SampleEntryBox::to_json(out);

  out["namespace"] = namespace_;
  out["schema_location"] = schema_location_;
  out["auxiliary_mime_types"] = auxiliary_mime_types_;

  Json::Value & children = out["children"];
  TSelf::children_to_json(children);
}


//----------------------------------------------------------------
// Mp4BoxFactory
//
struct Mp4BoxFactory : public BoxFactory
{
  // populate the LUT in the constructor:
  Mp4BoxFactory()
  {
    TBoxConstructor create_container =
      (TBoxConstructor)(create<Container>::please);
    this->add("moov", create_container);
    this->add("mvex", create_container);
    this->add("trak", create_container);

    // contains only TrackReferenceTypeBox(es)...
    this->add("tref", create_container);

    this->add("trgr", create_container);
    this->add("moof", create_container);
    this->add("traf", create_container);
    this->add("mfra", create_container);
    this->add("edts", create_container);
    this->add("mdia", create_container);
    this->add("minf", create_container);
    this->add("dinf", create_container);
    this->add("stbl", create_container);
    this->add("sinf", create_container);
    this->add("schi", create_container);
    this->add("udta", create_container);
    this->add("ludt", create_container);
    this->add("meco", create_container);
    this->add("paen", create_container);
    this->add("strk", create_container);
    this->add("strd", create_container);
    this->add("rinf", create_container);
    this->add("cinf", create_container);
    this->add("\xa9too", create_container);

    this->add("meta", create<ContainerEx>::please);

    this->add("stsd", create<ContainerList32>::please);
    this->add("dref", create<ContainerList32>::please);

    this->add("ftyp", create<FileTypeBox>::please);
    this->add("styp", create<FileTypeBox>::please);
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
    this->add("clap", create<CleanApertureBox>::please);
    this->add("pasp", create<PixelAspectRatioBox>::please);
    this->add("colr", create<ColourInformationBox>::please);
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
    this->add("tfhd", create<TrackFragmentHeaderBox>::please);
    this->add("trun", create<TrackRunBox>::please);
    this->add("tfra", create<TrackFragmentRandomAccessBox>::please);
    this->add("mfro", create<MovieFragmentRandomAccessOffsetBox>::please);
    this->add("tfdt", create<TrackFragmentBaseMediaDecodeTimeBox>::please);
    this->add("leva", create<LevelAssignmentBox>::please);
    this->add("trep", create<TrackExtensionPropertiesBox>::please);
    this->add("assp", create<AlternativeStartupSequencePropertiesBox>::please);
    this->add("sbgp", create<SampleToGroupBox>::please);
    this->add("sgpd", create<SampleGroupDescriptionBox>::please);
    this->add("cprt", create<CopyrightBox>::please);
    this->add("tsel", create<TrackSelectionBox>::please);
    this->add("kind", create<KindBox>::please);
    this->add("xml ", create<XMLBox>::please);
    this->add("bxml", create<BinaryXMLBox>::please);
    this->add("iloc", create<ItemLocationBox>::please);
    this->add("pitm", create<PrimaryItemBox>::please);
    this->add("ipro", create<ContainerList16>::please);
    this->add("infe", create<ItemInfoEntryBox>::please);
    this->add("iinf", create<ItemInfoBox>::please);
    this->add("mere", create<MetaboxRelationBox>::please);
    this->add("idat", create<ItemDataBox>::please);
    this->add("iref", create<ItemReferenceBox>::please);
    this->add("frma", create<OriginalFormatBox>::please);
    this->add("schm", create<SchemeTypeBox>::please);
    this->add("fiin", create<FDItemInformationBox>::please);
    this->add("fpar", create<FilePartitionBox>::please);
    this->add("fecr", create<FECReservoirBox>::please);
    this->add("segr", create<FDSessionGroupBox>::please);
    this->add("gitn", create<GroupIdToNameBox>::please);
    this->add("fire", create<FileReservoirBox>::please);
    this->add("stri", create<SubTrackInformationBox>::please);
    this->add("stsg", create<SubTrackSampleGroupBox>::please);
    this->add("stvi", create<StereoVideoBox>::please);
    this->add("sidx", create<SegmentIndexBox>::please);
    this->add("ssix", create<SubsegmentIndexBox>::please);
    this->add("prft", create<ProducerReferenceTimeBox>::please);
    this->add("icpv", create<VisualSampleEntryBox>::please);
    this->add("rtp ", create<RtpHintSampleEntryBox>::please);
    this->add("srtp", create<RtpHintSampleEntryBox>::please);
    this->add("tims", create<TimescaleEntryBox>::please);
    this->add("tsro", create<TimeOffsetBox>::please);
    this->add("snro", create<TimeOffsetBox>::please);
    this->add("srpp", create<SRTPProcessBox>::please);
    this->add("iods", create<ObjectDescriptorBox>::please);
    this->add("vmhd", create<VideoMediaHeaderBox>::please);
    this->add("smhd", create<SoundMediaHeaderBox>::please);
    this->add("gmhd", create_container);
    this->add("name", create<TextBox>::please);
    this->add("emsg", create<DASHEventMessageBox>::please);
    this->add("avc1", create<AVCSampleEntryBox>::please);
    this->add("avc2", create<AVCSampleEntryBox>::please);
    this->add("avc3", create<AVCSampleEntryBox>::please);
    this->add("avc4", create<AVCSampleEntryBox>::please);
    this->add("avcC", create<AVCConfigurationBox>::please);
    this->add("m4ae", create<AudioSampleEntryBox>::please);
    this->add("mp4a", create<AudioSampleEntryBox>::please);
    this->add("enca", create<AudioSampleEntryBox>::please);
    this->add("soun", create<AudioSampleEntryBox>::please);
    this->add("ac-3", create<AudioSampleEntryBox>::please);
    this->add("ec-3", create<AudioSampleEntryBox>::please);
    this->add("mp4v", create<VisualSampleEntryBox>::please);
    this->add("encv", create<VisualSampleEntryBox>::please);
    this->add("vide", create<VisualSampleEntryBox>::please);
    this->add("mp4s", create<MpegSampleEntryBox>::please);
    this->add("esds", create<ESDSBox>::please);
    this->add("dac3", create<AC3SpecificBox>::please);
    this->add("dec3", create<EC3SpecificBox>::please);
    this->add("chnl", create<ChannelLayoutBox>::please);
    this->add("dmix", create<DownMixInstructionsBox>::please);
    this->add("tlou", create<LoudnessBaseBox>::please);
    this->add("alou", create<LoudnessBaseBox>::please);
    this->add("metx", create<XMLMetaDataSampleEntryBox>::please);
    this->add("mett", create<TextMetaDataSampleEntryBox>::please);
    this->add("stxt", create<TextMetaDataSampleEntryBox>::please);
    this->add("sbtt", create<TextMetaDataSampleEntryBox>::please);
    this->add("txtC", create<TextFullBox>::please);
    this->add("uri ", create<TextFullBox>::please);
    this->add("uriI", create<DataFullBox>::please);
    this->add("urim", create<URIMetaSampleEntryBox>::please);
    this->add("hmhd", create<HintMediaHeaderBox>::please);
    this->add("stpp", create<XMLSubtitleSampleEntryBox>::please);
    this->add("msrc", create<TrackGroupTypeBox>::please);
  }

  // helper:
  static const Mp4BoxFactory & singleton()
  {
    static const Mp4BoxFactory singleton_;
    return singleton_;
  }
};


//----------------------------------------------------------------
// yae::qtff::CountryList::load
//
void
yae::qtff::CountryList::load(IBitstream & bin)
{
  count_ = bin.read<uint16_t>();
  for (uint16_t i = 0; i < count_; ++i)
  {
    countries_.push_back(TwoCC());
    TwoCC & cc = countries_.back();
    cc.load(bin);
  }
}

//----------------------------------------------------------------
// create<yae::qtff::CountryListAtom>::please
//
template yae::qtff::CountryListAtom *
create<yae::qtff::CountryListAtom>::please(const char * fourcc);

//----------------------------------------------------------------
// yae::qtff::CountryListAtom::load
//
void
yae::qtff::CountryListAtom::load(Mp4Context & mp4, IBitstream & bin)
{
  const std::size_t box_pos = bin.position();
  FullBox::load(mp4, bin);

  const std::size_t box_end = box_pos + Box::size_ * 8;
  entry_count_ = bin.read<uint32_t>();

  for (uint32_t i = 0; i < entry_count_ && bin.position() < box_end; ++i)
  {
    entries_.push_back(yae::qtff::CountryList());
    yae::qtff::CountryList & entry = entries_.back();
    entry.load(bin);
  }
}

//----------------------------------------------------------------
// yae::qtff::CountryListAtom::to_json
//
void
yae::qtff::CountryListAtom::to_json(Json::Value & out) const
{
  FullBox::to_json(out);
  out["entry_count"] = entry_count_;
  yae::save(out["entries"], entries_);
}


//----------------------------------------------------------------
// yae::qtff::LanguageList::load
//
void
yae::qtff::LanguageList::load(IBitstream & bin)
{
  count_ = bin.read<uint16_t>();
  for (uint16_t i = 0; i < count_; ++i)
  {
    languages_.push_back(iso_639_2t::PackedLang());
    iso_639_2t::PackedLang & language = languages_.back();
    language.load(bin);
  }
}

//----------------------------------------------------------------
// create<yae::qtff::LanguageListAtom>::please
//
template yae::qtff::LanguageListAtom *
create<yae::qtff::LanguageListAtom>::please(const char * fourcc);

//----------------------------------------------------------------
// yae::qtff::LanguageListAtom::load
//
void
yae::qtff::LanguageListAtom::load(Mp4Context & mp4, IBitstream & bin)
{
  const std::size_t box_pos = bin.position();
  FullBox::load(mp4, bin);

  const std::size_t box_end = box_pos + Box::size_ * 8;
  entry_count_ = bin.read<uint32_t>();

  for (uint32_t i = 0; i < entry_count_ && bin.position() < box_end; ++i)
  {
    entries_.push_back(LanguageList());
    LanguageList & entry = entries_.back();
    entry.load(bin);
  }
}

//----------------------------------------------------------------
// yae::qtff::LanguageListAtom::to_json
//
void
yae::qtff::LanguageListAtom::to_json(Json::Value & out) const
{
  FullBox::to_json(out);
  out["entry_count"] = entry_count_;
  yae::save(out["entries"], entries_);
}


//----------------------------------------------------------------
// create<yae::qtff::MetadataItemKeysAtom>::please
//
template yae::qtff::MetadataItemKeysAtom *
create<yae::qtff::MetadataItemKeysAtom>::please(const char * fourcc);

//----------------------------------------------------------------
// yae::qtff::MetadataItemKeysAtom::load
//
void
yae::qtff::MetadataItemKeysAtom::load(Mp4Context & mp4, IBitstream & bin)
{
  const std::size_t box_pos = bin.position();
  FullBox::load(mp4, bin);

  children_.clear();

  const std::size_t box_end = box_pos + Box::size_ * 8;
  entry_count_ = bin.read<uint32_t>();

  for (uint32_t i = 0; i < entry_count_ && bin.position() < box_end; ++i)
  {
    TBoxPtr box(new yae::qtff::MetadataItemKeyAtom());
    box->load(mp4, bin);
    YAE_ASSERT(!box->type_.same_as("uuid"));
    children_.push_back(box);
  }
}


//----------------------------------------------------------------
// create<yae::qtff::ItemInfoAtom>::please
//
template yae::qtff::ItemInfoAtom *
create<yae::qtff::ItemInfoAtom>::please(const char * fourcc);

//----------------------------------------------------------------
// yae::qtff::ItemInfoAtom::load
//
void
yae::qtff::ItemInfoAtom::load(Mp4Context & mp4, IBitstream & bin)
{
  FullBox::load(mp4, bin);
  item_id_ = bin.read<uint32_t>();
}

//----------------------------------------------------------------
// yae::qtff::ItemInfoAtom::to_json
//
void
yae::qtff::ItemInfoAtom::to_json(Json::Value & out) const
{
  FullBox::to_json(out);
  out["item_id"] = item_id_;
}


//----------------------------------------------------------------
// yae::qtff::TypeIndicator::load
//
void
yae::qtff::TypeIndicator::load(IBitstream & bin)
{
  indicator_byte_ = bin.read<uint32_t>(8);
  YAE_ASSERT(indicator_byte_ == 0);
  well_known_type_ = bin.read<uint32_t>(24);
}


//----------------------------------------------------------------
// yae::qtff::LocaleIndicator::load
//
void
yae::qtff::LocaleIndicator::load(IBitstream & bin)
{
  country_ = bin.read<uint16_t>();
  language_ = bin.read<uint16_t>();
}


//----------------------------------------------------------------
// create<yae::qtff::MetadataValueAtom>::please
//
template yae::qtff::MetadataValueAtom *
create<yae::qtff::MetadataValueAtom>::please(const char * fourcc);

//----------------------------------------------------------------
// yae::qtff::MetadataValueAtom::load
//
void
yae::qtff::MetadataValueAtom::load(Mp4Context & mp4, IBitstream & bin)
{
  const std::size_t box_pos = bin.position();
  Box::load(mp4, bin);

  const std::size_t box_end = box_pos + Box::size_ * 8;
  type_indicator_.load(bin);
  locale_indicator_.load(bin);
  value_ = bin.read_bytes_until(box_end);
}

//----------------------------------------------------------------
// yae::qtff::MetadataValueAtom::to_json
//
void
yae::qtff::MetadataValueAtom::to_json(Json::Value & out) const
{
  Box::to_json(out);
  yae::save(out["type_indicator"], type_indicator_);
  yae::save(out["locale_indicator"], locale_indicator_);
  out["value"] = yae::to_hex(value_.get(), value_.size());
}


//----------------------------------------------------------------
// yae::qtff::MetadataItemAtom::load
//
void
yae::qtff::MetadataItemAtom::load(Mp4Context & mp4, IBitstream & bin)
{
  const std::size_t box_pos = bin.position();
  Box::load(mp4, bin);

  children_.clear();

  const std::size_t box_end = box_pos + Box::size_ * 8;
  while (bin.position() < box_end)
  {
    Box next;
    next.peek_box_type(mp4, bin);

    TBoxPtr box;
    if (next.type_.same_as("itif"))
    {
      box.reset(new ItemInfoAtom());
    }
    else if (next.type_.same_as("name"))
    {
      box.reset(new NameAtom());
    }
    else
    {
      box.reset(new MetadataValueAtom());
    }

    box->load(mp4, bin);
    children_.push_back(box);
  }
}


//----------------------------------------------------------------
// create<yae::qtff::MetadataItemListAtom>::please
//
template yae::qtff::MetadataItemListAtom *
create<yae::qtff::MetadataItemListAtom>::please(const char * fourcc);

//----------------------------------------------------------------
// yae::qtff::MetadataItemListAtom::load
//
void
yae::qtff::MetadataItemListAtom::load(Mp4Context & mp4, IBitstream & bin)
{
  const std::size_t box_pos = bin.position();
  Box::load(mp4, bin);

  children_.clear();

  const std::size_t box_end = box_pos + Box::size_ * 8;
  while (bin.position() < box_end)
  {
    TBoxPtr box(new MetadataItemAtom());
    box->load(mp4, bin);
    children_.push_back(box);
  }
}


//----------------------------------------------------------------
// create<yae::qtff::ApertureDimensionsAtom>::please
//
template yae::qtff::ApertureDimensionsAtom *
create<yae::qtff::ApertureDimensionsAtom>::please(const char * fourcc);

//----------------------------------------------------------------
// yae::qtff::ApertureDimensionsAtom::load
//
void
yae::qtff::ApertureDimensionsAtom::load(Mp4Context & mp4, IBitstream & bin)
{
  FullBox::load(mp4, bin);
  width_ = bin.read<uint32_t>();
  height_ = bin.read<uint32_t>();
}

//----------------------------------------------------------------
// yae::qtff::ApertureDimensionsAtom::to_json
//
void
yae::qtff::ApertureDimensionsAtom::to_json(Json::Value & out) const
{
  FullBox::to_json(out);
  out["width"] = double(width_) / double(0x00010000);
  out["height"] = double(height_) / double(0x00010000);
}


//----------------------------------------------------------------
// create<yae::qtff::BaseMediaInfoAtom>::please
//
template yae::qtff::BaseMediaInfoAtom *
create<yae::qtff::BaseMediaInfoAtom>::please(const char * fourcc);

//----------------------------------------------------------------
// yae::qtff::BaseMediaInfoAtom::BaseMediaInfoAtom
//
yae::qtff::BaseMediaInfoAtom::BaseMediaInfoAtom():
  graphicsmode_(0),
  balance_(0),
  reserved_(0)
{
  opcolor_[0] = 0;
  opcolor_[1] = 0;
  opcolor_[2] = 0;
}

//----------------------------------------------------------------
// yae::qtff::BaseMediaInfoAtom::load
//
void
yae::qtff::BaseMediaInfoAtom::load(Mp4Context & mp4, IBitstream & bin)
{
  const std::size_t box_pos = bin.position();
  FullBox::load(mp4, bin);
  const std::size_t box_end = box_pos + Box::size_ * 8;

  graphicsmode_ = bin.read<uint16_t>();
  yae::LoadVec<uint16_t>(3, opcolor_, bin, box_end);
  balance_ = bin.read<uint16_t>();
  reserved_ = bin.read<uint16_t>();
}

//----------------------------------------------------------------
// yae::qtff::BaseMediaInfoAtom::to_json
//
void
yae::qtff::BaseMediaInfoAtom::to_json(Json::Value & out) const
{
  FullBox::to_json(out);
  out["graphicsmode"] = graphicsmode_;
  yae::save(out["opcolor"], opcolor_, 3);
  out["balance"] = balance_;
  out["reserved"] = reserved_;
}


//----------------------------------------------------------------
// create<yae::qtff::SampleDescriptionAtom>::please
//
template yae::qtff::SampleDescriptionAtom *
create<yae::qtff::SampleDescriptionAtom>::please(const char * fourcc);

//----------------------------------------------------------------
// yae::qtff::SampleDescriptionAtom::SampleDescriptionAtom
//
yae::qtff::SampleDescriptionAtom::SampleDescriptionAtom():
  data_reference_index_(0)
{
  reserved_[0] = 0;
  reserved_[1] = 0;
  reserved_[2] = 0;
  reserved_[3] = 0;
  reserved_[4] = 0;
  reserved_[5] = 0;
}

//----------------------------------------------------------------
// yae::qtff::SampleDescriptionAtom::load
//
std::size_t
yae::qtff::SampleDescriptionAtom::load_base(Mp4Context & mp4, IBitstream & bin)
{
  const std::size_t box_pos = bin.position();
  Box::load(mp4, bin);

  const std::size_t box_end = box_pos + Box::size_ * 8;
  yae::LoadVec<uint8_t>(6, reserved_, bin, box_end);
  data_reference_index_ = bin.read<uint16_t>();

  return box_pos;
}

//----------------------------------------------------------------
// yae::qtff::SampleDescriptionAtom::load
//
void
yae::qtff::SampleDescriptionAtom::load(Mp4Context & mp4, IBitstream & bin)
{
  const std::size_t box_pos = this->load_base(mp4, bin);
  const std::size_t box_end = box_pos + Box::size_ * 8;
  this->load_children_until(mp4, bin, box_end);
}

//----------------------------------------------------------------
// yae::qtff::SampleDescriptionAtom::to_json
//
void
yae::qtff::SampleDescriptionAtom::to_json(Json::Value & out) const
{
  yae::mp4::Container::to_json(out);
  yae::save(out["reserved"], reserved_, 6);
  out["data_reference_index"] = data_reference_index_;
}


//----------------------------------------------------------------
// create<yae::qtff::TimecodeSampleDescAtom>::please
//
template yae::qtff::TimecodeSampleDescAtom *
create<yae::qtff::TimecodeSampleDescAtom>::please(const char * fourcc);

//----------------------------------------------------------------
// yae::qtff::TimecodeSampleDescAtom::TimecodeSampleDescAtom
//
yae::qtff::TimecodeSampleDescAtom::TimecodeSampleDescAtom():
  reserved1_(0),
  flags_(0),
  timescale_(0),
  frame_duration_(0),
  frames_per_second_(0),
  reserved2_(0)
{}

//----------------------------------------------------------------
// yae::qtff::TimecodeSampleDescAtom::load
//
void
yae::qtff::TimecodeSampleDescAtom::load(Mp4Context & mp4, IBitstream & bin)
{
  const std::size_t box_pos = this->load_base(mp4, bin);
  const std::size_t box_end = box_pos + Box::size_ * 8;

  reserved1_ = bin.read<uint32_t>();
  flags_ = bin.read<uint32_t>();
  timescale_ = bin.read<uint32_t>();
  frame_duration_ = bin.read<uint32_t>();
  frames_per_second_ = bin.read<uint8_t>();

  std::size_t zero_bits = std::min<std::size_t>(24, bin.bits_left());
  reserved2_ = bin.read<uint32_t>(zero_bits);

  this->load_children_until(mp4, bin, box_end);
}

//----------------------------------------------------------------
// yae::qtff::TimecodeSampleDescAtom::to_json
//
void
yae::qtff::TimecodeSampleDescAtom::to_json(Json::Value & out) const
{
  SampleDescriptionAtom::to_json(out);

  out["reserved1"] = reserved1_;
  out["flags"] = flags_;
  out["timescale"] = timescale_;
  out["frame_duration"] = frame_duration_;
  out["frames_per_second"] = Json::UInt(frames_per_second_);
  out["reserved2"] = Json::UInt(reserved2_);
}


//----------------------------------------------------------------
// create<yae::qtff::TimecodeMediaInfoAtom>::please
//
template yae::qtff::TimecodeMediaInfoAtom *
create<yae::qtff::TimecodeMediaInfoAtom>::please(const char * fourcc);

//----------------------------------------------------------------
// yae::qtff::TimecodeMediaInfoAtom::TimecodeMediaInfoAtom
//
yae::qtff::TimecodeMediaInfoAtom::TimecodeMediaInfoAtom():
  text_font_(0),
  text_face_(0),
  text_size_(0),
  reserved_(0)
{
  text_color_[0] = 0;
  text_color_[1] = 0;
  text_color_[2] = 0;

  background_color_[0] = 0;
  background_color_[1] = 0;
  background_color_[2] = 0;
}

//----------------------------------------------------------------
// yae::qtff::TimecodeMediaInfoAtom::load
//
void
yae::qtff::TimecodeMediaInfoAtom::load(Mp4Context & mp4, IBitstream & bin)
{
  const std::size_t box_pos = bin.position();
  yae::mp4::FullBox::load(mp4, bin);

  const std::size_t box_end = box_pos + Box::size_ * 8;
  text_font_ = bin.read<uint16_t>();
  text_face_ = bin.read<uint16_t>();
  text_size_ = bin.read<uint16_t>();
  reserved_ = bin.read<uint16_t>();

  yae::LoadVec<uint16_t>(3, text_color_, bin, box_end);
  yae::LoadVec<uint16_t>(3, background_color_, bin, box_end);

  font_name_.clear();
  bin.read_pascal_string(font_name_);
}

//----------------------------------------------------------------
// yae::qtff::TimecodeMediaInfoAtom::to_json
//
void
yae::qtff::TimecodeMediaInfoAtom::to_json(Json::Value & out) const
{
  yae::mp4::FullBox::to_json(out);

  out["text_font"] = text_font_;
  out["text_face"] = text_face_;
  out["text_size"] = text_size_;
  out["reserved"] = reserved_;

  yae::save(out["text_color"], text_color_, 3);
  yae::save(out["background_color"], background_color_, 3);

  out["font_name"] = font_name_;
}


//----------------------------------------------------------------
// QuickTimeAtomFactory
//
struct QuickTimeAtomFactory : public BoxFactory
{
  // populate the LUT in the constructor:
  QuickTimeAtomFactory()
  {
    this->add("ctry", create<yae::qtff::CountryListAtom>::please);
    this->add("lang", create<yae::qtff::LanguageListAtom>::please);
    this->add("keys", create<yae::qtff::MetadataItemKeysAtom>::please);
    this->add("data", create<yae::qtff::MetadataValueAtom>::please);
    this->add("name", create<yae::qtff::NameAtom>::please);
    this->add("itif", create<yae::qtff::ItemInfoAtom>::please);
    this->add("ilst", create<yae::qtff::MetadataItemListAtom>::please);
    this->add("tapt", create<yae::mp4::Container>::please);
    this->add("clef", create<yae::qtff::ApertureDimensionsAtom>::please);
    this->add("prof", create<yae::qtff::ApertureDimensionsAtom>::please);
    this->add("enof", create<yae::qtff::ApertureDimensionsAtom>::please);
    this->add("gmin", create<yae::qtff::BaseMediaInfoAtom>::please);
    this->add("c608", create<yae::qtff::SampleDescriptionAtom>::please);
    this->add("tcmi", create<yae::qtff::TimecodeMediaInfoAtom>::please);
  }

  // helper:
  static const QuickTimeAtomFactory & singleton()
  {
    static const QuickTimeAtomFactory singleton_;
    return singleton_;
  }
};


//----------------------------------------------------------------
// Mp4Context::SetAncestor::SetAncestor
//
Mp4Context::SetAncestor::SetAncestor(Mp4Context & mp4, const mp4::Box * box):
  mp4_(mp4)
{
  mp4_.ancestors_.push_back(box);
}

//----------------------------------------------------------------
// Mp4Context::SetAncestor::~SetAncestor
//
Mp4Context::SetAncestor::~SetAncestor()
{
  mp4_.ancestors_.pop_back();
}

//----------------------------------------------------------------
// Mp4Context::is_ancestor_type
//
bool
Mp4Context::is_ancestor_type(const char * fourcc) const
{
  return (ancestors_.empty() ? false :
          ancestors_.back()->type_.same_as(fourcc));
}

//----------------------------------------------------------------
// Mp4Context::find_ancestor
//
const Box *
Mp4Context::find_ancestor(const char * fourcc) const
{
  for (std::list<const Box *>::const_reverse_iterator
         i = ancestors_.rbegin(); i != ancestors_.rend(); ++i)
  {
    const Box * box = *i;
    if (box->type_.same_as(fourcc))
    {
      return box;
    }
  }

  return NULL;
}

//----------------------------------------------------------------
// Mp4Context::parse
//
TBoxPtr
Mp4Context::parse(IBitstream & bin,
                  std::size_t end_pos,
                  BoxFactory * box_factory)
{
  const std::size_t box_pos = bin.position();
  TBoxPtr box(new Box());
  box->load(*this, bin);

  // YAE_BREAKPOINT_IF(box->type_.same_as("dref"));

  const std::size_t box_end = box_pos + box->size_ * 8;
  YAE_ASSERT(!end_pos || box_end <= end_pos);

  try
  {
    // temporarily override bitstream end position:
    yae::SetEnd override_bin_end(bin, box_end);

    // instantiate appropriate box type:
    TBoxConstructor box_constructor = NULL;
    if (parse_mdat_data_ && box->type_.same_as("mdat"))
    {
      box_constructor = (TBoxConstructor)(create<Container>::please);
    }
    else if (this->is_ancestor_type("tref"))
    {
      // children of tref are always TrackReferenceTypeBox:
      box_constructor =
        (TBoxConstructor)(create<TrackReferenceTypeBox>::please);
    }
    else if (box_factory)
    {
      box_constructor = box_factory->get(box->type_);
    }
    else
    {
      if (box->type_.same_as("tmcd"))
      {
        if (this->is_ancestor_type("gmhd"))
        {
          box_constructor = (TBoxConstructor)(create<Container>::please);
        }
        else if (this->is_ancestor_type("stsd"))
        {
          box_constructor = (TBoxConstructor)
            (create<yae::qtff::TimecodeSampleDescAtom>::please);
        }
      }

      if (!box_constructor)
      {
        box_constructor = Mp4BoxFactory::singleton().get(box->type_);
      }

      if (!box_constructor)
      {
        box_constructor = QuickTimeAtomFactory::singleton().get(box->type_);
      }
    }

    if (box_constructor)
    {
      box.reset(box_constructor(box->type_.str_));
      bin.seek(box_pos);

      // temporarily override bitstream end position:
      yae::SetEnd override_bitstream_end_position(bin, box_end);
      box->load(*this, bin);
    }

    if (bin.position() != box_end)
    {
      yae::Data unparsed;
      bin.skip_until_byte_aligned();
      std::size_t unparsed_bytes = (box_end - bin.position()) / 8;
      if (unparsed_bytes)
      {
        std::size_t bytes_to_load =
          (unparsed_bytes <= 16) ? unparsed_bytes : 13;
        unparsed = bin.read_bytes(bytes_to_load);
      }

      yae_wlog("possibly %s box type: '%4s', "
               "file pos: %" PRIu64 ", size: %" PRIu64 "%s",
               box_constructor ? "mis-parsed" : "unsupported",
               box->type_.str_,
               file_position_ + box_pos / 8,
               box->size_,
               unparsed.empty() ? "" :
               yae::strfmt(", unparsed: %s%s",
                           unparsed.to_hex().c_str(),
                           unparsed_bytes == unparsed.size() ?
                           "" : "...").c_str());
    }
  }
  catch (const std::exception & e)
  {
    yae_elog("error loading box type: '%4s', "
             "file pos: %" PRIu64 ", size: %" PRIu64 ", exception: %s",
             box->type_.str_,
             file_position_ + box_pos / 8,
             box->size_,
             e.what());
  }
  catch (...)
  {
    yae_elog("error loading box type: '%4s', "
             "file pos: %" PRIu64 ", size: %" PRIu64 ", unexpected exception",
             box->type_.str_,
             file_position_ + box_pos / 8,
             box->size_);
  }

  // keep track of the file type being parsed:
  if (box->type_.same_as("ftyp"))
  {
    ftyp_ = boost::dynamic_pointer_cast<FileTypeBox>(box);
  }
  else if (box->type_.same_as("styp"))
  {
    styp_ = boost::dynamic_pointer_cast<FileTypeBox>(box);
  }

  // skip to the next box:
  bin.seek(box_end);

  return box;
}

//----------------------------------------------------------------
// Mp4Context::parse_file
//
bool
Mp4Context::parse_file(const std::string & src_path,
                       ParserCallbackInterface * cb)
{
  yae::TOpenFile src(src_path, "rb");
  if (!src.is_open())
  {
    yae_elog("failed to open file %s", src_path.c_str());
    return false;
  }

  while (!src.is_eof())
  {
    uint64_t box_start = src.ftell64();
    file_position_ = box_start;

    uint64_t box_size;
    FourCC box_type;
    if (!yae::read_mp4_box_size(src.file_, box_size, box_type))
    {
      if (box_start == src.get_filesize())
      {
        break;
      }

      yae_elog("failed to read mp4 box size "
               "from %s at file position %" PRIu64 "",
               src_path.c_str(),
               box_start);
      return false;
    }

    // where the next box will start:
    uint64_t box_end = box_start + box_size;

    yae::mp4::TBoxPtr box;
    if (box_type.same_as("mdat") && !load_mdat_data_)
    {
      // skip it:
      box.reset(new mp4::Box());
      box->size_ = box_size;
      box->type_ = box_type;
    }
    else
    {
      // FIXME: implement FileBitstream so that we don't have to load
      // the entire box in memory at once:

      yae::Data data(box_size);
      if (src.load(data) != box_size)
      {
        yae_elog("failed to load %" PRIu64 " bytes "
                 "from %s at file position %" PRIu64 "",
                 box_size,
                 src_path.c_str(),
                 box_start);
        return false;
      }

      yae::Bitstream bin(data);
      box = this->parse(bin, box_size * 8);
      YAE_ASSERT(box);
    }

    if (!box)
    {
      yae_elog("failed to parse %s at file position %" PRIu64 "",
               src_path.c_str(),
               box_start);
      return false;
    }

    if (cb && !cb->observe(*this, box))
    {
      break;
    }

    if (src.fseek64(box_end, SEEK_SET) != 0)
    {
      yae_elog("failed to seek %s to file position %" PRIu64 "",
               src_path.c_str(),
               box_end);
      return false;
    }
  }

  return true;
}

//----------------------------------------------------------------
// TrackInfo
//
struct TrackInfo
{
  TrackInfo():
    trex_(NULL),
    tkhd_(NULL),
    mdhd_(NULL),
    hdlr_(NULL),
    stsd_(NULL),
    stsz_(NULL),
    stsc_(NULL),
    stts_(NULL),
    stco_(NULL)
  {}

  const TrackExtendsBox * trex_;
  const TrackHeaderBox * tkhd_;
  const MediaHeaderBox * mdhd_;
  const HandlerBox * hdlr_;
  const ContainerList32 * stsd_;
  const SampleSizeBox * stsz_;
  const SampleToChunkBox * stsc_;
  const TimeToSampleBox * stts_;
  const ChunkOffsetBox * stco_;
};

//----------------------------------------------------------------
// yae::get_timeline
//
void
yae::get_timeline(const TBoxPtrVec & boxes, yae::Timeline & timeline)
{
  std::map<int, TrackInfo> tracks;

  const Container * moov = NULL;
  const Container * moof = NULL;
  const MovieHeaderBox * mvhd = NULL;

  const std::size_t num_boxes = boxes.size();
  for (std::size_t i = 0; i < num_boxes; ++i)
  {
    const Box * top_level_box = boxes[i].get();
    if (top_level_box->type_.same_as("moov"))
    {
      YAE_ASSERT(!moov);
      moov = dynamic_cast<const Container *>(top_level_box);
      YAE_ASSERT(moov);

      const TBoxPtrVec & children = moov->children_;
      for (std::size_t j = 0, n = children.size(); j < n; ++j)
      {
        const Box * child = children[j].get();
        if (child->type_.same_as("mvhd"))
        {
          mvhd = dynamic_cast<const MovieHeaderBox *>(child);
        }
        else if (child->type_.same_as("mvex"))
        {
          const TrackExtendsBox * trex =
            child->find<TrackExtendsBox>("trex");
          if (!trex)
          {
            continue;
          }

          TrackInfo & track = tracks[trex->track_ID_];
          track.trex_ = trex;
        }
        else if (child->type_.same_as("trak"))
        {
          const TrackHeaderBox * tkhd =
            child->find<TrackHeaderBox>("tkhd");
          if (!tkhd)
          {
            continue;
          }

          TrackInfo & track = tracks[tkhd->track_ID_];
          track.tkhd_ = tkhd;

          const Container * mdia = child->find<Container>("mdia");
          if (!mdia)
          {
            continue;
          }

          track.mdhd_ = mdia->find<MediaHeaderBox>("mdhd");
          track.hdlr_ = mdia->find<HandlerBox>("hdlr");

          const Container * minf = mdia->find<Container>("minf");
          if (!minf)
          {
            continue;
          }

          const Container * stbl = minf->find<Container>("stbl");
          if (!stbl)
          {
            continue;
          }

          track.stsd_ = stbl->find<ContainerList32>("stsd");
          track.stsz_ = stbl->find<SampleSizeBox>("stsz");
          track.stsc_ = stbl->find<SampleToChunkBox>("stsc");
          track.stts_ = stbl->find<TimeToSampleBox>("stts");
          track.stco_ = stbl->find<ChunkOffsetBox>("stco");
        }
      }
    }
    else if (top_level_box->type_.same_as("moof"))
    {
      moof = dynamic_cast<const Container *>(top_level_box);
      YAE_ASSERT(moof);

      const TBoxPtrVec & children = moof->children_;
      for (std::size_t j = 0, n = children.size(); j < n; ++j)
      {
        const Box * child = children[j].get();

        if (child->type_.same_as("traf"))
        {
          const Container * traf = dynamic_cast<const Container *>(child);
          YAE_ASSERT(traf);
          if (!traf)
          {
            continue;
          }

          const TrackFragmentHeaderBox * tfhd =
            traf->find<TrackFragmentHeaderBox>("tfhd");
          YAE_ASSERT(tfhd);
          if (!tfhd)
          {
            continue;
          }

          const TrackInfo & track = tracks[tfhd->track_ID_];
          YAE_ASSERT(track.hdlr_);
          if (!track.hdlr_)
          {
            continue;
          }

          // shortcuts:
          const std::string track_id =
            yae::strfmt("%s_%02i",
                        track.hdlr_->handler_type_.str_,
                        tfhd->track_ID_);

          const TrackFragmentBaseMediaDecodeTimeBox * tfdt =
            traf->find<TrackFragmentBaseMediaDecodeTimeBox>("tfdt");
          YAE_ASSERT(tfdt);
          if (!tfdt)
          {
            continue;
          }

          const TrackRunBox * trun = traf->find<TrackRunBox>("trun");
          YAE_ASSERT(trun);
          if (!trun)
          {
            continue;
          }

          // shortcuts:
          const std::vector<uint32_t> & trun_sdur =
            trun->sample_duration_;

          const std::vector<int64_t> & trun_comp =
            trun->sample_composition_time_offset_;

          const std::vector<uint32_t> & trun_size =
            trun->sample_size_;

          std::size_t num_samples = trun->sample_count_;
          if (!trun_sdur.empty())
          {
            YAE_ASSERT(num_samples == trun_sdur.size());
            num_samples = std::min(num_samples, trun_sdur.size());
          }

          if (!trun_comp.empty())
          {
            YAE_ASSERT(num_samples == trun_comp.size());
            num_samples = std::min(num_samples, trun_comp.size());
          }

          if (!trun_size.empty())
          {
            YAE_ASSERT(num_samples == trun_size.size());
            num_samples = std::min(num_samples, trun_size.size());
          }

          // shortcuts:
          int64_t pts_0 = tfdt->baseMediaDecodeTime_;
          int64_t timebase = track.mdhd_->timescale_;
          int64_t default_ctts = 0;

          uint32_t default_dur = 0;
          if (tfhd->default_sample_duration_is_present())
          {
            default_dur = tfhd->default_sample_duration_;
          }
          else if (track.trex_)
          {
            default_dur = track.trex_->default_sample_duration_;
          }

          uint32_t default_size = 0;
          if (tfhd->default_sample_size_is_present())
          {
            default_size = tfhd->default_sample_size_;
          }
          else if (track.trex_)
          {
            default_size = track.trex_->default_sample_size_;
          }

          // calculate DTS offset:
          uint64_t dts_offset = 0;
          {
            uint64_t sum_dur = 0;
            for (std::size_t k = 0; k < num_samples; ++k)
            {
              uint32_t dur = yae::get(trun_sdur, k, default_dur);

              // NOTE: ctts[k] = (pts[k] - dts[k]) - (pts[0] - dts[0])
              int64_t ctts = yae::get(trun_comp, k, default_ctts);

              int64_t pts = pts_0 + sum_dur + ctts;
              sum_dur += dur;

              int64_t dts = (pts - ctts) - dts_offset;
              if (pts < dts)
              {
                dts_offset += (dts - pts);
              }
            }
          }

          // reconstruct timeline:
          {
            uint64_t sum_dur = 0;
            for (std::size_t k = 0; k < num_samples; ++k)
            {
              uint32_t sample_size = yae::get(trun_size, k, default_size);
              uint32_t dur = yae::get(trun_sdur, k, default_dur);

              // NOTE: ctts[k] = (pts[k] - dts[k]) - (pts[0] - dts[0])
              int64_t ctts = yae::get(trun_comp, k, default_ctts);
              int64_t pts = pts_0 + sum_dur + ctts;
              sum_dur += dur;

              int64_t dts = (pts - ctts) - dts_offset;
              timeline.add_packet(track_id,
                                  k == 0, // keyframe ... use sample_flags
                                  sample_size,
                                  TTime(dts, timebase),
                                  TTime(pts, timebase),
                                  TTime(dur, timebase),
                                  TTime(dur, timebase).sec() + 1e-6);
            }
          }
        }
      }
    }
  }
}
