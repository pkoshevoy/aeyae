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
  // Clip
  //
  struct YAE_API Clip
  {
    TDemuxerInterfacePtr src_;
    DemuxerSummary summary_;
    std::string track_;
    Timespan keep_;
  };

  //----------------------------------------------------------------
  // TClipPtr
  //
  typedef boost::shared_ptr<Clip> TClipPtr;

  //----------------------------------------------------------------
  // RemuxModel
  //
  struct YAE_API RemuxModel
  {
    RemuxModel():
      current_(0)
    {}

    // composition of the remuxed output:
    std::vector<TClipPtr> remux_;

    // index of currently selected clip:
    std::size_t current_;
  };

  //----------------------------------------------------------------
  // ILayout
  //
  template <typename TModel,
            typename TModelIndex,
            typename TView,
            typename TViewStyle>
  struct YAE_API ILayout
  {
    typedef TModel model_type;
    typedef TModelIndex index_type;
    typedef TView view_type;
    typedef TViewStyle style_type;

    virtual ~ILayout() {}

    virtual void layout(TModel & model, const TModelIndex & index,
                        TView & view, const TViewStyle & style,
                        Item & item) = 0;

    // shortcut:
    inline void layout(Item & item,
                       TView & view,
                       TModel & model,
                       const TViewStyle & style)
    {
      this->layout(model, TModelIndex(), view, style, item);
    }
  };

  //----------------------------------------------------------------
  // TLayout
  //
  typedef ILayout<RemuxModel, std::size_t, RemuxView, RemuxViewStyle> TLayout;

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
    TLayoutPtr layout_clip_;
    TLayoutPtr layout_gops_;
    TLayoutPtr layout_gop_;

    // shared common properties:
    ItemRef row_height_;
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
