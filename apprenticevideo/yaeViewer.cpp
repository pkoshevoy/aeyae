// -*- Mode: c++; tab-width: 8; c-basic-offset: 2; indent-tabs-mode: nil -*-
// NOTE: the first line of this file sets up source code indentation rules
// for Emacs; it is also a hint to anyone modifying this file.

// Created   : Mon Jul  5 19:33:44 MDT 2010
// Copyright : Pavel Koshevoy
// License   : MIT -- http://www.opensource.org/licenses/mit-license.php


// yae includes:
#include <yaeAPI.h>
#include <yaeReader.h>
#include <yaeViewer.h>

// Qt includes:
#include <QGridLayout>


//----------------------------------------------------------------
// loadQImagePlane
// 
static void
loadQImagePlane(QImage & qimg,
                bool flipTheImage,
                const unsigned char * data,
                const unsigned int dataRowBytes)
{
  const unsigned int w = qimg.width();
  const unsigned int h = qimg.height();
  
  unsigned char * qimgData = qimg.bits();
  int qimgRowBytes = qimg.bytesPerLine();
  
  if (flipTheImage)
  {
    qimgData += qimgRowBytes * (h - 1);
    qimgRowBytes = -qimgRowBytes;
  }
  
  for (unsigned int i = 0; i < h; ++i,
         data += dataRowBytes, qimgData += qimgRowBytes)
  {
    const unsigned char * dataPixel = data;
    unsigned char * qimgPixel = qimgData;
    
    for (unsigned int j = 0; j < w; ++j)
    {
      *qimgPixel++ = *dataPixel;
      *qimgPixel++ = *dataPixel;
      *qimgPixel++ = *dataPixel++;
    }
  }
}

//----------------------------------------------------------------
// clip
// 
template <typename data_t>
static data_t
clip(const data_t & n, const data_t & min, const data_t & max)
{
  return std::min(max, std::max(min, n));
}

//----------------------------------------------------------------
// loadQImageYUV
// 
static void
loadQImageYUV(QImage & qimg,
              bool flipTheImage,
              const unsigned char * yuv420p)
{
  const unsigned int w = qimg.width();
  const unsigned int h = qimg.height();
  
  const unsigned int u_offset = w * h;
  const unsigned int v_offset = (u_offset >> 2);
  const unsigned int w2 = w / 2;
  
  // convert YUV420 into RGB:
  for (unsigned int y = 0; y < h; y++)
  {
    unsigned int qimgY = flipTheImage ? h - y - 1 : y;
    
    for (unsigned int x = 0; x < w; x++)
    {
      unsigned int iy = y * w + x;
      unsigned int iu = u_offset + (y >> 1) * w2 + (x >> 1);
      unsigned int iv = iu + v_offset;
      
      const unsigned char & Y = *(yuv420p + iy);
      const unsigned char & U = *(yuv420p + iu);
      const unsigned char & V = *(yuv420p + iv);
      
      // taken from wikipedia:
      int C = Y - 16;
      int D = U - 128;
      int E = V - 128;
      int R = clip((298 * C           + 409 * E + 128) >> 8, 0, 255);
      int G = clip((298 * C - 100 * D - 208 * E + 128) >> 8, 0, 255);
      int B = clip((298 * C + 516 * D           + 128) >> 8, 0, 255);
      
      QRgb pixel = qRgb(R, G, B);
      qimg.setPixel(x, qimgY, pixel);
    }
  }
}


namespace yae
{
  
  //----------------------------------------------------------------
  // Viewer::Viewer
  // 
  Viewer::Viewer(IReader * reader):
    QWidget(),
    reader_(NULL),
    flipTheImage_(false)
  {
    QGridLayout * grid = new QGridLayout(this);
    grid->setSpacing(0);
    grid->setContentsMargins(0, 0, 0, 0);
    
    labelY_ = new QLabel(this);
    grid->addWidget(labelY_, 0, 0);
    labelY_->setAlignment(Qt::AlignHCenter | Qt::AlignVCenter);
    labelY_->show();
    
    labelU_ = new QLabel(this);
    grid->addWidget(labelU_, 1, 0);
    labelU_->setAlignment(Qt::AlignHCenter | Qt::AlignVCenter);
    labelU_->show();
    
    labelV_ = new QLabel(this);
    grid->addWidget(labelV_, 1, 1);
    labelV_->setAlignment(Qt::AlignHCenter | Qt::AlignVCenter);
    labelV_->show();
    
    labelRGB_ = new QLabel(this);
    grid->addWidget(labelRGB_, 0, 1);
    labelRGB_->setAlignment(Qt::AlignHCenter | Qt::AlignVCenter);
    labelRGB_->show();
    
    setReader(reader);
  }

