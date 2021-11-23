// -*- Mode: c++; tab-width: 8; c-basic-offset: 2; indent-tabs-mode: nil -*-
// NOTE: the first line of this file sets up source code indentation rules
// for Emacs; it is also a hint to anyone modifying this file.

// Created      : Sat Jan 27 18:06:40 MST 2018
// Copyright    : Pavel Koshevoy
// License      : MIT -- http://www.opensource.org/licenses/mit-license.php

#ifndef YAE_REMUX_VIEW_H_
#define YAE_REMUX_VIEW_H_

// standard:
#include <vector>

// Qt library:
#include <QAction>
#include <QFont>
#include <QSignalMapper>

// aeyae:
#include "yae/api/yae_shared_ptr.h"
#include "yae/ffmpeg/yae_demuxer.h"
#include "yae/ffmpeg/yae_demuxer_reader.h"
#include "yae/ffmpeg/yae_remux.h"
#include "yae/thread/yae_task_runner.h"
#include "yae/utils/yae_lru_cache.h"
#include "yae/video/yae_video.h"

// yaeui:
#include "yaeInputArea.h"
#include "yaeItemView.h"
#include "yaeItemViewStyle.h"
#include "yaePlayerItem.h"
#include "yaeScrollview.h"
#include "yaeTimelineItem.h"


namespace yae
{

  // forward declarations:
  class Texture;
  class Text;
  class RemuxView;
  struct RemuxViewStyle;

  //----------------------------------------------------------------
  // TLayout
  //
  typedef ILayout<RemuxModel, RemuxView, RemuxViewStyle> TLayout;

  //----------------------------------------------------------------
  // TLayoutPtr
  //
  typedef yae::shared_ptr<TLayout> TLayoutPtr;


  //----------------------------------------------------------------
  // RemuxViewStyle
  //
  struct YAEUI_API RemuxViewStyle : public ItemViewStyle
  {
    RemuxViewStyle(const char * id, const RemuxView & view);

    TLayoutPtr layout_;
  };

  //----------------------------------------------------------------
  // Gop
  //
  struct YAEUI_API Gop
  {
    Gop(const TDemuxerInterfacePtr & demuxer = TDemuxerInterfacePtr(),
        const std::string & track_id = std::string(),
        std::size_t gop_start_packet_index = 0,
        std::size_t gop_end_packet_index = 0):
      demuxer_(demuxer),
      track_(track_id),
      i0_(gop_start_packet_index),
      i1_(gop_end_packet_index)
    {}

    inline operator std::string() const
    {
      std::ostringstream oss;
      oss << demuxer_.get() << "," << track_ << "[" << i0_ << "," << i1_ << ")";
      return oss.str();
    }

    TDemuxerInterfacePtr demuxer_;
    std::string track_;
    std::size_t i0_;
    std::size_t i1_;
  };

  //----------------------------------------------------------------
  // operator
  //
  inline std::ostream &
  operator << (std::ostream & os, const Gop & gop)
  {
    os << gop.demuxer_ << ' ' << gop.track_ << ' ' << gop.i0_ << ' ' << gop.i1_;
    return os;
  }

  //----------------------------------------------------------------
  // TVideoFrames
  //
  typedef std::vector<TVideoFramePtr> TVideoFrames;

  //----------------------------------------------------------------
  // TVideoFramesPtr
  //
  typedef yae::shared_ptr<TVideoFrames> TVideoFramesPtr;

  //----------------------------------------------------------------
  // TGopCache
  //
  typedef LRUCache<std::string, TVideoFramesPtr> TGopCache;

  //----------------------------------------------------------------
  // VideoFrameItem
  //
  struct YAEUI_API VideoFrameItem : public Item
  {
    VideoFrameItem(const char * id, std::size_t frame);

    // virtual:
    void uncache();

    // virtual:
    void paintContent() const;
    void unpaintContent() const;

    // accessors:
    inline std::size_t frameIndex() const
    { return frame_; }

    // helper:
    TVideoFramePtr videoFrame() const;

    ItemRef opacity_;

  protected:
    // intentionally disabled:
    VideoFrameItem(const VideoFrameItem &);
    VideoFrameItem & operator = (const VideoFrameItem &);

    std::size_t frame_;
    mutable yae::shared_ptr<TLegacyCanvas> renderer_;
  };

  //----------------------------------------------------------------
  // GopItem
  //
  struct YAEUI_API GopItem : public Item
  {
    GopItem(const char * id,
            const Gop & gop,
            TPixelFormatId pixelFormatOverride = kInvalidPixelFormat);

    ~GopItem();

    // repaint requests, etc...
    void setContext(const Canvas::ILayer & view);

    // virtual:
    void paintContent() const;
    void unpaintContent() const;

    // accessor, used by the VideoFrameItem:
    inline TGopCache::TRefPtr cached() const
    {
      boost::lock_guard<boost::mutex> lock(mutex_);
      return cached_;
    }

    // accessor, used by the VideoFrameItem:
    inline const Canvas::ILayer * getLayer() const
    { return layer_; }

    // accessors:
    inline const Gop & gop() const
    { return gop_; }

    inline TPixelFormatId pixelFormat() const
    { return pixelFormat_; }

