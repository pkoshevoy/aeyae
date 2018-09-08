// -*- Mode: c++; tab-width: 8; c-basic-offset: 2; indent-tabs-mode: nil -*-
// NOTE: the first line of this file sets up source code indentation rules
// for Emacs; it is also a hint to anyone modifying this file.

// Created      : Tue Oct 20 19:19:59 PDT 2015
// Copyright    : Pavel Koshevoy
// License      : MIT -- http://www.opensource.org/licenses/mit-license.php

// Qt library:
#include <QImage>

// local interfaces:
#include "yaeCanvasRenderer.h"
#include "yaeTransform.h"


namespace yae
{

  //----------------------------------------------------------------
  // GetUAxis
  //
  struct GetUAxis : public TVec2DExpr
  {
    GetUAxis(const ItemRef & rotation):
      rotation_(rotation)
    {}

    // virtual:
    void evaluate(TVec2D & result) const
    {
      double angle = rotation_.get();
      double sin_a = sin(angle);
      double cos_a = cos(angle);
      result.set_x(cos_a);
      result.set_y(sin_a);
    }

    const ItemRef & rotation_;
  };

  //----------------------------------------------------------------
  // getVAxis
  //
  inline static TVec2D
  getVAxis(const TVec2D & u_axis)
  {
    // v-axis is orthogonal to u-axis,
    // meaning it is at a 90 degree angle to u-axis;
    //
    // cos(a + pi/2) == -sin(a)
    // sin(a + pi/2) ==  cos(a)
    //
    return TVec2D(-u_axis.y(), u_axis.x());
  }

  //----------------------------------------------------------------
  // SameExpression
  //
  struct SameExpression
  {
    SameExpression(const IPropertiesBase * ref = NULL):
      ref_(ref)
    {}

    inline bool operator()(const TPropertiesBasePtr & expression) const
    {
      if (expression.get() == ref_)
      {
        return true;
      }

      return false;
    }

    const IPropertiesBase * ref_;
  };

  //----------------------------------------------------------------
  // Transform::Transform
  //
  Transform::Transform(const char * id):
    Item(id),
    rotation_(ItemRef::constant(0.0))
  {
    // override base class content and extent calculation
    // in order to handle coordinate system transformations:
    xContentLocal_ = Item::xContent_;
    yContentLocal_ = Item::yContent_;
    Item::xContent_ = addExpr(new TransformedXContent(*this));
    Item::yContent_ = addExpr(new TransformedYContent(*this));

    // get rid of base implementation of xExtent and yExtent:
    Item::expr_.remove_if(SameExpression(Item::xExtent_.ref()));
    Item::expr_.remove_if(SameExpression(Item::yExtent_.ref()));

    Item::xExtent_ = Item::xContent_;
    Item::yExtent_ = Item::yContent_;

    uAxis_ = addExpr(new GetUAxis(rotation_));
  }

  //----------------------------------------------------------------
  // Transform::uncache
  //
  void
  Transform::uncache()
  {
    rotation_.uncache();
    xContentLocal_.uncache();
    yContentLocal_.uncache();
    uAxis_.uncache();
    Item::uncache();
  }

  //----------------------------------------------------------------
  // Transform::getInputHandlers
  //
  void
  Transform::getInputHandlers(// coordinate system origin of
                              // the input area, expressed in the
                              // coordinate system of the root item:
                              const TVec2D & itemCSysOrigin,

                              // point expressed in the coord.sys. of the item,
                              // rootCSysPoint = itemCSysOrigin + itemCSysPoint
                              const TVec2D & itemCSysPoint,

                              // pass back input areas overlapping above point,
                              // along with its coord. system origin expressed
                              // in the coordinate system of the root item:
                              std::list<InputHandler> & inputHandlers)
  {
    if (!Item::overlaps(itemCSysPoint))
    {
      return;
    }

    TVec2D origin;
    TVec2D u_axis;
    TVec2D v_axis;
    getCSys(origin, u_axis, v_axis);

    // transform point to local coordinate system:
    TVec2D localCSysOffset = itemCSysOrigin + origin;
    TVec2D ptInLocalCoords = wcs_to_lcs(origin, u_axis, v_axis, itemCSysPoint);

    for (std::vector<ItemPtr>::const_iterator i = Item::children_.begin();
         i != Item::children_.end(); ++i)
    {
      const ItemPtr & child = *i;
      child->getInputHandlers(localCSysOffset, ptInLocalCoords, inputHandlers);
    }
  }

