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
  class RemuxView;
  struct RemuxViewStyle;
  class Texture;
  class Text;

  //----------------------------------------------------------------
  // Clip
  //
  struct YAE_API Clip
  {
    TDemuxerInterfacePtr src_;
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
    TDemuxerInterfacePtr src_;
    DemuxerSummary summary_;

    // composition of the remuxed output:
    std::vector<TClipPtr> remux_;
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
  struct YAE_API RemuxViewStyle : public Item
  {
    RemuxViewStyle(const char * id, const ItemView & view);

    const ItemView & view_;

    TLayoutPtr layout_root_;
    TLayoutPtr layout_clips_;
    TLayoutPtr layout_clip_;
    TLayoutPtr layout_gops_;
    TLayoutPtr layout_gop_;

    // font palette:
    QFont font_;
    QFont font_small_;
    QFont font_large_;
    QFont font_fixed_;

    // shared common properties:
    ItemRef title_height_;
    ItemRef frame_width_;
    ItemRef row_height_;
    ItemRef font_size_;

    // color palette:
    ColorRef bg_;
    ColorRef fg_;

    ColorRef border_;
    ColorRef cursor_;
    ColorRef scrollbar_;
    ColorRef separator_;
    ColorRef underline_;

    ColorRef bg_controls_;
    ColorRef fg_controls_;

    ColorRef bg_focus_;
    ColorRef fg_focus_;

    ColorRef bg_edit_selected_;
    ColorRef fg_edit_selected_;

    ColorRef bg_timecode_;
    ColorRef fg_timecode_;

    ColorRef timeline_excluded_;
    ColorRef timeline_included_;
  };


  //----------------------------------------------------------------
  // ColorAttr
  //
  struct YAE_API ColorAttr : public TColorExpr
  {
    ColorAttr(const Item & item,
              const char * attr,
              double alphaScale = 1.0,
              double alphaTranslate = 0.0):
      item_(item),
      attr_(attr),
      alphaScale_(alphaScale),
      alphaTranslate_(alphaTranslate)
    {}

    // virtual:
    void evaluate(Color & result) const
    {
      // it's a convention:
      const Item & style = item_["style"];

      if (style.getAttr<Color>(attr_, result))
      {
        result = result.a_scaled(alphaScale_, alphaTranslate_);
      }
    }

    const Item & item_;
    std::string attr_;
    double alphaScale_;
    double alphaTranslate_;
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
    RemuxModel * model_;
  };

}


#endif // YAE_DEMUXER_VIEW_H_