  protected:
    // called asynchronously upon completion of GOP decoding task:
    static void
    cb(const yae::shared_ptr<AsyncTaskQueue::Task> & task, void * ctx);

    // intentionally disabled:
    GopItem(const GopItem &);
    GopItem & operator = (const GopItem &);

    Gop gop_;
    TPixelFormatId pixelFormat_;
    const Canvas::ILayer * layer_;
    mutable boost::mutex mutex_;
    mutable yae::shared_ptr<AsyncTaskQueue::Task> async_;
    mutable TGopCache::TRefPtr cached_;

    // keep track if decoding fails to avoid trying and failing again:
    bool failed_;
  };


  //----------------------------------------------------------------
  // PlayheadPosition
  //
  struct SerialDemuxerPosition
  {
    SerialDemuxerPosition():
      prog_id_(0),
      pts_(0, 0),
      src_index_(0),
      src_dts_(0, 0)
    {}

    int prog_id_;
    TTime pts_;
    yae::weak_ptr<DemuxerInterface> src_;
    std::size_t src_index_;
    TTime src_dts_;
  };


  //----------------------------------------------------------------
  // RemuxView
  //
  class YAEUI_API RemuxView : public ItemView
  {
    Q_OBJECT;

  public:

    enum ViewMode
    {
      kSourceMode = 0,
      kLayoutMode = 1,
      kPreviewMode = 2,
      kPlayerMode = 3,
      kExportMode = 4,
    };

    RemuxView(const char * name);

    // virtual:
    void setContext(const yae::shared_ptr<IOpenGLContext> & context);

    // data source:
    void setModel(RemuxModel * model);

    inline RemuxModel * model() const
    { return model_; }

    // virtual:
    RemuxViewStyle * style() const
    { return style_; }

    // virtual:
    bool processKeyEvent(Canvas * canvas, QKeyEvent * event);
    bool processMouseEvent(Canvas * canvas, QMouseEvent * event);

    // virtual:
    bool processMouseTracking(const TVec2D & mousePt);

    // virtual:
    bool processRightClick();

    // helpers:
    void append_source(const std::string & name,
                       const TDemuxerInterfacePtr & src);

    void append_clip(const TClipPtr & clip);

  protected:
    void add_clip_row(const TClipPtr & clip);

  public:
    void remove_clip(std::size_t index);
    void repeat_clip();

    void relayout_gops(const TDemuxerInterfacePtr & src);

    // helper:
    inline TClipPtr selected_clip() const
    {
      return (selected_ < model_->clips_.size() ?
              model_->clips_[selected_] :
              TClipPtr());
    }

    // helper:
    inline TClipPtr current_clip() const
    {
      return (view_mode_ == kLayoutMode) ? selected_clip() : output_clip();
    }

    // lookup GOPs container item for a given clip:
    inline ItemPtr get_gops_container(const TClipPtr & clip) const
    { return yae::get(clips_, clip, ItemPtr()); }

    // accessor:
    inline ViewMode view_mode() const
    { return view_mode_; }

  signals:
    void remux();
    void toggle_fullscreen();

  public slots:
    void layoutChanged();
    void dataChanged();

    void timecode_changed_t0(int i);
    void timecode_changed_t1(int i);

    void set_in_point();
    void set_out_point();

    // NOTE: switching away from layout mode creates a serial demuxer
    //       from current source clips:
    void set_view_mode(RemuxView::ViewMode mode);
    void enable_focus_group();

    bool is_playback_paused();
    void toggle_playback();

  protected:
    TClipPtr output_clip() const;

    void capture_playhead_position();
    void maybe_layout_gops();
    void maybe_update_player();

    RemuxModel * model_;
    ViewMode view_mode_;
    mutable TClipPtr output_clip_;
    mutable TSerialDemuxerPtr serial_demuxer_;
    mutable std::string model_json_str_;

    DemuxerReaderPtr reader_;

    // timeline playhead position, resilient to re-ordering of clips:
    SerialDemuxerPosition playhead_;

  public:
    yae::shared_ptr<RemuxViewStyle, Item> style_;
    yae::shared_ptr<PlayerItem, Item> player_;
    yae::shared_ptr<TimelineItem, Item> timeline_;

    // index of currently selected clip:
    std::size_t selected_;

    QSignalMapper t0_;
    QSignalMapper t1_;

    QAction actionSetInPoint_;
    QAction actionSetOutPoint_;

    // LUT of top level clip container items:
    std::map<TClipPtr, ItemPtr> clips_;

    // share the same GOP layout between clips of the same source:
    std::map<TDemuxerInterfacePtr, ItemPtr> gops_;

    typedef std::map<std::size_t, std::size_t> TRowLut;
    std::map<TDemuxerInterfacePtr, TRowLut> gops_row_lut_;

    // source items, indexed by source file path:
    std::map<std::string, ItemPtr> source_item_;

    // use the same scale for all dts/pts plots:
    yae::shared_ptr<Segment> time_range_;

    // use the same scale for all packet size plots:
    yae::shared_ptr<Segment> size_range_;
  };

}


#endif // YAE_DEMUXER_VIEW_H_