  //----------------------------------------------------------------
  // Transform::getVisibleItems
  //
  void
  Transform::getVisibleItems(// coordinate system origin of
                             // the item, expressed in the
                             // coordinate system of the root item:
                             const TVec2D & itemCSysOrigin,

                             // point expressed in the coord.sys. of the item,
                             // rootCSysPoint = itemCSysOrigin + itemCSysPoint
                             const TVec2D & itemCSysPoint,

                             // pass back items overlapping above point,
                             // along with its coord. system origin expressed
                             // in the coordinate system of the root item:
                             std::list<VisibleItem> & visibleItems)
  {
    if (!(Item::visible() && Item::overlaps(itemCSysPoint)))
    {
      return;
    }

    TVec2D origin;
    TVec2D u_axis;
    TVec2D v_axis;
    getCSys(origin, u_axis, v_axis);

    // transform point to local coordinate system:
    TVec2D localCSysOffset = itemCSysOrigin + origin;
    TVec2D ptInLocalCoords = wcs_to_lcs(origin, u_axis, v_axis, itemCSysPoint);

    for (std::vector<ItemPtr>::const_iterator i = Item::children_.begin();
         i != Item::children_.end(); ++i)
    {
      const ItemPtr & child = *i;
      child->getVisibleItems(localCSysOffset, ptInLocalCoords, visibleItems);
    }

    visibleItems.push_back(VisibleItem(this, itemCSysOrigin));
  }

  //----------------------------------------------------------------
  // Transform::paint
  //
  bool
  Transform::paint(const Segment & xregion,
                   const Segment & yregion,
                   Canvas * canvas) const
  {
    if (!visibleInRegion(xregion, yregion))
    {
      unpaint();
      return false;
    }

    this->paintContent();
    painted_ = true;

    // transform x,y-region to local coordinate system u,v-region:
    TVec2D origin;
    TVec2D u_axis;
    TVec2D v_axis;
    getCSys(origin, u_axis, v_axis);

    double angle = rotation_.get();
    double degrees = 180.0 * (angle / M_PI);

    TVec2D p00 = wcs_to_lcs(origin, u_axis, v_axis,
                            TVec2D(xregion.start(), yregion.start()));

    TVec2D p01 = wcs_to_lcs(origin, u_axis, v_axis,
                            TVec2D(xregion.start(), yregion.end()));

    TVec2D p10 = wcs_to_lcs(origin, u_axis, v_axis,
                            TVec2D(xregion.end(), yregion.start()));

    TVec2D p11 = wcs_to_lcs(origin, u_axis, v_axis,
                            TVec2D(xregion.end(), yregion.end()));

    double u0 = std::min<double>(std::min<double>(p00.x(), p01.x()),
                                 std::min<double>(p10.x(), p11.x()));

    double u1 = std::max<double>(std::max<double>(p00.x(), p01.x()),
                                 std::max<double>(p10.x(), p11.x()));

    double v0 = std::min<double>(std::min<double>(p00.y(), p01.y()),
                                 std::min<double>(p10.y(), p11.y()));

    double v1 = std::max<double>(std::max<double>(p00.y(), p01.y()),
                                 std::max<double>(p10.y(), p11.y()));

    Segment uregion(u0, u1 - u0);
    Segment vregion(v0, v1 - v0);

    TGLSaveMatrixState pushMatrix(GL_MODELVIEW);
    YAE_OGL_11_HERE();
    YAE_OGL_11(glTranslated(origin.x(), origin.y(), 0.0));
    YAE_OGL_11(glRotated(degrees, 0.0, 0.0, 1.0));

    TGLSaveClientState pushClientAttr(GL_CLIENT_ALL_ATTRIB_BITS);
    YAE_OGL_11(glEnable(GL_LINE_SMOOTH));
    YAE_OGL_11(glHint(GL_LINE_SMOOTH_HINT, GL_NICEST));
    YAE_OGL_11(glEnable(GL_POLYGON_SMOOTH));
    YAE_OGL_11(glHint(GL_POLYGON_SMOOTH_HINT, GL_NICEST));

    this->paintChildren(uregion, vregion, canvas);

    return true;
  }

