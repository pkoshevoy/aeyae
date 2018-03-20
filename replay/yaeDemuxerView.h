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
#include <QFont>

// aeyae:
#include "yae/ffmpeg/yae_demuxer.h"
#include "yae/thread/yae_task_runner.h"
#include "yae/utils/yae_lru_cache.h"
#include "yae/video/yae_video.h"

// local:
#include "yaeInputArea.h"
#include "yaeItemView.h"
#include "yaeScrollview.h"


namespace yae
{

  // forward declarations:
  class Texture;
  class Text;
  class RemuxView;
  struct RemuxViewStyle;


  //----------------------------------------------------------------
  // Media
  //
  struct YAE_API Media
  {
    Media(const TDemuxerInterfacePtr & demuxer = TDemuxerInterfacePtr(),
          const DemuxerSummary & summary = DemuxerSummary()):
      demuxer_(demuxer),
      summary_(summary)
    {}

    inline bool operator < (const Media & other) const
    { return demuxer_ < other.demuxer_; }

    TDemuxerInterfacePtr demuxer_;
    DemuxerSummary summary_;
  };

  //----------------------------------------------------------------
  // TMediaPtr
  //
  typedef boost::shared_ptr<Media> TMediaPtr;

  //----------------------------------------------------------------
  // Clip
  //
  struct YAE_API Clip
  {
    Clip(const TMediaPtr & media = TMediaPtr(),
         const std::string & track = std::string(),
         const Timespan & keep = Timespan()):
      media_(media),
      track_(track),
      keep_(keep)
    {}

    TMediaPtr media_;
    std::string track_;
    Timespan keep_;
  };

  //----------------------------------------------------------------
  // RemuxModel
  //
  struct YAE_API RemuxModel
  {
    RemuxModel():
      current_(0)
    {}

    // media, indexed by source path:
    std::map<std::string, TMediaPtr> media_;

    // composition of the remuxed output:
    std::vector<Clip> clips_;

    // index of currently selected clip:
    std::size_t current_;
  };

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
  typedef boost::shared_ptr<TLayout> TLayoutPtr;


  //----------------------------------------------------------------
  // RemuxViewStyle
  //
  struct YAE_API RemuxViewStyle : public ItemViewStyle
  {
    RemuxViewStyle(const char * id, const ItemView & view);

    TLayoutPtr layout_root_;
    TLayoutPtr layout_clips_;
    TLayoutPtr layout_gops_;

    // shared common properties:
    ItemRef row_height_;
  };

  //----------------------------------------------------------------
  // Gop
  //
  struct YAE_API Gop
  {
    Gop(Media * media = NULL,
        const std::string & track_id = std::string(),
        std::size_t gop_start_packet_index = 0,
        std::size_t gop_end_packet_index = 0):
      media_(media),
      track_(track_id),
      i0_(gop_start_packet_index),
      i1_(gop_end_packet_index)
    {}

    inline bool operator < (const Gop & other) const
    {
      return (media_ < other.media_ ||
              (media_ == other.media_ &&
               (track_ < other.track_ ||
                (track_ == other.track_ &&
                 (i0_ < other.i0_ ||
                  (i0_ == other.i0_ &&
                   i1_ < other.i1_))))));
    }

    Media * media_;
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
    os << gop.media_ << ' ' << gop.track_ << ' ' << gop.i0_ << ' ' << gop.i1_;
    return os;
  }

  //----------------------------------------------------------------
  // TVideoFrames
  //
  typedef std::vector<TVideoFramePtr> TVideoFrames;

  //----------------------------------------------------------------
  // TVideoFramesPtr
  //
  typedef boost::shared_ptr<TVideoFrames> TVideoFramesPtr;

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

    ItemRef opacity_;

  protected:
    // intentionally disabled:
    VideoFrameItem(const VideoFrameItem &);
    VideoFrameItem & operator = (const VideoFrameItem &);

    std::size_t frame_;
    mutable boost::shared_ptr<TLegacyCanvas> renderer_;
  };

  //----------------------------------------------------------------
  // GopItem
  //
  struct YAE_API GopItem : public Item
  {
    GopItem(const char * id, const Gop & gop);

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

    // accessor:
    inline const Gop & gop() const
    { return gop_; }

  protected:
    // called asynchronously upon completion of GOP decoding task:
    static void
    cb(const boost::shared_ptr<AsyncTaskQueue::Task> & task, void * ctx);

    // intentionally disabled:
    GopItem(const GopItem &);
    GopItem & operator = (const GopItem &);

    Gop gop_;
    const Canvas::ILayer * layer_;
    mutable boost::mutex mutex_;
    mutable boost::shared_ptr<AsyncTaskQueue::Task> async_;
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
    RemuxView();

    // data source:
    void setModel(RemuxModel * model);

    inline RemuxModel * model() const
    { return model_; }

    // virtual:
    const ItemViewStyle * style() const
    { return &style_; }

    // virtual:
    bool processMouseEvent(Canvas * canvas, QMouseEvent * event);

    // virtual:
    bool resizeTo(const Canvas * canvas);

  public slots:
    // adjust scrollview position to ensure a given item is visible:
    // void ensureVisible(const QModelIndex & itemIndex);

    // shortcut:
    // void ensureCurrentItemIsVisible();
    void layoutChanged();
    void dataChanged();

  protected:
    RemuxViewStyle style_;
    RemuxModel * model_;
  };

}


#endif // YAE_DEMUXER_VIEW_H_
