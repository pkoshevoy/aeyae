// -*- Mode: c++; tab-width: 8; c-basic-offset: 2; indent-tabs-mode: nil -*-
// NOTE: the first line of this file sets up source code indentation rules
// for Emacs; it is also a hint to anyone modifying this file.

// Created      : Sun Dec 20 20:13:45 PST 2015
// Copyright    : Pavel Koshevoy
// License      : MIT -- http://www.opensource.org/licenses/mit-license.php

// standard libraries:
#include <algorithm>

// Qt library:
#include <QApplication>
#include <QImage>
#include <QLineEdit>
#include <QPainter>
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
    TPrivate(const QString & text);
   ~TPrivate();

    bool processEvent(TextInput & item,
                      Canvas::ILayer & canvasLayer,
                      Canvas * canvas,
                      QEvent * e);

    void onPress(TextInput & item, const TVec2D & lcsPt);

    void onDoubleClick(TextInput & item, const TVec2D & lcsPt);

    void onDrag(TextInput & item,
                const TVec2D & lcsDragStart,
                const TVec2D & lcsDragEnd);

    void uncache();
    void layoutText(const TextInput & item);
    bool uploadTexture(const TextInput & item);
    void paint(const TextInput & item);

    QLineEdit lineEdit_;
    QTextLayout textLayout_;
    QTextLine textLine_;
    QFont font_;

    BoolRef ready_;
    qreal offset_;
    GLuint texId_;
    GLuint iw_;
    GLuint ih_;
    GLuint downsample_;
    int cursorDragStart_;

    // optional id of focus proxy item that manages this text input;
    const Item * proxy_;
  };

  //----------------------------------------------------------------
  // TextInput::TPrivate::TPrivate
  //
  TextInput::TPrivate::TPrivate(const QString & text):
    offset_(0),
    texId_(0),
    iw_(0),
    ih_(0),
    downsample_(1),
    cursorDragStart_(0),
    proxy_(NULL)
  {
    lineEdit_.hide();
    lineEdit_.setText(text);
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

    if (et == QEvent::KeyPress ||
        et == QEvent::KeyRelease)
    {
      QKeyEvent & ke = *(static_cast<QKeyEvent *>(e));
      if (ke.key() == Qt::Key_Escape)
      {
        if (et == QEvent::KeyRelease)
        {
          ItemFocus::singleton().clearFocus();
        }

        return true;
      }
    }

    bool ok = false;

    int beforeCursor = -1;
    int beforeSelStart = -1;
    QString beforeSelection;
    QString beforeText;

    int afterCursor = -1;
    int afterSelStart = -1;
    QString afterSelection;
    QString afterText;

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
      beforeText = lineEdit_.text();
      beforeCursor = lineEdit_.cursorPosition();
      beforeSelStart = lineEdit_.selectionStart();
      beforeSelection = lineEdit_.selectedText();

      ok = lineEdit_.event(e);

      afterText = lineEdit_.text();
      afterCursor = lineEdit_.cursorPosition();
      afterSelStart = lineEdit_.selectionStart();
      afterSelection = lineEdit_.selectedText();
    }
#if 0 // ndef NDEBUG
    else
    {
      yae_debug << "TextInput::TPrivate::event: " << yae::toString(et);
    }

    yae_debug << "FIXME: line edit: " << lineEdit_.text().toUtf8().constData();
