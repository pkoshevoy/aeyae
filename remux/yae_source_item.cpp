// -*- Mode: c++; tab-width: 8; c-basic-offset: 2; indent-tabs-mode: nil -*-
// NOTE: the first line of this file sets up source code indentation rules
// for Emacs; it is also a hint to anyone modifying this file.

// Created      : Sun Aug 12 12:18:04 MDT 2018
// Copyright    : Pavel Koshevoy
// License      : MIT -- http://www.opensource.org/licenses/mit-license.php

// standard:
#include <sstream>

// aeyae:
#include "yae/utils/yae_utils.h"

// local:
#include "yae_checkbox_item.h"
#include "yae_input_proxy_item.h"
#include "yae_source_item.h"
#include "yaeItemViewStyle.h"


namespace yae
{

  //----------------------------------------------------------------
  // SourceItem::SourceItem
  //
  SourceItem::SourceItem(const char * id,
                         ItemView & view,
                         const std::string & name,
                         const TDemuxerInterfacePtr & src):
    QObject(),
    Item(id),
    view_(view),
    name_(name),
    src_(src)
  {}

  //----------------------------------------------------------------
  // SourceItem::layout
  //
  void
  SourceItem::layout()
  {
    // shortcut:
    const DemuxerSummary & summary = src_->summary();
    const ItemViewStyle & style = *(view_.style());

    Item * prev_row = NULL;
    {
      Item & row = this->addNew<Item>("src_name_row");
      row.anchors_.left_ = ItemRef::reference(*this, kPropertyLeft);
      row.anchors_.top_ = ItemRef::reference(*this, kPropertyTop);
      row.height_ = ItemRef::reference(style.row_height_);
      row.margins_.set_left(ItemRef::reference(style.row_height_, 1));

      Text & text = row.addNew<Text>("text");
      text.anchors_.left_ = ItemRef::reference(row, kPropertyLeft);
      text.anchors_.vcenter_ = ItemRef::reference(row, kPropertyVCenter);
      text.fontSize_ = ItemRef::reference(style.row_height_, 0.3);
      text.text_ = TVarRef::constant(TVar(name_.c_str()));

      prev_row = &row;
    }

    int track_focus_index = ItemFocus::singleton().getGroupOffset("sources");
    for (std::map<int, Timeline>::const_iterator
           i = summary.timeline_.begin(); i != summary.timeline_.end(); ++i)
    {
      // shortcuts:
      const int prog_id = i->first;
      const Timeline & timeline = i->second;
      const TProgramInfo & program = yae::at(summary.programs_, prog_id);

      Item & prog = this->addNew<Item>(str("prog_", prog_id).c_str());
      {
        std::ostringstream oss;
        oss << "program " << prog_id;

        std::string prog_name = yae::get(program.metadata_, "service_name");
        if (!prog_name.empty())
        {
          oss << ", " << prog_name;
        }

        prog.anchors_.top_ = ItemRef::reference(*prev_row, kPropertyBottom);
        prog.anchors_.left_ = ItemRef::reference(*this, kPropertyLeft);
        prog.margins_.set_left(ItemRef::reference(style.row_height_, 2));

        Item & row = prog.addNew<Item>("name");
        row.anchors_.left_ = ItemRef::reference(prog, kPropertyLeft);
        row.anchors_.top_ = ItemRef::reference(prog, kPropertyTop);
        row.height_ = ItemRef::reference(style.row_height_);

        Text & text = row.addNew<Text>("text");
        text.anchors_.left_ = ItemRef::reference(row, kPropertyLeft);
        text.anchors_.vcenter_ = ItemRef::reference(row, kPropertyVCenter);
        text.fontSize_ = ItemRef::reference(style.row_height_, 0.3);
        text.text_ = TVarRef::constant(TVar(oss.str().c_str()));

        prev_row = &row;
      }

      for (std::map<std::string, Timeline::Track>::const_iterator
             j = timeline.tracks_.begin(); j != timeline.tracks_.end(); ++j)
      {
        // shortcuts:
        const std::string & track_id = j->first;
        const Timeline::Track & tt = j->second;
        TrackPtr track = yae::get(summary.decoders_, track_id);
        const char * track_name = track ? track->getName() : NULL;
        const char * track_lang = track ? track->getLang() : NULL;

        std::ostringstream oss;
        oss << track_id;

        const char * codec_name = track->getCodecName();
        if (codec_name)
        {
          oss << ", " << codec_name;
        }

        if (track_name)
        {
          oss << ", " << track_name;
        }

        if (track_lang)
        {
          oss << " (" << track_lang << ")";
        }

        VideoTrackPtr video =
          boost::dynamic_pointer_cast<VideoTrack, Track>(track);

        if (video)
        {
          VideoTraits traits;
          video->getTraits(traits);

          double par = (traits.pixelAspectRatio_ != 0.0 &&
                        traits.pixelAspectRatio_ != 1.0 ?
                        traits.pixelAspectRatio_ : 1.0);
          unsigned int w = (unsigned int)(0.5 + par * traits.visibleWidth_);
          oss << ", " << w << " x " << traits.visibleHeight_
              << ", " << traits.frameRate_ << " fps";
        }

        AudioTrackPtr audio =
          boost::dynamic_pointer_cast<AudioTrack, Track>(track);

        if (audio)
        {
          AudioTraits traits;
          audio->getTraits(traits);

          oss << ", " << traits.sampleRate_ << " Hz"
              << ", " << int(traits.channelLayout_) << " channels";
        }

        Item & row = prog.addNew<Item>(str("track_", track_id).c_str());
        row.anchors_.top_ = ItemRef::reference(*prev_row, kPropertyBottom);
        row.anchors_.left_ = ItemRef::reference(prog, kPropertyLeft);
        row.height_ = ItemRef::reference(style.row_height_, 1);
        row.margins_.set_left(ItemRef::reference(style.row_height_, 1));

        CheckboxItem & cbox = row.add(new CheckboxItem("cbox", view_));
        cbox.anchors_.left_ = ItemRef::reference(row, kPropertyLeft);
        cbox.anchors_.vcenter_ = ItemRef::reference(row, kPropertyVCenter);
        // cbox.height_ = ItemRef::reference(row.height_, 0.64);
        cbox.height_ = ItemRef::reference(row.height_, 0.75);
        cbox.width_ = cbox.height_;

        Text & text = row.addNew<Text>("text");
        text.anchors_.left_ = ItemRef::reference(cbox, kPropertyRight);
        text.margins_.set_left(ItemRef::reference(cbox, kPropertyWidth, 0.5));
        text.anchors_.vcenter_ = ItemRef::reference(row, kPropertyVCenter);
        text.fontSize_ = ItemRef::reference(style.row_height_, 0.3);
        text.text_ = TVarRef::constant(TVar(oss.str().c_str()));

        InputProxy & proxy = row.addNew<InputProxy>
          (str("proxy_", track_id).c_str());
        proxy.anchors_.left_ = ItemRef::reference(cbox, kPropertyLeft);
        proxy.anchors_.right_ = ItemRef::reference(text, kPropertyRight);
        proxy.anchors_.top_ = ItemRef::reference(row, kPropertyTop);
        proxy.height_ = ItemRef::reference(row, kPropertyHeight);

        proxy.ia_ = cbox.self_;
        ItemFocus::singleton().setFocusable(view_,
                                            proxy,
                                            "sources",
                                            track_focus_index);
        track_focus_index++;

        prev_row = &row;
      }
    }

    Item & spacer = addNew<Item>("spacer");
    spacer.anchors_.left_ = ItemRef::constant(0);
    spacer.anchors_.top_ = ItemRef::reference(*prev_row, kPropertyBottom);
    spacer.height_ = ItemRef::reference(style.row_height_);
    spacer.width_ = ItemRef::constant(0);
  }

}
