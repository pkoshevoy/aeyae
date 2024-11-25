// -*- Mode: c++; tab-width: 8; c-basic-offset: 2; indent-tabs-mode: nil -*-
// NOTE: the first line of this file sets up source code indentation rules
// for Emacs; it is also a hint to anyone modifying this file.

// Created      : Tue Jun  5 21:57:04 MDT 2018
// Copyright    : Pavel Koshevoy
// License      : MIT -- http://www.opensource.org/licenses/mit-license.php

#ifndef YAE_ITEM_VIEW_STYLE_H_
#define YAE_ITEM_VIEW_STYLE_H_

// aeyae:
#include "yae/api/yae_api.h"

// standard:
#include <map>

// Qt:
#include <QFont>

// yaeui:
#include "yaeColor.h"
#include "yaeItemView.h"
#include "yaeTexture.h"


namespace yae
{

  // forward declarations:
  class ItemView;


  //----------------------------------------------------------------
  // xbuttonImage
  //
  YAEUI_API QImage
  xbuttonImage(unsigned int w,
               const Color & color,
               const Color & background = Color(0x000000, 0.0),
               double thickness = 0.2,
               double rotateAngle = 45.0);

  //----------------------------------------------------------------
  // triangleImage
  //
  // create an image of an equilateral triangle inscribed within
  // an invisible circle of diameter w, and rotated about the center
  // of the circle by a given rotation angle (expressed in degrees):
  //
  YAEUI_API QImage
  triangleImage(unsigned int w,
                const Color & color,
                const Color & background = Color(0x0000000, 0.0),
                double rotateAngle = 0.0);

  //----------------------------------------------------------------
  // barsImage
  //
  YAEUI_API QImage
  barsImage(unsigned int w,
            const Color & color,
            const Color & background = Color(0x0000000, 0.0),
            unsigned int nbars = 2,
            double thickness = 0.8,
            double rotateAngle = 0.0);

  //----------------------------------------------------------------
  // trashcanImage
  //
  YAEUI_API QImage
  trashcanImage(unsigned int w,
                const Color & color,
                const Color & background);


  //----------------------------------------------------------------
  // ItemViewStyle
  //
  struct YAEUI_API ItemViewStyle : public Item
  {
    ItemViewStyle(const char * id, const ItemView & view);

    // virtual:
    void uncache();

    // a reference to GridCellWidth 
    inline yae::shared_ptr<GridCellWidth, TDoubleExpr> cell_width_expr() const
    {
      yae::shared_ptr<GridCellWidth, TDoubleExpr> expr =
        cell_width_.get_expr<GridCellWidth>();
      YAE_ASSERT(expr);
      return expr;
    }

    // the view that owns this style:
    const ItemView & view_;

    // font palette:
    QFont font_;
    QFont font_small_;
    QFont font_large_;
    QFont font_fixed_;

    // shared common properties:
    ItemRef dpi_;
    ItemRef device_pixel_ratio_;
    ItemRef row_height_;
    ItemRef cell_width_;
    ItemRef cell_height_;
    ItemRef title_height_;
    ItemRef font_size_;
    ItemRef unit_size_;

    // color palette:
    ColorRef bg_;
    ColorRef fg_;

    ColorRef border_;
    ColorRef cursor_;
    ColorRef cursor_fg_;
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
    ColorRef timeline_played_;

    // gradients:
    TGradientPtr timeline_shadow_;

    // textures:
    TTexturePtr grid_on_;
    TTexturePtr grid_off_;
    TTexturePtr pause_;
    TTexturePtr play_;

    // indexed by nearest power-of-2 size:
    std::map<uint32_t, TTexturePtr> trashcan_;
  };


  //----------------------------------------------------------------
  // ILayout
  //
  template <typename TModel, typename TView, typename TViewStyle>
  struct YAEUI_API ILayout
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
  // StyleAttr
  //
  template <typename TStyle, typename TDataRef>
  struct YAEUI_API StyleAttr : public Expression<typename TDataRef::value_type>
  {
    typedef typename TDataRef::value_type TData;

    StyleAttr(const ItemView & view,
              TDataRef TStyle::* const attr):
      view_(view),
      attr_(attr)
    {}

    // virtual:
    void evaluate(TData & result) const
    {
      const TStyle * style = dynamic_cast<const TStyle *>(view_.style());

      if (!style)
      {
        throw std::runtime_error("item view lacks expected style");
      }

      result = ((*style).*attr_).get();
    }

    const ItemView & view_;
    TDataRef TStyle::* const attr_;
  };

  //----------------------------------------------------------------
  // StyleAttr<TStyle, ItemRef>
  //
  template <typename TStyle>
  struct YAEUI_API StyleAttr<TStyle, ItemRef> : public TDoubleExpr
  {
    StyleAttr(const ItemView & view,
              ItemRef TStyle::* const attr,
              double scale = 1.0,
              double translate = 0.0,
              bool odd_round_up = false):
      view_(view),
      attr_(attr),
      scale_(scale),
      translate_(translate),
      odd_round_up_(odd_round_up)
    {}

    // virtual:
    void evaluate(double & result) const
    {
      const TStyle * style = dynamic_cast<const TStyle *>(view_.style());

      if (!style)
      {
        throw std::runtime_error("item view lacks expected style");
      }

      result = ((*style).*attr_).get();
      result *= scale_;
      result += translate_;

      if (odd_round_up_)
      {
        int i = 1 | int(ceil(result));
        result = double(i);
      }
    }

    const ItemView & view_;
    ItemRef TStyle::* const attr_;
    double scale_;
    double translate_;
    bool odd_round_up_;
  };