#endif

    if (beforeText != afterText ||
        beforeCursor != afterCursor ||
        beforeSelStart != afterSelStart ||
        beforeSelection != afterSelection)
    {
      item.uncache();
      return true;
    }

    return ok && e->isAccepted();
  }

  //----------------------------------------------------------------
  // TextInput::TPrivate::onPress
  //
  void
  TextInput::TPrivate::onPress(TextInput & item, const TVec2D & lcsPt)
  {
    if (!textLine_.isValid())
    {
      return;
    }

    // update cursor position here:
    double supersample = item.supersample_.get();
    cursorDragStart_ = textLine_.xToCursor(lcsPt.x() * supersample + offset_,
                                           QTextLine::CursorBetweenCharacters);
    lineEdit_.setCursorPosition(cursorDragStart_);
    lineEdit_.setSelection(cursorDragStart_, 0);
    item.uncache();
  }

  //----------------------------------------------------------------
  // TextInput::TPrivate::onDoubleClick
  //
  void
  TextInput::TPrivate::onDoubleClick(TextInput & item, const TVec2D & lcsPt)
  {
    // select all:
    int selLength = lineEdit_.text().length();
    lineEdit_.setSelection(0, selLength);
    item.uncache();
  }

  //----------------------------------------------------------------
  // TextInput::TPrivate::onDrag
  //
  void
  TextInput::TPrivate::onDrag(TextInput & item,
                              const TVec2D & lcsDragStart,
                              const TVec2D & lcsDragEnd)
  {
    if (!textLine_.isValid())
    {
      return;
    }

    // update selection here:
    double supersample = item.supersample_.get();

    double x1 = offset_ + supersample * lcsDragEnd.x();
    int selEnd = textLine_.xToCursor(x1, QTextLine::CursorBetweenCharacters);

    int selLength = selEnd - cursorDragStart_;
    if (!selLength)
    {
      return;
    }

    lineEdit_.setSelection(cursorDragStart_, selLength);
    item.uncache();
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
  // TextInput::TPrivate::layoutText
  //
  void
  TextInput::TPrivate::layoutText(const TextInput & item)
  {
    BBox bbox;
    item.Item::get(kPropertyBBox, bbox);

    double supersample = item.supersample_.get();
    iw_ = (int)std::ceil(bbox.w_ * supersample);
    ih_ = (int)std::ceil(bbox.h_ * supersample);

    QString text = lineEdit_.text();
    int textLength = text.length();

    font_ = item.font_;
    double fontSize = std::max(9.0, item.fontSize_.get());
    font_.setPixelSize(fontSize * supersample);

    textLayout_.clearLayout();
    textLayout_.setText(text);
    textLayout_.setFont(font_);
    textLayout_.beginLayout();

    textLine_ = textLayout_.createLine();
    textLine_.setNumColumns(textLength);
    textLine_.setPosition(QPointF(0, 0));

    QTextLine endLine = textLayout_.createLine();
    YAE_ASSERT(!endLine.isValid());
    textLayout_.endLayout();
  }

  //----------------------------------------------------------------
  // addFormatRange
  //
  static void
  addFormatRange(QVector<QTextLayout::FormatRange> & ranges,
                 int start,
                 int length,
                 const QFont & font,
                 const Color & fg,
                 const Color & bg)
  {
    if (length < 1)
    {
      return;
    }

    ranges.push_back(QTextLayout::FormatRange());
    QTextLayout::FormatRange & range = ranges.back();
    range.start = start;
    range.length = length;
    range.format.setFont(font);
    range.format.setBackground(QColor(bg));
    range.format.setForeground(QColor(fg));
  }

  //----------------------------------------------------------------
  // TextInput::TPrivate::uploadTexture
  //
  bool
  TextInput::TPrivate::uploadTexture(const TextInput & item)
  {
    layoutText(item);

    double supersample = item.supersample_.get();
    int selStart = lineEdit_.selectionStart();
    int selLength = lineEdit_.selectedText().length();
    int selEnd = (selLength < 1) ? 0 : selStart + selLength;
    int textLen = lineEdit_.text().length();
    int cursorPos = lineEdit_.cursorPosition();
    int cursorWidth = int(ceil(std::max<double>(1.0, supersample *
                                                item.cursorWidth_.get())));

    qreal cx0 = textLine_.cursorToX(cursorPos);
    qreal cx1 = cx0 + cursorWidth;
    qreal x1 = offset_ + qreal(iw_);

    if (cx0 < offset_)
    {
      offset_ = cx0;
    }
    else if (cx1 > x1)
    {
      offset_ = cx1 - qreal(iw_);
    }

    GLsizei widthPowerOfTwo = powerOfTwoGEQ<GLsizei>(iw_);
    GLsizei heightPowerOfTwo = powerOfTwoGEQ<GLsizei>(ih_);
    QImage img(widthPowerOfTwo, heightPowerOfTwo, QImage::Format_ARGB32);
    {
      const Color & fg = item.color_.get();

#if (QT_VERSION < QT_VERSION_CHECK(5, 0, 0)) && defined(__APPLE__)
      Color bg = fg.transparent();
#else
      const Color & bg = item.background_.get();
#endif
      img.fill(QColor(bg).rgba());

      const Color & selFg = item.selectionFg_.get();
      const Color & selBg = item.selectionBg_.get();

      QVector<QTextLayout::FormatRange> ranges;
      addFormatRange(ranges, 0, selStart, font_, fg, bg);
      addFormatRange(ranges, selStart, selLength, font_, selFg, selBg);
      addFormatRange(ranges, selEnd, textLen - selEnd, font_, fg, bg);

      qreal lineHeight = textLine_.height();
      YAE_ASSERT(lineHeight <= qreal(ih_));

      qreal yoffset = 0.5 * (qreal(ih_) - lineHeight);
      QPointF offset(-offset_, yoffset);

      QPainter painter(&img);
      painter.setRenderHints(QPainter::TextAntialiasing);
      textLayout_.draw(&painter, offset, ranges);

      QColor cursorColor(item.cursorColor_.get());
      QPen cursorPen(cursorColor);
      painter.setPen(cursorPen);

      // It appears Qt 5.9.3+ QTextLayout::drawCursor uses
      // QPainter::RasterOp_NotDestination composition mode,
      // which doesn't work as expected with a custom cursor color:
#if 0
      textLayout_.drawCursor(&painter, offset, cursorPos, cursorWidth);
#else
      if (cursorWidth > 1)
      {
        painter.setCompositionMode(QPainter::CompositionMode_Multiply);
      }
      else
      {
        painter.setCompositionMode(QPainter::CompositionMode_SourceOver);
      }

      painter.fillRect(cx0 - offset_,
                       -yoffset,
                       cx1 - cx0,
                       lineHeight,
                       cursorPen.brush());
#endif
    }

    // do not upload supersampled texture at full size, scale down first:
    downsample_ = downsampleImage(img, supersample);

    bool ok = yae::uploadTexture2D(img, texId_,
                                   supersample == 1.0 ?
                                   GL_NEAREST : GL_LINEAR_MIPMAP_LINEAR);
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
    item.Item::get(kPropertyBBox, bbox);

    bbox.x_ = floor(bbox.x_ + 0.5);
    bbox.y_ = floor(bbox.y_ + 0.5);

    double supersample = item.supersample_.get();
    bbox.w_ = double(iw_) / supersample;
    bbox.h_ = double(ih_) / supersample;

    int iw = (iw_ + downsample_ - 1) / downsample_;
    int ih = (ih_ + downsample_ - 1) / downsample_;
    double opacity = item.opacity_.get();
    paintTexture2D(bbox, texId_, iw, ih, opacity);
  }

  //----------------------------------------------------------------
  // TextInput::TextInput
  //
  TextInput::TextInput(const char * id, const QString & text):
    Item(id),
    p_(new TextInput::TPrivate(text)),
    opacity_(ItemRef::constant(1.0)),
    color_(ColorRef::constant(Color(0x7f7f7f, 0.5))),
    background_(ColorRef::constant(Color(0x000000, 0.0)))
  {
    fontSize_ = ItemRef::constant(font_.pointSizeF());
    supersample_ = ItemRef::constant(1.0);
    p_->ready_ = addExpr(new UploadTexture<TextInput>(*this));

    cursorWidth_ = ItemRef::constant(1);

    bool ok = true;
    ok = connect(&(p_->lineEdit_), SIGNAL(textChanged(const QString &)),
                 this, SIGNAL(textChanged(const QString &)));
    YAE_ASSERT(ok);

    ok = connect(&(p_->lineEdit_), SIGNAL(textEdited(const QString &)),
                 this, SIGNAL(textEdited(const QString &)));
    YAE_ASSERT(ok);

    ok = connect(&(p_->lineEdit_), SIGNAL(editingFinished()),
                 this, SLOT(onEditingFinished()));
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
  // TextInput::get
  //
  void
  TextInput::get(Property property, double & value) const
  {
    if (property == kPropertyCursorWidth)
    {
      value = cursorWidth_.get();
    }
    else if (property == kPropertyOpacity)
    {
      value = opacity_.get();
    }
    else
    {
      Item::get(property, value);
    }
  }

  //----------------------------------------------------------------
  // TextInput::get
  //
  void
  TextInput::get(Property property, bool & value) const
  {
    if (property == kPropertyHasText)
    {
      value = !(text().isEmpty());
    }
    else
    {
      Item::get(property, value);
    }
  }

  //----------------------------------------------------------------
  // TextInput::get
  //
  void
  TextInput::get(Property property, Color & value) const
  {
    if (property == kPropertyColor)
    {
      value = color_.get();
    }
    else if (property == kPropertyColorBg)
    {
      value = background_.get();
    }
    else if (property == kPropertyColorCursor)
    {
      value = cursorColor_.get();
    }
    else if (property == kPropertyColorSelFg)
    {
      value = selectionFg_.get();
    }
    else if (property == kPropertyColorSelBg)
    {
      value = selectionBg_.get();
    }
    else
    {
      Item::get(property, value);
    }
  }

  //----------------------------------------------------------------
  // TextInput::get
  //
  void
  TextInput::get(Property property, TVar & value) const
  {
    if (property == kPropertyText)
    {
      value = TVar(text());
    }
    else
    {
      Item::get(property, value);
    }
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
  // TextInput::onPress
  //
  void
  TextInput::onPress(const TVec2D & lcsPt)
  {
    p_->onPress(*this, lcsPt);
  }

  //----------------------------------------------------------------
  // TextInput::onDoubleClick
  //
  void
  TextInput::onDoubleClick(const TVec2D & lcsPt)
  {
    p_->onDoubleClick(*this, lcsPt);
  }

  //----------------------------------------------------------------
  // TextInput::onDrag
  //
  void
  TextInput::onDrag(const TVec2D & lcsDragStart, const TVec2D & lcsDragEnd)
  {
    p_->onDrag(*this, lcsDragStart, lcsDragEnd);
  }

  //----------------------------------------------------------------
  // TextInput::uncache
  //
  void
  TextInput::uncache()
  {
    fontSize_.uncache();
    supersample_.uncache();
    cursorWidth_.uncache();
    opacity_.uncache();
    color_.uncache();
    background_.uncache();
    cursorColor_.uncache();
    selectionFg_.uncache();
    selectionBg_.uncache();
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
  // TextInput::visible
  //
  bool
  TextInput::visible() const
  {
    return Item::visible() && opacity_.get() > 0.0;
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
    p_->layoutText(*this);
  }

  //----------------------------------------------------------------
  // TextInput::setFocusProxy
  //
  void
  TextInput::setFocusProxy(const Item * proxy)
  {
    p_->proxy_ = proxy;
  }

  //----------------------------------------------------------------
  // TextInput::onEditingFinished
  //
  void
  TextInput::onEditingFinished()
  {
    QString payload = text();
    emit editingFinished(payload);

    if (p_->proxy_)
    {
      ItemFocus::singleton().clearFocus(p_->proxy_);
    }
  }


  //----------------------------------------------------------------
  // TextInputProxy::TextInputProxy
  //
  TextInputProxy::TextInputProxy(const char * id,
                                 Text & view,
                                 TextInput & edit):
    InputArea(id),
    view_(view),
    edit_(edit),
    copyViewToEdit_(BoolRef::constant(false)),
    editingFinishedOnFocusOut_(BoolRef::constant(false))
  {
    edit_.setFocusProxy(this);
    placeholder_ = TVarRef::constant(TVar(QObject::tr("")));
  }

  //----------------------------------------------------------------
  // TextInputProxy::~TextInputProxy
  //
  TextInputProxy::~TextInputProxy()
  {
    ItemFocus::singleton().removeFocusable(this);
  }

  //----------------------------------------------------------------
  // TextInputProxy::uncache
  //
  void
  TextInputProxy::uncache()
  {
    bgNoFocus_.uncache();
    bgOnFocus_.uncache();
    placeholder_.uncache();
    copyViewToEdit_.uncache();
    editingFinishedOnFocusOut_.uncache();
    InputArea::uncache();
  }

  //----------------------------------------------------------------
  // TextInputProxy::onPress
  //
  bool
  TextInputProxy::onPress(const TVec2D & itemCSysOrigin,
                          const TVec2D & rootCSysPoint)
  {
    if (!ItemFocus::singleton().hasFocus(this))
    {
      ItemFocus::singleton().setFocus(this);
    }

    TVec2D originEdit(edit_.left(), edit_.top());
    TVec2D lcsPt = (rootCSysPoint - itemCSysOrigin) - originEdit;
    edit_.onPress(lcsPt);
    return true;
  }

  //----------------------------------------------------------------
  // TextInputProxy::onDoubleClick
  //
  bool
  TextInputProxy::onDoubleClick(const TVec2D & itemCSysOrigin,
                                const TVec2D & rootCSysPoint)
  {
    TVec2D originEdit(edit_.left(), edit_.top());
    TVec2D lcsPt = (rootCSysPoint - itemCSysOrigin) - originEdit;
    edit_.onDoubleClick(lcsPt);
    return true;
  }

  //----------------------------------------------------------------
  // TextInputProxy::onDrag
  //
  bool
  TextInputProxy::onDrag(const TVec2D & itemCSysOrigin,
                         const TVec2D & rootCSysDragStart,
                         const TVec2D & rootCSysDragEnd)
  {
    TVec2D originEdit(edit_.left(), edit_.top());
    TVec2D lcsDragStart = (rootCSysDragStart - itemCSysOrigin) - originEdit;
    TVec2D lcsDragEnd = (rootCSysDragEnd - itemCSysOrigin) - originEdit;
    edit_.onDrag(lcsDragStart, lcsDragEnd);
    return true;
  }

  //----------------------------------------------------------------
  // TextInputProxy::onFocus
  //
  void
  TextInputProxy::onFocus()
  {
    if (copyViewToEdit_.get())
    {
      edit_.setText(view_.text());
    }

    view_.uncache();
    edit_.uncache();
    edit_.onFocus();
    InputArea::onFocus();
  }

  //----------------------------------------------------------------
  // TextInputProxy::onFocusOut
  //
  void
  TextInputProxy::onFocusOut()
  {
    if (editingFinishedOnFocusOut_.get())
    {
      edit_.onEditingFinished();
    }

    view_.uncache();
    edit_.uncache();
    edit_.onFocusOut();
    InputArea::onFocusOut();
  }

  //----------------------------------------------------------------
  // TextInputProxy::processEvent
  //
  bool
  TextInputProxy::processEvent(Canvas::ILayer & canvasLayer,
                               Canvas * canvas,
                               QEvent * event)
  {
    TMakeCurrentContext currentContext(*canvasLayer.context());
    return edit_.processEvent(canvasLayer, canvas, event);
  }

  //----------------------------------------------------------------
  // TextInputProxy::get
  //
  void
  TextInputProxy::get(Property property, bool & value) const
  {
    if (property == kPropertyHasText)
    {
      value = !(edit_.text().isEmpty());
    }
    else
    {
      InputArea::get(property, value);
    }
  }

  //----------------------------------------------------------------
  // TextInputProxy::get
  //
  void
  TextInputProxy::get(Property property, Color & value) const
  {
    if (property == kPropertyColorNoFocusBg)
    {
      value = bgNoFocus_.get();
    }
    else if (property == kPropertyColorOnFocusBg)
    {
      value = bgOnFocus_.get();
    }
    else
    {
      InputArea::get(property, value);
    }
  }

  //----------------------------------------------------------------
  // TextInputProxy::get
  //
  void
  TextInputProxy::get(Property property, TVar & value) const
  {
    if (property == kPropertyText)
    {
      QString text = edit_.text();

      if (text.isEmpty())
      {
        text = placeholder_.get().toString();
      }

      value = TVar(text);
    }
    else
    {
      InputArea::get(property, value);
    }
  }

}
