// -*- Mode: c++; tab-width: 8; c-basic-offset: 2; indent-tabs-mode: nil -*-
// NOTE: the first line of this file sets up source code indentation rules
// for Emacs; it is also a hint to anyone modifying this file.

// Created      : Sun Dec 20 20:13:45 PST 2015
// Copyright    : Pavel Koshevoy
// License      : MIT -- http://www.opensource.org/licenses/mit-license.php

// Qt library:
#include <QApplication>
#include <QImage>
#include <QLineEdit>
#include <QTextLayout>

// local interfaces:
#include "yaeCanvasRenderer.h"
#include "yaeItemFocus.h"
#include "yaeTextInput.h"
#include "yaeTexture.h"
#include "yaeUtilsQt.h"


namespace yae
{

  //----------------------------------------------------------------
  // TextInput::TPrivate
  //
  struct TextInput::TPrivate
  {
    TPrivate();
   ~TPrivate();

    bool processEvent(TextInput & item,
                      Canvas::ILayer & canvasLayer,
                      Canvas * canvas,
                      QEvent * e);

    void uncache();
    bool uploadTexture(const TextInput & item);
    void paint(const TextInput & item);

    QWidget * parent_;
    QLineEdit lineEdit_;

    BoolRef ready_;
    qreal offset_;
    GLuint texId_;
    GLuint iw_;
    GLuint ih_;
  };

  //----------------------------------------------------------------
  // TextInput::TPrivate::TPrivate
  //
  TextInput::TPrivate::TPrivate():
    parent_(QApplication::topLevelWidgets().front()),
    offset_(0),
    texId_(0),
    iw_(0),
    ih_(0)
  {
    lineEdit_.hide();
  }

  //----------------------------------------------------------------
  // TextInput::TPrivate::~TPrivate
  //
  TextInput::TPrivate::~TPrivate()
  {
    uncache();
  }

  //----------------------------------------------------------------
  // TextInput::TPrivate::processEvent
  //
  bool
  TextInput::TPrivate::processEvent(TextInput & item,
                                    Canvas::ILayer & canvasLayer,
                                    Canvas * canvas,
                                    QEvent * e)
  {
    QEvent::Type et = e->type();
    if (et == QEvent::Paint)
    {
      return false;
    }

    if (et == QEvent::KeyPress)
    {
      QKeyEvent & ke = *(static_cast<QKeyEvent *>(e));
      if (ke.key() == Qt::Key_Escape)
      {
        ItemFocus::singleton().clearFocus();
        return true;
      }
    }

    bool ok = false;
    QString before;
    QString after;

    if (/* et == QEvent::MouseButtonPress ||
        et == QEvent::MouseButtonRelease ||
        et == QEvent::MouseButtonDblClick ||
        et == QEvent::MouseMove || */
        et == QEvent::KeyPress ||
        et == QEvent::KeyRelease ||
        et == QEvent::ShortcutOverride ||
        et == QEvent::Shortcut ||
        et == QEvent::InputMethod ||
        et == QEvent::LocaleChange ||
        et == QEvent::LanguageChange ||
        et == QEvent::LayoutDirectionChange ||
        et == QEvent::KeyboardLayoutChange ||
        et == QEvent::RequestSoftwareInputPanel ||
        et == QEvent::CloseSoftwareInputPanel ||
        et == QEvent::Speech ||
        et == QEvent::Clipboard ||
        et == QEvent::MetaCall ||
        et == QEvent::ApplicationLayoutDirectionChange ||
        et == QEvent::ThreadChange)
    {
      before = lineEdit_.text();
      ok = lineEdit_.event(e);
      after = lineEdit_.text();
    }
    else
    {
#ifndef NDEBUG
      std::cerr
        << "TextInput::TPrivate::event: "
        << yae::toString(et)
        << std::endl;
#endif
    }

#ifndef NDEBUG
    std::cerr
      << "FIXME: line edit: " << lineEdit_.text().toUtf8().constData()
      << std::endl;
#endif

    // FIXME: should also check whether cursor position changed,
    //        or selection changed
    if (before != after)
    {
      this->uncache();
      return true;
    }

    return ok && e->isAccepted();
  }

  //----------------------------------------------------------------
  // TextInput::TPrivate::uncache
  //
  void
  TextInput::TPrivate::uncache()
  {
    ready_.uncache();

    YAE_OGL_11_HERE();
    YAE_OGL_11(glDeleteTextures(1, &texId_));
    texId_ = 0;
  }

