// -*- Mode: c++; tab-width: 8; c-basic-offset: 2; indent-tabs-mode: nil -*-
// NOTE: the first line of this file sets up source code indentation rules
// for Emacs; it is also a hint to anyone modifying this file.

// Created      : Fri Dec 18 23:01:16 PST 2015
// Copyright    : Pavel Koshevoy
// License      : MIT -- http://www.opensource.org/licenses/mit-license.php

// local includes:
#include "yaeColor.h"
#include "yaeGradient.h"
#include "yaeItemRef.h"
#include "yaePlaylistView.h"
#include "yaeProperty.h"
#include "yaeRectangle.h"
#include "yaeRoundRect.h"
#include "yaeSegment.h"
#include "yaeText.h"
#include "yaeTimelineControls.h"
#include "yaeTimelineView.h"

namespace yae
{

  //----------------------------------------------------------------
  // TimelineView::TimelineView
  //
  TimelineView::TimelineView():
    ItemView("timeline"),
    model_(NULL)
  {
    Item & root = *root_;

    // setup an invisible item so its height property expression
    // could be computed once and the result reused in other places
    // that need to compute the same property expression:
    Item & titleHeight = root.addNewHidden<Item>("title_height");
    titleHeight.height_ =
      titleHeight.addExpr(new CalcTitleHeight(root, 24.0));

    Gradient & shadow = root.addNew<Gradient>("shadow");
    shadow.anchors_.fill(root);
    shadow.anchors_.top_.reset();
    shadow.height_ = ItemRef::scale(titleHeight, kPropertyHeight, 3.0);

    shadow.color_[0.000000] = Color(0x000000, 0.00392156862745098039);
    shadow.color_[0.052083] = Color(0x000000, 0.00784313725490196078);
    shadow.color_[0.083333] = Color(0x000000, 0.01176470588235294117);
    shadow.color_[0.135417] = Color(0x000000, 0.01568627450980392156);
    shadow.color_[0.156250] = Color(0x000000, 0.01960784313725490196);
    shadow.color_[0.166667] = Color(0x000000, 0.02352941176470588235);
    shadow.color_[0.187500] = Color(0x000000, 0.02745098039215686274);
    shadow.color_[0.208333] = Color(0x000000, 0.03137254901960784313);
    shadow.color_[0.218750] = Color(0x000000, 0.03529411764705882352);
    shadow.color_[0.229167] = Color(0x000000, 0.03921568627450980392);
    shadow.color_[0.239583] = Color(0x000000, 0.04313725490196078431);
    shadow.color_[0.260417] = Color(0x000000, 0.04705882352941176470);
    shadow.color_[0.270833] = Color(0x000000, 0.05098039215686274509);
    shadow.color_[0.281250] = Color(0x000000, 0.05882352941176470588);
    shadow.color_[0.302083] = Color(0x000000, 0.06274509803921568627);
    shadow.color_[0.312500] = Color(0x000000, 0.07058823529411764705);
    shadow.color_[0.322917] = Color(0x000000, 0.07450980392156862745);
    shadow.color_[0.333333] = Color(0x000000, 0.07843137254901960784);
    shadow.color_[0.343750] = Color(0x000000, 0.08627450980392156862);
    shadow.color_[0.354167] = Color(0x000000, 0.09019607843137254901);
    shadow.color_[0.364583] = Color(0x000000, 0.09411764705882352941);
    shadow.color_[0.375000] = Color(0x000000, 0.10196078431372549019);
    shadow.color_[0.385417] = Color(0x000000, 0.10588235294117647058);
    shadow.color_[0.395833] = Color(0x000000, 0.11372549019607843137);
    shadow.color_[0.406250] = Color(0x000000, 0.12156862745098039215);
    shadow.color_[0.416667] = Color(0x000000, 0.12941176470588235294);
    shadow.color_[0.427083] = Color(0x000000, 0.13333333333333333333);
    shadow.color_[0.437500] = Color(0x000000, 0.14117647058823529411);
    shadow.color_[0.447917] = Color(0x000000, 0.14901960784313725490);
    shadow.color_[0.458333] = Color(0x000000, 0.15686274509803921568);
    shadow.color_[0.468750] = Color(0x000000, 0.16470588235294117647);
    shadow.color_[0.479167] = Color(0x000000, 0.17254901960784313725);
    shadow.color_[0.489583] = Color(0x000000, 0.18039215686274509803);
    shadow.color_[0.500000] = Color(0x000000, 0.19215686274509803921);
    shadow.color_[0.510417] = Color(0x000000, 0.20000000000000000000);
    shadow.color_[0.520833] = Color(0x000000, 0.20784313725490196078);
    shadow.color_[0.531250] = Color(0x000000, 0.21960784313725490196);
    shadow.color_[0.541667] = Color(0x000000, 0.22745098039215686274);
    shadow.color_[0.552083] = Color(0x000000, 0.23921568627450980392);
    shadow.color_[0.562500] = Color(0x000000, 0.25098039215686274509);
    shadow.color_[0.572917] = Color(0x000000, 0.25882352941176470588);
    shadow.color_[0.583333] = Color(0x000000, 0.26666666666666666666);
    shadow.color_[0.593750] = Color(0x000000, 0.27843137254901960784);
    shadow.color_[0.604167] = Color(0x000000, 0.28627450980392156862);
    shadow.color_[0.614583] = Color(0x000000, 0.29803921568627450980);
    shadow.color_[0.625000] = Color(0x000000, 0.30980392156862745098);
    shadow.color_[0.635417] = Color(0x000000, 0.32156862745098039215);
    shadow.color_[0.645833] = Color(0x000000, 0.32941176470588235294);
    shadow.color_[0.656250] = Color(0x000000, 0.34509803921568627450);
    shadow.color_[0.666667] = Color(0x000000, 0.35294117647058823529);
    shadow.color_[0.677083] = Color(0x000000, 0.36470588235294117647);
    shadow.color_[0.687500] = Color(0x000000, 0.37647058823529411764);
    shadow.color_[0.697917] = Color(0x000000, 0.38823529411764705882);
    shadow.color_[0.708333] = Color(0x000000, 0.39215686274509803921);
    shadow.color_[0.718750] = Color(0x000000, 0.40392156862745098039);
    shadow.color_[0.729167] = Color(0x000000, 0.41568627450980392156);
    shadow.color_[0.739583] = Color(0x000000, 0.42745098039215686274);
    shadow.color_[0.750000] = Color(0x000000, 0.43529411764705882352);
    shadow.color_[0.760417] = Color(0x000000, 0.44705882352941176470);
    shadow.color_[0.770833] = Color(0x000000, 0.45882352941176470588);
    shadow.color_[0.781250] = Color(0x000000, 0.47058823529411764705);
    shadow.color_[0.791667] = Color(0x000000, 0.47843137254901960784);
    shadow.color_[0.802083] = Color(0x000000, 0.49411764705882352941);
    shadow.color_[0.812500] = Color(0x000000, 0.50196078431372549019);
    shadow.color_[0.822917] = Color(0x000000, 0.51372549019607843137);
    shadow.color_[0.833333] = Color(0x000000, 0.52156862745098039215);
    shadow.color_[0.843750] = Color(0x000000, 0.52941176470588235294);
    shadow.color_[0.854167] = Color(0x000000, 0.53725490196078431372);
    shadow.color_[0.864583] = Color(0x000000, 0.55294117647058823529);
    shadow.color_[0.875000] = Color(0x000000, 0.56078431372549019607);
    shadow.color_[0.885417] = Color(0x000000, 0.56862745098039215686);
    shadow.color_[0.895833] = Color(0x000000, 0.58039215686274509803);
    shadow.color_[0.906250] = Color(0x000000, 0.58823529411764705882);
    shadow.color_[0.916667] = Color(0x000000, 0.59607843137254901960);
    shadow.color_[0.927083] = Color(0x000000, 0.60784313725490196078);
    shadow.color_[0.937500] = Color(0x000000, 0.61960784313725490196);
    shadow.color_[0.947917] = Color(0x000000, 0.63137254901960784313);
    shadow.color_[0.958333] = Color(0x000000, 0.64313725490196078431);
    shadow.color_[0.968750] = Color(0x000000, 0.65490196078431372549);
    shadow.color_[0.979167] = Color(0x000000, 0.66666666666666666666);
    shadow.color_[0.989583] = Color(0x000000, 0.67843137254901960784);
    shadow.color_[1.000000] = Color(0x000000, 0.69019607843137254901);
  }

  //----------------------------------------------------------------
  // TimelineView::setModel
  //
  void
  TimelineView::setModel(TimelineControls * model)
  {
    model_ = model;
  }

}