  //----------------------------------------------------------------
  // Viewer::~Viewer
  // 
  Viewer::~Viewer()
  {
    if (reader_)
    {
      reader_->destroy();
    }
  }
  
  //----------------------------------------------------------------
  // Viewer::setReader
  // 
  void
  Viewer::setReader(IReader * reader)
  {
    reader_ = reader;
    traits_ = VideoTraits();
    frame_ =  TVideoFramePtr();
    flipTheImage_ = false;
    
    if (reader_)
    {
      reader_->getVideoTraits(traits_);
    }
    
    std::size_t imgW = traits_.visibleWidth_;
    std::size_t imgH = traits_.visibleHeight_;
    
    y_ = QImage(imgW, imgH, QImage::Format_RGB888);
    u_ = QImage(imgW / 2, imgH / 2, QImage::Format_RGB888);
    v_ = QImage(imgW / 2, imgH / 2, QImage::Format_RGB888);
    rgb_ = QImage(imgW, imgH, QImage::Format_RGB888);
    
    setMinimumSize(imgW * 2, imgH * 2);
    labelY_->setMinimumSize(imgW, imgH);
    labelU_->setMinimumSize(imgW, imgH);
    labelV_->setMinimumSize(imgW, imgH);
    labelRGB_->setMinimumSize(imgW, imgH);
  }

  //----------------------------------------------------------------
  // Viewer::loadFrame
  // 
  bool
  Viewer::loadFrame()
  {
    if (!reader_->readVideo(frame_))
    {
      return false;
    }
    
    std::size_t imgW = frame_->traits_.visibleWidth_;
    std::size_t imgH = frame_->traits_.visibleHeight_;
    std::size_t yFrameSize = imgW * imgH;
    std::size_t uFrameSize = imgW * imgH / 4;
    const unsigned char * dataBuffer = frame_->getBuffer<unsigned char>();
    
    // update the labels:
    loadQImagePlane(y_,
                    flipTheImage_,
                    dataBuffer,
                    imgW);
    
    loadQImagePlane(u_,
                    flipTheImage_,
                    dataBuffer + yFrameSize,
                    imgW / 2);
    
    loadQImagePlane(v_,
                    flipTheImage_,
                    dataBuffer + yFrameSize + uFrameSize,
                    imgW / 2);
    
    loadQImageYUV(rgb_, flipTheImage_, dataBuffer);
    
    labelY_->setPixmap(QPixmap::fromImage(y_));
    labelU_->setPixmap(QPixmap::fromImage(u_));
    labelV_->setPixmap(QPixmap::fromImage(v_));
    labelRGB_->setPixmap(QPixmap::fromImage(rgb_));
    
    std::cout << "loaded frame " << frame_ << std::endl;
    return true;
  }
  
  //----------------------------------------------------------------
  // Viewer::keyPressEvent
  // 
  void
  Viewer::keyPressEvent(QKeyEvent * event)
  {
    const int k = event->key();
    switch (k)
    {
      case Qt::Key_PageUp:
      case Qt::Key_Up:
      case Qt::Key_Left:
	// stepBack();
	loadFrame();
        event->accept();
        return;

      case Qt::Key_PageDown:
      case Qt::Key_Down:
      case Qt::Key_Right:
	loadFrame();
        event->accept();
        return;

      case Qt::Key_F:
        flipTheImage_ = !flipTheImage_;
	// stepBack();
        loadFrame();
        event->accept();
        return;
        
      default:
        break;
    }
    
    QWidget::keyPressEvent(event);
  }
  
}
