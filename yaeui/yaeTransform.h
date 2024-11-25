// -*- Mode: c++; tab-width: 8; c-basic-offset: 2; indent-tabs-mode: nil -*-
// NOTE: the first line of this file sets up source code indentation rules
// for Emacs; it is also a hint to anyone modifying this file.

// Created      : Tue Oct 20 19:19:59 PDT 2015
// Copyright    : Pavel Koshevoy
// License      : MIT -- http://www.opensource.org/licenses/mit-license.php

#ifndef YAE_TRANSFORM_H_
#define YAE_TRANSFORM_H_

// aeyae:
#include "yae/api/yae_api.h"

// yaeui:
#include "yaeItem.h"


namespace yae
{

  //----------------------------------------------------------------
  // Transform
  //
  struct Transform : public Item
  {
    friend struct TransformedXContent;
    friend struct TransformedYContent;

    Transform(const char * id);

    // virtual:
    void uncache();
    bool paint(const Segment & xregion,
               const Segment & yregion,
               Canvas * canvas) const;

    // virtual:
    void getInputHandlers(// coordinate system origin of
                          // the input area, expressed in the
                          // coordinate system of the root item:
                          const TVec2D & itemCSysOrigin,

                          // point expressed in the coord. system of the item,
                          // rootCSysPoint = itemCSysOrigin + itemCSysPoint
                          const TVec2D & itemCSysPoint,

                          // pass back input areas overlapping above point,
                          // along with its coord. system origin expressed
                          // in the coordinate system of the root item:
                          std::list<InputHandler> & inputHandlers);

    // virtual:
    void getVisibleItems(// coordinate system origin of
                         // the item, expressed in the
                         // coordinate system of the root item:
                         const TVec2D & itemCSysOrigin,

                         // point expressed in the coord.sys. of the item,
                         // rootCSysPoint = itemCSysOrigin + itemCSysPoint
                         const TVec2D & itemCSysPoint,

                         // pass back items overlapping above point,
                         // along with its coord. system origin expressed
                         // in the coordinate system of the root item:
                         std::list<VisibleItem> & visibleItems);

    // helper:
    void getCSys(TVec2D & origin, TVec2D & uAxis, TVec2D & vAxis) const;

    // nested item coordinate system rotation angle
    // relative to this items local coordinate system,
    // expressed in radians:
    ItemRef rotation_;

  protected:
    SegmentRef xContentLocal_;
    SegmentRef yContentLocal_;

    // local coordinate system u-axis (v-axis is derived from u-axis):
    TVec2DRef uAxis_;
  };


  //----------------------------------------------------------------
  // TransformedXContent
  //
  struct TransformedXContent : public TSegmentExpr
  {
    TransformedXContent(const Transform & item);

    // virtual:
    void evaluate(Segment & result) const;

    const Transform & item_;
  };


  //----------------------------------------------------------------
  // TransformedYContent
  //
  struct TransformedYContent : public TSegmentExpr
  {
    TransformedYContent(const Transform & item);

    // virtual:
    void evaluate(Segment & result) const;

    const Transform & item_;
  };

}


#endif // YAE_TRANSFORM_H_