  //----------------------------------------------------------------
  // style_item_ref
  //
  template <typename TStyle>
  inline StyleAttr<TStyle, ItemRef> *
  style_item_ref(const ItemView & view,
                 ItemRef TStyle::* const attr,
                 double s = 1.0,
                 double t = 0.0,
                 bool odd_round_up = false)
  {
    return new StyleAttr<TStyle, ItemRef>(view, attr, s, t, odd_round_up);
  }

  //----------------------------------------------------------------
  // StyleAttr<TStyle, ColorRef>
  //
  template <typename TStyle>
  struct YAEUI_API StyleAttr<TStyle, ColorRef> : public TColorExpr
  {
    StyleAttr(const ItemView & view,
              ColorRef TStyle::* const attr,
              double alphaScale = 1.0,
              double alphaTranslate = 0.0):
      view_(view),
      attr_(attr),
      alphaScale_(alphaScale),
      alphaTranslate_(alphaTranslate)
    {}

    // virtual:
    void evaluate(Color & result) const
    {
      const TStyle * style = dynamic_cast<const TStyle *>(view_.style());

      if (!style)
      {
        throw std::runtime_error("item view lacks expected style");
      }

      result = ((*style).*attr_).get();
      result = result.a_scaled(alphaScale_, alphaTranslate_);
    }

    const ItemView & view_;
    ColorRef TStyle::* const attr_;
    double alphaScale_;
    double alphaTranslate_;
  };

  //----------------------------------------------------------------
  // style_color_ref
  //
  template <typename TStyle>
  inline StyleAttr<TStyle, ColorRef> *
  style_color_ref(const ItemView & view,
                  ColorRef TStyle::* const attr,
                  double s = 1.0,
                  double t = 0.0)
  {
    return new StyleAttr<TStyle, ColorRef>(view, attr, s, t);
  }

  //----------------------------------------------------------------
  // StyleTitleHeight
  //
  struct StyleTitleHeight : public StyleAttr<ItemViewStyle, ItemRef>
  {
    typedef StyleAttr<ItemViewStyle, ItemRef> TBase;

    StyleTitleHeight(const ItemView & view,
                     double s = 1.0,
                     double t = 0.0,
                     bool odd_round_up = false):
      TBase(view, &ItemViewStyle::title_height_, s, t, odd_round_up)
    {}
  };

  //----------------------------------------------------------------
  // StyleRowHeight
  //
  struct StyleRowHeight : public StyleAttr<ItemViewStyle, ItemRef>
  {
    typedef StyleAttr<ItemViewStyle, ItemRef> TBase;

    StyleRowHeight(const ItemView & view,
                   double s = 1.0,
                   double t = 0.0,
                   bool odd_round_up = false):
      TBase(view, &ItemViewStyle::row_height_, s, t, odd_round_up)
    {}
  };

  //----------------------------------------------------------------
  // StyleTimelineShadow
  //
  struct StyleTimelineShadow : public TGradientExpr
  {
    StyleTimelineShadow(const ItemView & view):
      view_(view)
    {}

    // virtual:
    void evaluate(TGradientPtr & result) const
    {
      const ItemViewStyle * style = view_.style();
      YAE_ASSERT(style);

      if (style)
      {
        result = style->timeline_shadow_;
      }
    }

    const ItemView & view_;
  };

  //----------------------------------------------------------------
  // StyleGridOnTexture
  //
  struct StyleGridOnTexture : public TTextureExpr
  {
    StyleGridOnTexture(const ItemView & view):
      view_(view)
    {}

    // virtual:
    void evaluate(TTexturePtr & result) const
    {
      const ItemViewStyle * style = view_.style();
      YAE_ASSERT(style);

      if (style)
      {
        result = style->grid_on_;
      }
    }

    const ItemView & view_;
  };

  //----------------------------------------------------------------
  // StyleGridOffTexture
  //
  struct StyleGridOffTexture : public TTextureExpr
  {
    StyleGridOffTexture(const ItemView & view):
      view_(view)
    {}

    // virtual:
    void evaluate(TTexturePtr & result) const
    {
      const ItemViewStyle * style = view_.style();
      YAE_ASSERT(style);

      if (style)
      {
        result = style->grid_off_;
      }
    }

    const ItemView & view_;
  };

  //----------------------------------------------------------------
  // StylePauseTexture
  //
  struct StylePauseTexture : public TTextureExpr
  {
    StylePauseTexture(const ItemView & view):
      view_(view)
    {}

    // virtual:
    void evaluate(TTexturePtr & result) const
    {
      const ItemViewStyle * style = view_.style();
      YAE_ASSERT(style);

      if (style)
      {
        result = style->pause_;
      }
    }

    const ItemView & view_;
  };

  //----------------------------------------------------------------
  // StylePlayTexture
  //
  struct StylePlayTexture : public TTextureExpr
  {
    StylePlayTexture(const ItemView & view):
      view_(view)
    {}

    // virtual:
    void evaluate(TTexturePtr & result) const
    {
      const ItemViewStyle * style = view_.style();
      YAE_ASSERT(style);

      if (style)
      {
        result = style->play_;
      }
    }

    const ItemView & view_;
  };

  //----------------------------------------------------------------
  // GetTexTrashcan
  //
  struct GetTexTrashcan : public TTextureExpr
  {
    GetTexTrashcan(const ItemView & view, const Item & item);

    // virtual:
    void evaluate(TTexturePtr & result) const;

    const ItemView & view_;
    const Item & item_;
  };

}

#endif // YAE_ITEM_VIEW_STYLE_H_
