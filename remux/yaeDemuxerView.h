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
#include "yae/thread/yae_task_runner.h"
#include "yae/utils/yae_lru_cache.h"
#include "yae/video/yae_video.h"

// local:
#include "yaeInputArea.h"
#include "yaeItemView.h"
#include "yaeRemux.h"
#include "yaeScrollview.h"


namespace yae
{

  // forward declarations:
  class Texture;
  class Text;
  class RemuxView;
  struct RemuxViewStyle;


  //----------------------------------------------------------------
  // ILayout
  //
  template <typename TModel, typename TView, typename TViewStyle>
  struct YAE_API ILayout
  {
    typedef TModel model_type;
    typedef TView view_type;
    typedef TViewStyle style_type;

    virtual ~ILayout() {}

    virtual void layout(TModel & model,
                        TView & view,
                        const TViewStyle & style,
                        Item & item,
                        void * context = NULL) = 0;

    // shortcut:
    inline void layout(Item & item,
                       TView & view,
                       TModel & model,
                       const TViewStyle & style)
    {
      this->layout(model, view, style, item, NULL);
    }
  };

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
  struct YAE_API RemuxViewStyle : public ItemViewStyle
  {
    RemuxViewStyle(const char * id, const RemuxView & view);

    TLayoutPtr layout_;
  };

  //----------------------------------------------------------------
  // Gop
  //
  struct YAE_API Gop
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

    inline bool operator < (const Gop & other) const
    {
      return (demuxer_ < other.demuxer_ ||
              (demuxer_ == other.demuxer_ &&
               (track_ < other.track_ ||
                (track_ == other.track_ &&
                 (i0_ < other.i0_ ||
                  (i0_ == other.i0_ &&
                   i1_ < other.i1_))))));
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
  typedef LRUCache<Gop, TVideoFramesPtr> TGopCache;

  //----------------------------------------------------------------
  // VideoFrameItem
  //
  struct YAE_API VideoFrameItem : public Item
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
  struct YAE_API GopItem : public Item
  {
    GopItem(const char * id,
            const Gop & gop,
            TPixelFormatId pixelFormatOverride = kInvalidPixelFormat);

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
  // RemuxView
  //
  class YAE_API RemuxView : public ItemView
  {
    Q_OBJECT;

  public:

    enum ViewMode
    {
      kLayoutMode = 0,
      kPreviewMode = 1
    };

    RemuxView();

    // data source:
    void setModel(RemuxModel * model);

    inline RemuxModel * model() const
    { return model_; }

    // virtual:
    const ItemViewStyle * style() const
    { return &style_; }

    // virtual:
    bool processKeyEvent(Canvas * canvas, QKeyEvent * event);
    bool processMouseEvent(Canvas * canvas, QMouseEvent * event);

    // virtual:
    bool processMouseTracking(const TVec2D & mousePt);

    // virtual:
    bool processRightClick();

    // helpers:
    void append_clip(const TClipPtr & clip);
    void remove_clip(std::size_t index);
    void repeat_clip();

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
      return (view_mode_ == kPreviewMode) ? output_clip() : selected_clip();
    }

    // accessor:
    inline ViewMode view_mode() const
    { return view_mode_; }

  signals:
    void remux();

  public slots:
    void layoutChanged();
    void dataChanged();

    void timecode_changed_t0(int i);
    void timecode_changed_t1(int i);

    void set_in_point();
    void set_out_point();

    // NOTE: switching to preview mode creates a serial demuxer
    //       from current source clips:
    void set_view_mode(RemuxView::ViewMode mode);

    void emit_remux();

  protected:
    TClipPtr output_clip() const;

    RemuxModel * model_;
    ViewMode view_mode_;
    mutable TClipPtr output_clip_;
    mutable TSerialDemuxerPtr serial_demuxer_;
    mutable std::string model_json_str_;

  public:
    RemuxViewStyle style_;

    // index of currently selected clip:
    std::size_t selected_;

    QSignalMapper t0_;
    QSignalMapper t1_;

    QAction actionSetInPoint_;
    QAction actionSetOutPoint_;

    // share the same GOP layout between clips of the same source:
    std::map<TDemuxerInterfacePtr, ItemPtr> gops_;

    typedef std::map<std::size_t, std::size_t> TRowLut;
    std::map<TDemuxerInterfacePtr, TRowLut> gops_row_lut_;
  };

}


#endif // YAE_DEMUXER_VIEW_H_
