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

    // optional id of focus proxy item that manages this text input;
    std::string proxyId_;
  };

  //----------------------------------------------------------------
  // TextInput::TPrivate::TPrivate
  //
  TextInput::TPrivate::TPrivate():
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
      std::cerr
        << "TextInput::TPrivate::event: "
        << yae::toString(et)
        << std::endl;
    }

    std::cerr
      << "FIXME: line edit: " << lineEdit_.text().toUtf8().constData()
      << std::endl;
#endif

    if (beforeText != afterText ||
        beforeCursor != afterCursor ||
        beforeSelStart != afterSelStart ||
        beforeSelection != afterSelection)
    {
      this->uncache();
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
    int cursorPos = textLine_.xToCursor(lcsPt.x() + offset_,
                                        QTextLine::CursorBetweenCharacters);
    lineEdit_.setCursorPosition(cursorPos);
    lineEdit_.setSelection(cursorPos, 0);
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
    int selStart = textLine_.xToCursor(lcsDragStart.x() + offset_,
                                       QTextLine::CursorBetweenCharacters);
    int selEnd = textLine_.xToCursor(lcsDragEnd.x() + offset_,
                                     QTextLine::CursorBetweenCharacters);
    if (selStart > selEnd)
    {
      std::swap(selStart, selEnd);
    }

    int selLength = selEnd - selStart;
    if (selLength < 1)
    {
      return;
    }

    lineEdit_.setSelection(selStart, selLength);
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

    iw_ = (int)std::ceil(bbox.w_);
    ih_ = (int)std::ceil(bbox.h_);

    QString text = lineEdit_.text();
    int textLength = text.length();

    font_ = item.font_;
    double fontSize = item.fontSize_.get();
    font_.setPointSizeF(fontSize);

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
  // TextInput::TPrivate::uploadTexture
  //
  bool
  TextInput::TPrivate::uploadTexture(const TextInput & item)
  {
    layoutText(item);

    int selStart = lineEdit_.selectionStart();
    int selLength = lineEdit_.selectedText().length();
    int cursorPos = lineEdit_.cursorPosition();
    int cursorWidth = std::max<int>(1, int(ceil(item.cursorWidth_.get())));

    qreal cx0 = textLine_.cursorToX(cursorPos);
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

    QImage img(iw_, ih_, QImage::Format_ARGB32);
    img.fill(0);

    QVector<QTextLayout::FormatRange> selections;
    if (selLength > 0)
    {
      selections.push_back(QTextLayout::FormatRange());
      QTextLayout::FormatRange & sel = selections.back();
      sel.start = selStart;
      sel.length = selLength;
      sel.format.setFont(font_);
      sel.format.setBackground(QColor(item.selectionBg_.get()));
      sel.format.setForeground(QColor(item.selectionFg_.get()));
    }

    qreal lineHeight = textLine_.height();
    YAE_ASSERT(lineHeight <= qreal(ih_));

    qreal yoffset = 0.5 * (qreal(ih_) - lineHeight);
    QPointF offset(-offset_, yoffset);
    QRectF clip(offset, QSizeF(iw_, ih_));
    QPainter painter(&img);

    const Color & color = item.color_.get();
    painter.setPen(QColor(color));
    textLayout_.draw(&painter, offset, selections, clip);

    const Color & cursorColor = item.cursorColor_.get();
    painter.setPen(QColor(cursorColor));
    textLayout_.drawCursor(&painter, offset, cursorPos, cursorWidth);

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
    item.Item::get(kPropertyBBox, bbox);

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

    cursorWidth_ = ItemRef::constant(1);

    bool ok = true;
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
    cursorWidth_.uncache();
    color_.uncache();
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
  // TextInput::setFocusProxyId
  //
  void
  TextInput::setFocusProxyId(const std::string & proxyId)
  {
    p_->proxyId_ = proxyId;
  }

  //----------------------------------------------------------------
  // TextInput::onEditingFinished
  //
  void
  TextInput::onEditingFinished()
  {
    QString payload = text();
    emit editingFinished(payload);

    if (!p_->proxyId_.empty())
    {
      ItemFocus::singleton().clearFocus(p_->proxyId_);
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
    copyViewToEdit_(false)
  {
    edit_.setFocusProxyId(id_);
    placeholder_ = TVarRef::constant(TVar(QObject::tr("")));
  }

  //----------------------------------------------------------------
  // TextInputProxy::uncache
  //
  void
  TextInputProxy::uncache()
  {
    placeholder_.uncache();
    InputArea::uncache();
  }

  //----------------------------------------------------------------
  // TextInputProxy::onPress
  //
  bool
  TextInputProxy::onPress(const TVec2D & itemCSysOrigin,
                          const TVec2D & rootCSysPoint)
  {
    if (!ItemFocus::singleton().hasFocus(id_))
    {
      ItemFocus::singleton().setFocus(id_);
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
    if (copyViewToEdit_)
    {
      edit_.setText(view_.text());
    }

    view_.uncache();
    edit_.uncache();
    edit_.onFocus();
  }

  //----------------------------------------------------------------
  // TextInputProxy::onFocusOut
  //
  void
  TextInputProxy::onFocusOut()
  {
    view_.uncache();
    edit_.uncache();
    edit_.onFocusOut();
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
