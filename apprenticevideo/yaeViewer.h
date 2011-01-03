// -*- Mode: c++; tab-width: 8; c-basic-offset: 2; indent-tabs-mode: nil -*-
// NOTE: the first line of this file sets up source code indentation rules
// for Emacs; it is also a hint to anyone modifying this file.

// Created   : Mon Jul  5 19:22:58 MDT 2010
// Copyright : Pavel Koshevoy
// License   : MIT -- http://www.opensource.org/licenses/mit-license.php

#ifndef YAE_VIEWER_H_
#define YAE_VIEWER_H_

// system includes:
#include <stdio.h>
#include <assert.h>
#include <iostream>
#include <vector>

// Qt includes:
#include <QImage>
#include <QLabel>
#include <QWidget>
#include <QKeyEvent>

// yae includes:
#include <yaeAPI.h>
#include <yaeReader.h>


namespace yae
{

  //----------------------------------------------------------------
  // Viewer
  // 
  // Simple UI used for debugging Readers
  // 
  class Viewer : public QWidget
  {
    Q_OBJECT;
    
  public:
    Viewer(IReader * reader);
    ~Viewer();
    
    void setReader(IReader * reader);
    
    bool loadFrame();
    
  protected:
    // virtual:
    void keyPressEvent(QKeyEvent * event);
    
    QLabel * labelY_;
    QLabel * labelU_;
    QLabel * labelV_;
    QLabel * labelRGB_;
  
    QImage y_;
    QImage u_;
    QImage v_;
    QImage rgb_;
    
    IReader * reader_;
    VideoTraits traits_;
    TVideoFramePtr frame_;
    bool flipTheImage_;
  };
  
}

#endif // YAE_VIEWER_H_