  //----------------------------------------------------------------
  // TextInput::TPrivate::uploadTexture
  //
  bool
  TextInput::TPrivate::uploadTexture(const TextInput & item)
  {
    BBox bbox;
    item.get(kPropertyBBox, bbox);

    iw_ = (int)std::ceil(bbox.w_);
    ih_ = (int)std::ceil(bbox.h_);

    QImage img(iw_, ih_, QImage::Format_ARGB32);
    img.fill(0);

    QString text = lineEdit_.text();
    int textLength = text.length();
    int selStart = lineEdit_.selectionStart();
    int selLength = lineEdit_.selectedText().length();
    int cursorPos = lineEdit_.cursorPosition();

    QFont font = item.font_;
    double fontSize = item.fontSize_.get();
    font.setPointSizeF(fontSize);

    QFontMetricsF fm(font, &img);
    qreal cursorWidth = 1;

    QTextLayout textLayout(text, font, &img);
    textLayout.beginLayout();

    QTextLine textLine = textLayout.createLine();
    textLine.setNumColumns(textLength + 1);
    textLine.setPosition(QPointF(0, 0));
    qreal lineHeight = textLine.height();
    YAE_ASSERT(lineHeight <= qreal(ih_));

    QTextLine endLine = textLayout.createLine();
    YAE_ASSERT(!endLine.isValid());
    textLayout.endLayout();

    qreal cx0 = textLine.cursorToX(cursorPos);
    qreal cx1 = cx0 + cursorWidth;
    qreal x1 = offset_ + qreal(iw_);

    if (cx0 < offset_)
    {
      offset_ = std::max<qreal>(0.0, cx1 - qreal(iw_));
    }
    else if (cx1 > x1)
    {
      offset_ = cx1 - qreal(iw_);
    }

    // paint text:
    {
      QVector<QTextLayout::FormatRange> selections;
      if (selLength > 0)
      {
        selections.push_back(QTextLayout::FormatRange());
        QTextLayout::FormatRange & sel = selections.back();
        sel.start = selStart;
        sel.length = selLength;
        sel.format.setFont(font);
        sel.format.setBackground(QColor(item.selectionBg_.get()));
        sel.format.setForeground(QColor(item.selectionFg_.get()));
      }

      qreal yoffset = 0.5 * (qreal(ih_) - lineHeight);
      QPointF offset(-offset_, yoffset);
      QRectF clip(offset, QSizeF(iw_, ih_));
      QPainter painter(&img);

      const Color & color = item.color_.get();
      painter.setPen(QColor(color));
      textLayout.draw(&painter, offset, selections, clip);

      const Color & cursorColor = item.cursorColor_.get();
      painter.setPen(QColor(cursorColor));
      textLayout.drawCursor(&painter, offset, cursorPos);
    }

    bool ok = yae::uploadTexture2D(img, texId_, iw_, ih_, GL_NEAREST);
    return ok;
  }

  //----------------------------------------------------------------
  // TextInput::TPrivate::paint
  //
  void
  TextInput::TPrivate::paint(const TextInput & item)
  {
    if (!ready_.get())
    {
      YAE_ASSERT(false);
      return;
    }

    BBox bbox;
    item.get(kPropertyBBox, bbox);

    bbox.x_ = floor(bbox.x_ + 0.5);
    bbox.y_ = floor(bbox.y_ + 0.5);
    bbox.w_ = iw_;
    bbox.h_ = ih_;

    paintTexture2D(bbox, texId_, iw_, ih_);
  }

  //----------------------------------------------------------------
  // TextInput::TextInput
  //
  TextInput::TextInput(const char * id):
    Item(id),
    p_(new TextInput::TPrivate()),
    color_(ColorRef::constant(Color(0x7f7f7f, 0.5)))
  {
#if (QT_VERSION >= QT_VERSION_CHECK(4, 8, 0))
    font_.setHintingPreference(QFont::PreferFullHinting);
#endif
    font_.setStyleHint(QFont::SansSerif);
    font_.setStyleStrategy((QFont::StyleStrategy)
                           (QFont::PreferOutline |
                            QFont::PreferAntialias |
                            QFont::OpenGLCompatible));

    static bool hasImpact =
      QFontInfo(QFont("impact")).family().
      contains(QString::fromUtf8("impact"), Qt::CaseInsensitive);

    if (hasImpact)
    {
      font_.setFamily("impact");
    }
#if (QT_VERSION < QT_VERSION_CHECK(5, 0, 0)) || !defined(__APPLE__)
    else
#endif
    {
      font_.setStretch(QFont::Condensed);
      font_.setWeight(QFont::Black);
    }

    fontSize_ = ItemRef::constant(font_.pointSizeF());
    p_->ready_ = addExpr(new UploadTexture<TextInput>(*this));

    bool ok = true;
    ok = connect(&(p_->lineEdit_), SIGNAL(textEdited(const QString &)),
                 this, SIGNAL(textEdited(const QString &)));
    YAE_ASSERT(ok);

    ok = connect(&(p_->lineEdit_), SIGNAL(editingFinished()),
                 this, SIGNAL(editingFinished()));
    YAE_ASSERT(ok);
  }

  //----------------------------------------------------------------
  // TextInput::~TextInput
  //
  TextInput::~TextInput()
  {
    delete p_;
  }

  //----------------------------------------------------------------
  // TextInput::processEvent
  //
  bool
  TextInput::processEvent(Canvas::ILayer & canvasLayer,
                          Canvas * canvas,
                          QEvent * e)
  {
    return p_->processEvent(*this, canvasLayer, canvas, e);
  }

  //----------------------------------------------------------------
  // TextInput::uncache
  //
  void
  TextInput::uncache()
  {
    // FIXME: p_->uncache()?
    color_.uncache();
    fontSize_.uncache();
    p_->uncache();
    Item::uncache();
  }

  //----------------------------------------------------------------
  // TextInput::paintContent
  //
  void
  TextInput::paintContent() const
  {
    p_->paint(*this);
  }

  //----------------------------------------------------------------
  // TextInput::unpaintContent
  //
  void
  TextInput::unpaintContent() const
  {
    p_->uncache();
  }

  //----------------------------------------------------------------
  // TextInput::text
  //
  QString
  TextInput::text() const
  {
    return p_->lineEdit_.text();
  }

  //----------------------------------------------------------------
  // TextInput::setText
  //
  void
  TextInput::setText(const QString & text)
  {
    p_->lineEdit_.setText(text);
  }

}