  //----------------------------------------------------------------
  // Transform::getCSys
  //
  void
  Transform::getCSys(TVec2D & origin, TVec2D & uAxis, TVec2D & vAxis) const
  {
    YAE_ASSERT(anchors_.hcenter_.isValid() && anchors_.vcenter_.isValid());
    origin.set_x(anchors_.hcenter_.get());
    origin.set_y(anchors_.vcenter_.get());
    uAxis = uAxis_.get();
    vAxis = getVAxis(uAxis);
  }


  //----------------------------------------------------------------
  // TransformedXContent::TransformedXContent
  //
  TransformedXContent::TransformedXContent(const Transform & item):
    item_(item)
  {}

  //----------------------------------------------------------------
  // TransformedXContent::evaluate
  //
  void
  TransformedXContent::evaluate(Segment & result) const
  {
    Segment xcontent = item_.xContentLocal_.get();
    Segment ycontent = item_.yContentLocal_.get();

    TVec2D origin;
    TVec2D u_axis;
    TVec2D v_axis;
    item_.getCSys(origin, u_axis, v_axis);

    TVec2D p00 = lcs_to_wcs(origin, u_axis, v_axis,
                            TVec2D(xcontent.start(), ycontent.start()));

    TVec2D p01 = lcs_to_wcs(origin, u_axis, v_axis,
                            TVec2D(xcontent.start(), ycontent.end()));

    TVec2D p10 = lcs_to_wcs(origin, u_axis, v_axis,
                            TVec2D(xcontent.end(), ycontent.start()));

    TVec2D p11 = lcs_to_wcs(origin, u_axis, v_axis,
                            TVec2D(xcontent.end(), ycontent.end()));

    double x0 = std::min<double>(std::min<double>(p00.x(), p01.x()),
                                 std::min<double>(p10.x(), p11.x()));

    double x1 = std::max<double>(std::max<double>(p00.x(), p01.x()),
                                 std::max<double>(p10.x(), p11.x()));

    result = Segment(x0, x1 - x0);
  }


  //----------------------------------------------------------------
  // TransformedYContent::TransformedYContent
  //
  TransformedYContent::TransformedYContent(const Transform & item):
    item_(item)
  {}

  //----------------------------------------------------------------
  // TransformedYContent::evaluate
  //
  void
  TransformedYContent::evaluate(Segment & result) const
  {
    Segment xcontent = item_.xContentLocal_.get();
    Segment ycontent = item_.yContentLocal_.get();

    TVec2D origin;
    TVec2D u_axis;
    TVec2D v_axis;
    item_.getCSys(origin, u_axis, v_axis);

    TVec2D p00 = lcs_to_wcs(origin, u_axis, v_axis,
                            TVec2D(xcontent.start(), ycontent.start()));

    TVec2D p01 = lcs_to_wcs(origin, u_axis, v_axis,
                            TVec2D(xcontent.start(), ycontent.end()));

    TVec2D p10 = lcs_to_wcs(origin, u_axis, v_axis,
                            TVec2D(xcontent.end(), ycontent.start()));

    TVec2D p11 = lcs_to_wcs(origin, u_axis, v_axis,
                            TVec2D(xcontent.end(), ycontent.end()));

    double y0 = std::min<double>(std::min<double>(p00.y(), p01.y()),
                                 std::min<double>(p10.y(), p11.y()));

    double y1 = std::max<double>(std::max<double>(p00.y(), p01.y()),
                                 std::max<double>(p10.y(), p11.y()));

    result = Segment(y0, y1 - y0);
  }

}
