// -*- Mode: c++; tab-width: 8; c-basic-offset: 2; indent-tabs-mode: nil -*-
// NOTE: the first line of this file sets up source code indentation rules
// for Emacs; it is also a hint to anyone modifying this file.

// Created      : Sun Dec 20 20:13:45 PST 2015
// Copyright    : Pavel Koshevoy
// License      : MIT -- http://www.opensource.org/licenses/mit-license.php

#ifndef YAE_TEXT_INPUT_H_
#define YAE_TEXT_INPUT_H_

// aeyae:
#include "yae/api/yae_api.h"

// Qt:
#include <QEvent>
#include <QFont>
#include <QObject>

// yaeui:
#include "yaeInputArea.h"
#include "yaeItem.h"
#include "yaeText.h"


namespace yae
{

  //----------------------------------------------------------------
  // TextInput
  //
  class TextInput : public QObject, public Item
  {
    Q_OBJECT;

    TextInput(const TextInput &);
    TextInput & operator = (const TextInput &);

  public:
    TextInput(const char * id, const QString & text = QString());
    ~TextInput();

    // virtual:
    void get(Property property, double & value) const;

    // virtual:
    void get(Property property, bool & value) const;

    // virtual:
    void get(Property property, Color & value) const;

    // virtual:
    void get(Property property, TVar & value) const;

    // virtual:
    bool processEvent(Canvas::ILayer & canvasLayer,
                      Canvas * canvas,
                      QEvent * event);

    // helpers for use with focus proxy:
    void onPress(const TVec2D & lcsClickPt);
    void onDoubleClick(const TVec2D & lcsClickPt);
    void onDrag(const TVec2D & lcsDragStart,
                const TVec2D & lcsDragEnd);

    // virtual:
    void uncache();

    // virtual:
    void paintContent() const;
    void unpaintContent() const;

    // virtual:
    bool visible() const;

    // accessors to current text payload:
    QString text() const;
    void setText(const QString & text);

    // when finished editing it is better to clear focus
    // which most likely belongs to a proxy:
    void setFocusProxy(const Item * proxy);

  signals:
    void textChanged(const QString & text);
    void textEdited(const QString & text);
    void editingFinished(const QString & text);

  public slots:
    void onEditingFinished();

  public:
    struct TPrivate;
    TPrivate * p_;

    QFont font_;
    ItemRef fontSize_; // in points
    ItemRef supersample_;
    ItemRef cursorWidth_; // in pixels
    ItemRef opacity_;

    ColorRef color_;
    ColorRef background_;
    ColorRef cursorColor_;
    ColorRef selectionFg_;
    ColorRef selectionBg_;
  };


  //----------------------------------------------------------------
  // TextInputProxy
  //
  struct TextInputProxy : public InputArea
  {
    TextInputProxy(const char * id, Text & view, TextInput & edit);
    ~TextInputProxy();

    // virtual:
    void uncache();

    // virtual:
    bool onPress(const TVec2D & itemCSysOrigin,
                 const TVec2D & rootCSysPoint);

    // virtual:
    bool onDoubleClick(const TVec2D & itemCSysOrigin,
                       const TVec2D & rootCSysPoint);

    // virtual:
    bool onDrag(const TVec2D & itemCSysOrigin,
                const TVec2D & rootCSysDragStart,
                const TVec2D & rootCSysDragEnd);

    // virtual:
    void onFocus();

    // virtual:
    void onFocusOut();

    // virtual:
    bool processEvent(Canvas::ILayer & canvasLayer,
                      Canvas * canvas,
                      QEvent * event);

    // virtual:
    void get(Property property, bool & value) const;

    // virtual:
    void get(Property property, Color & value) const;

    // virtual:
    void get(Property property, TVar & value) const;

    Text & view_;
    TextInput & edit_;

    ColorRef bgNoFocus_;
    ColorRef bgOnFocus_;
    TVarRef placeholder_;
    BoolRef copyViewToEdit_;
    BoolRef editingFinishedOnFocusOut_;
  };

  //----------------------------------------------------------------
  // ClearTextInput
  //
  struct ClearTextInput : public InputArea
  {
    ClearTextInput(const char * id, TextInput & edit, Text & view):
      InputArea(id),
      edit_(edit),
      view_(view)
    {}

    // virtual:
    bool onPress(const TVec2D & itemCSysOrigin,
                 const TVec2D & rootCSysPoint)
    { return true; }

    // virtual:
    bool onClick(const TVec2D & itemCSysOrigin,
                 const TVec2D & rootCSysPoint)
    {
      edit_.setText(QString());
      edit_.uncache();
      view_.uncache();
      return true;
    }

    TextInput & edit_;
    Text & view_;
  };

}


#endif // YAE_TEXT_INPUT_H_
