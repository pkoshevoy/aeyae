/*
Copyright 2004-2007 University of Utah

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
THE SOFTWARE.
*/


// File         : the_qt_trail.cxx
// Author       : Pavel Aleksandrovich Koshevoy
// Created      : 2001/06/24 16:47:24
// Copyright    : (C) 2001, 2002, 2003, 2004
// License      : MIT
// Description  : An event trail recoring/playback mechanism used
//                for regression testing and debugging.

// local includes:
#include "Qt/the_qt_trail.hxx"
#include "Qt/the_qt_input_device_event.hxx"
#include "utils/the_utils.hxx"
#include "utils/the_text.hxx"
#include "io/io_base.hxx"
#include "utils/instance_method_call.hxx"

// Qt includes:
#include <QtGlobal>
#include <QtCore>
#include <QApplication>
#include <QWidget>
#include <QObject>
#include <QDir>
#include <QString>
#include <QMessageBox>
#include <QWheelEvent>
#include <QCloseEvent>
#include <QMoveEvent>
#include <QEvent>
#include <QKeyEvent>
#include <QTimerEvent>
#include <QResizeEvent>
#include <QTabletEvent>
#include <QMouseEvent>
#include <QThread>
#include <QTime>
#include <QPushButton>
#include <QSize>
#include <QRect>

// system includes:
#include <iostream>
#include <sstream>
#include <string>
#include <list>
#include <assert.h>
#include <stdlib.h>
#include <algorithm>

// namespace stuff:
using std::istream;
using std::ostream;
using std::ifstream;
using std::ofstream;


//----------------------------------------------------------------
// operator >>
// 
static istream &
operator >> (istream & in, QString & str)
{
  std::string tmp;
  in >> tmp;
  str = QString::fromUtf8(tmp.c_str());
  return in;
}

//----------------------------------------------------------------
// operator <<
// 
static ostream &
operator << (ostream & out, const QString & str)
{
  std::string tmp(str.toUtf8().constData());
  out << tmp.c_str();
  return out;
}

//----------------------------------------------------------------
// qevent_type_to_str
// 
// helper function:
static const char *
qevent_type_to_str(QEvent::Type event_type)
{
  switch (event_type)
  {
    case QEvent::None:
      return "None";

    case QEvent::Timer:
      return "Timer";

    case QEvent::MouseButtonPress:
      return "MouseButtonPress";

    case QEvent::MouseButtonRelease:
      return "MouseButtonRelease";

    case QEvent::MouseButtonDblClick:
      return "MouseButtonDblClick";

    case QEvent::MouseMove:
      return "MouseMove";

    case QEvent::KeyPress:
      return "KeyPress";

    case QEvent::KeyRelease:
      return "KeyRelease";

    case QEvent::FocusIn:
      return "FocusIn";

    case QEvent::FocusOut:
      return "FocusOut";

    case QEvent::Enter:
      return "Enter";

    case QEvent::Leave:
      return "Leave";

    case QEvent::Paint:
      return "Paint";

    case QEvent::Move:
      return "Move";

    case QEvent::Resize:
      return "Resize";

    case QEvent::Create:
      return "Create";

    case QEvent::Destroy:
      return "Destroy";

    case QEvent::Show:
      return "Show";

    case QEvent::Hide:
      return "Hide";

    case QEvent::Close:
      return "Close";

    case QEvent::Quit:
      return "Quit";

    case QEvent::ParentChange:
      return "ParentChange";

    case QEvent::ParentAboutToChange:
      return "ParentAboutToChange";

    case QEvent::ThreadChange:
      return "ThreadChange";

    case QEvent::WindowActivate:
      return "WindowActivate";

    case QEvent::WindowDeactivate:
      return "WindowDeactivate";
      
    case QEvent::ShowToParent:
      return "ShowToParent";
      
    case QEvent::HideToParent:
      return "HideToParent";

    case QEvent::Wheel:
      return "Wheel";

    case QEvent::WindowTitleChange:
      return "WindowTitleChange";

    case QEvent::WindowIconChange:
      return "WindowIconChange";

    case QEvent::ApplicationWindowIconChange:
      return "ApplicationWindowIconChange";

    case QEvent::ApplicationFontChange:
      return "ApplicationFontChange";

    case QEvent::ApplicationLayoutDirectionChange:
      return "ApplicationLayoutDirectionChange";

    case QEvent::ApplicationPaletteChange:
      return "ApplicationPaletteChange";

    case QEvent::PaletteChange:
      return "PaletteChange";

    case QEvent::Clipboard:
      return "Clipboard";

    case QEvent::Speech:
      return "Speech";

    case QEvent::MetaCall:
      return "MetaCall";

    case QEvent::SockAct:
      return "SockAct";

    case QEvent::WinEventAct:
      return "WinEventAct";

    case QEvent::DeferredDelete:
      return "DeferredDelete";

    case QEvent::DragEnter:
      return "DragEnter";

    case QEvent::DragMove:
      return "DragMove";

    case QEvent::DragLeave:
      return "DragLeave";
      
    case QEvent::Drop:
      return "Drop";

    case QEvent::DragResponse:
      return "DragResponse";

    case QEvent::ChildAdded:
      return "ChildAdded";

    case QEvent::ChildPolished:
      return "ChildPolished";

    case 70:
      return "ChildInserted";

    case QEvent::ChildRemoved:
      return "ChildRemoved";

    case QEvent::ShowWindowRequest:
      return "ShowWindowRequest";

    case QEvent::PolishRequest:
      return "PolishRequest";

    case QEvent::Polish:
      return "Polish";

    case QEvent::LayoutRequest:
      return "LayoutRequest";

    case QEvent::UpdateRequest:
      return "UpdateRequest";

    case QEvent::UpdateLater:
      return "UpdateLater";

    case QEvent::EmbeddingControl:
      return "EmbeddingControl";

    case QEvent::ActivateControl:
      return "ActivateControl";

    case QEvent::DeactivateControl:
      return "DeactivateControl";

    case QEvent::ContextMenu:
      return "ContextMenu";

    case QEvent::InputMethod:
      return "InputMethod";

    case QEvent::AccessibilityPrepare:
      return "AccessibilityPrepare";

    case QEvent::TabletMove:
      return "TabletMove";

    case QEvent::LocaleChange:
      return "LocaleChange";

    case QEvent::LanguageChange:
      return "LanguageChange";

    case QEvent::LayoutDirectionChange:
      return "LayoutDirectionChange";

    case QEvent::Style:
      return "Style";

    case QEvent::TabletPress:
      return "TabletPress";

    case QEvent::TabletRelease:
      return "TabletRelease";

    case QEvent::OkRequest:
      return "OkRequest";

    case QEvent::HelpRequest:
      return "HelpRequest";

    case QEvent::IconDrag:
      return "IconDrag";

    case QEvent::FontChange:
      return "FontChange";

    case QEvent::EnabledChange:
      return "EnabledChange";

    case QEvent::ActivationChange:
      return "ActivationChange";

    case QEvent::StyleChange:
      return "StyleChange";

    case QEvent::IconTextChange:
      return "IconTextChange";

    case QEvent::ModifiedChange:
      return "ModifiedChange";

    case QEvent::MouseTrackingChange:
      return "MouseTrackingChange";

    case QEvent::WindowBlocked:
      return "WindowBlocked";

    case QEvent::WindowUnblocked:
      return "WindowUnblocked";

    case QEvent::WindowStateChange:
      return "WindowStateChange";

    case QEvent::ToolTip:
      return "ToolTip";

    case QEvent::WhatsThis:
      return "WhatsThis";

    case QEvent::StatusTip:
      return "StatusTip";

    case QEvent::ActionChanged:
      return "ActionChanged";

    case QEvent::ActionAdded:
      return "ActionAdded";

    case QEvent::ActionRemoved:
      return "ActionRemoved";

    case QEvent::FileOpen:
      return "FileOpen";

    case QEvent::Shortcut:
      return "Shortcut";

    case QEvent::ShortcutOverride:
      return "ShortcutOverride";

    case QEvent::WhatsThisClicked:
      return "WhatsThisClicked";

    case QEvent::ToolBarChange:
      return "ToolBarChange";

    case QEvent::ApplicationActivated:
      return "ApplicationActivated";

    case QEvent::ApplicationDeactivated:
      return "ApplicationDeactivated";

    case QEvent::QueryWhatsThis:
      return "QueryWhatsThis";

    case QEvent::EnterWhatsThisMode:
      return "EnterWhatsThisMode";

    case QEvent::LeaveWhatsThisMode:
      return "LeaveWhatsThisMode";

    case QEvent::ZOrderChange:
      return "ZOrderChange";

    case QEvent::HoverEnter:
      return "HoverEnter";

    case QEvent::HoverLeave:
      return "HoverLeave";

    case QEvent::HoverMove:
      return "HoverMove";

    case QEvent::AccessibilityHelp:
      return "AccessibilityHelp";

    case QEvent::AccessibilityDescription:
      return "AccessibilityDescription";
      
    case QEvent::AcceptDropsChange:
      return "AcceptDropsChange";

    case QEvent::MenubarUpdated:
      return "MenubarUpdated";

    case QEvent::ZeroTimerEvent:
      return "ZeroTimerEvent";

    case QEvent::User:
      return "User";
      
    case QEvent::MaxUser:
      return "MaxUser";
      
    default:
      break;
  }
  
  static char buffer[256];
#ifdef WIN32
#define snprintf _snprintf
#endif
  snprintf(buffer, 256, "%i", event_type);
  return buffer;
}


#if 0
//----------------------------------------------------------------
// dump_children
// 
static void
dump_children(QObject * parent)
{
  const QObjectList & children = parent->children();
  for (QObjectList::const_iterator i = children.begin();
       i != children.end(); ++i)
  {
    QObject * obj = *i;
    cerr << parent->objectName() << " -> " << obj->objectName() << endl;
  }
}
#endif


//----------------------------------------------------------------
// dump_children_tree
// 
static void
dump_children_tree(ostream & so, const QObject * parent, unsigned int indent)
{
  for (unsigned int i = 0; i < indent; i++) so << ' ';
  so << parent->metaObject()->className() << ':' << parent->objectName();
  if (parent->isWidgetType())
  {
    const QWidget * widget = static_cast<const QWidget *>(parent);
    so << ' ' << widget->x() << ':' << widget->y()
       << ':' << widget->width() << ':' << widget->height();
  }
  so << endl;
  
  const QObjectList & children = parent->children();
  for (QObjectList::const_iterator i = children.begin();
       i != children.end(); ++i)
  {
    const QObject * obj = *i;
    dump_children_tree(so, obj, indent + 2);
  }
}

//----------------------------------------------------------------
// encode_special_chars
// 
static const QString
encode_special_chars(const QString & text,
		     const char * special_chars)
{
  std::string text_utf8(text.toUtf8().constData());
  std::string result_utf8 = encode_special_chars(text_utf8, special_chars);
  QString result = QString::fromUtf8(result_utf8.c_str());
  return result;
}


//----------------------------------------------------------------
// find_object_from_path
// 
static void
find_object_from_path(QObject * root,
		      const QString * path,
		      const unsigned int & path_size,
		      std::list<QObject *> & objects)
{
  // sanity checks:
  if (root == NULL) return;
  if (path_size < 1) return;
  
  if (path[0] != root->objectName() &&
      path[0] != root->metaObject()->className())
  {
    return;
  }
  
  // the base case:
  if (path_size == 1)
  {
    objects.push_back(root);
    return;
  }
  
  const QObjectList & children = root->children();
  for (QObjectList::const_iterator i = children.begin();
       i != children.end(); ++i)
  {
    // recursion:
    find_object_from_path(*i, &path[1], path_size - 1, objects);
  }
}


//----------------------------------------------------------------
// QObjectTraits::special_chars_
// 
const char *
QObjectTraits::special_chars_ = "/ \t\n";

//----------------------------------------------------------------
// QObjectTraits::QObjectTraits
// 
QObjectTraits::QObjectTraits():
  path_(NULL),
  path_size_(0),
  class_name_(),
  index_(UINT_MAX),
  is_visible_(false)
{}

//----------------------------------------------------------------
// QObjectTraits::QObjectTraits
// 
QObjectTraits::QObjectTraits(const QObject * obj):
  path_(NULL),
  path_size_(0),
  class_name_(obj->metaObject()->className()),
  index_(UINT_MAX),
  is_visible_(false)
{
  if (obj->isWidgetType())
  {
    const QWidget * widget = static_cast<const QWidget *>(obj);
    is_visible_ = widget->isVisible();
  }
  
  QString full_path;
  convert_object_ptr_to_full_path(obj, full_path);
  
  init_path(full_path);
  
  std::list<QObject *> objects;
  matching_objects(objects);
  index_ = index_of(objects, const_cast<QObject *>(obj));
}

//----------------------------------------------------------------
// QObjectTraits::QObjectTraits
// 
QObjectTraits::QObjectTraits(const char * full_path,
			     const char * class_name,
			     const unsigned int & index,
			     const bool & is_visible):
  path_(NULL),
  path_size_(0),
  class_name_(class_name),
  index_(index),
  is_visible_(is_visible)
{
  init_path(QString::fromUtf8(full_path));
}

//----------------------------------------------------------------
// QObjectTraits::QObjectTraits
// 
QObjectTraits::QObjectTraits(const QObjectTraits & traits):
  path_(NULL),
  path_size_(0),
  class_name_(),
  index_(UINT_MAX),
  is_visible_(false)
{
  *this = traits;
}

//----------------------------------------------------------------
// QObjectTraits::~QObjectTraits
// 
QObjectTraits::~QObjectTraits()
{
  delete [] path_;
}

//----------------------------------------------------------------
// QObjectTraits::operator =
// 
QObjectTraits &
QObjectTraits::operator = (const QObjectTraits & traits)
{
  if (&traits == this) return *this;
  
  delete [] path_;
  path_ = NULL;
  
  path_size_ = traits.path_size_;
  path_ = new QString [path_size_];
  for (unsigned int i = 0; i < path_size_; i++)
  {
    path_[i] = traits.path_[i];
  }
  
  class_name_ = traits.class_name_;
  index_ = traits.index_;
  is_visible_ = traits.is_visible_;
  
  return *this;
}

//----------------------------------------------------------------
// QObjectTraits::object
// 
QObject *
QObjectTraits::object() const
{
  std::list<QObject *> objects;
  matching_objects(objects);
  
  if (objects.empty()) return NULL;
  if (is_size_one(objects)) return objects.front();
  
  // try to disambiguate between the duplicates:
  std::list<QObject *>::iterator iter = iterator_at_index(objects, index_);
  if (iter == objects.end()) return NULL;
  
  return *iter;
}

//----------------------------------------------------------------
// QObjectTraits::widget
// 
QWidget *
QObjectTraits::widget() const
{
  QObject * obj = object();
  if (obj == NULL) return NULL;
  if (obj->isWidgetType() == false) return NULL;
  
  return static_cast<QWidget *>(obj);
}

//----------------------------------------------------------------
// QObjectTraits::convert_object_ptr_to_full_path
// 
void
QObjectTraits::convert_object_ptr_to_full_path(const QObject * object,
					       QString & full_path)
{
  while (object != NULL)
  {
    QString name = object->objectName();
    if (name.isEmpty())
    {
      name = object->metaObject()->className();
    }
    
    full_path = ('/' +
		 encode_special_chars(name, special_chars_) +
		 full_path);
    
    object = object->parent();
  }
}

//----------------------------------------------------------------
// QObjectTraits::split_the_path_into_components
// 
void
QObjectTraits::split_the_path_into_components(const QString & full_path,
					      QList<QString> & path_names)
{
  static const char escape = '\\';
  static const char separator = '/';

  std::string utf8_path(full_path.toUtf8().constData());
  std::string utf8_name;
  
  unsigned int path_len = utf8_path.size();
  for (unsigned int i = 1; i < path_len; i++)
  {
    char p = utf8_path[i - 1]; // previous character.
    char c = utf8_path[i];     // current character.
    
    if (((p != escape) && (c == separator)) ||
	((i + 1) == path_len))
    {
      utf8_name += p;
      if ((i + 1) == path_len) utf8_name += c;
      std::string decoded = decode_special_chars(utf8_name);
      path_names.append(QString::fromUtf8(utf8_name.c_str()));
      utf8_name.clear();
      i++;
    }
    else if (i > 1)
    {
      utf8_name += p;
    }
  }
}

//----------------------------------------------------------------
// QObjectTraits::save
// 
void
QObjectTraits::save(ostream & ostr) const
{
  for (unsigned int i = 0; i < path_size_; i++)
  {
    ostr << '/' << encode_special_chars(path_[i], special_chars_);
  }
  
  ostr << ' '
       << class_name_ << ' '
       << index_ << ' '
       << is_visible_;
}

//----------------------------------------------------------------
// QObjectTraits::load
// 
void
QObjectTraits::load(istream & istr)
{
  QString full_path;
  istr >> full_path;
  
  istr >> class_name_;
  istr >> index_;
  istr >> is_visible_;
  
  init_path(full_path);
}

//----------------------------------------------------------------
// QObjectTraits::init_path
// 
void
QObjectTraits::init_path(const QString & full_path)
{
  delete [] path_;
  path_ = NULL;
  path_size_ = 0;
  
  // split the full path to object into a list of component path names:
  QList<QString> path_names;
  split_the_path_into_components(full_path, path_names);
  
  path_size_ = path_names.size();
  path_ = new QString [path_size_];
  for (unsigned int i = 0; i < path_size_; i++)
  {
    path_[i] = path_names[i];
  }
}

//----------------------------------------------------------------
// QObjectTraits::matching_objects
// 
void
QObjectTraits::matching_objects(std::list<QObject *> & objects) const
{
  // find a corresponding QObject for every element of the path:
  QWidgetList top_level_widgets = QApplication::topLevelWidgets();
  for (QWidgetList::iterator i = top_level_widgets.begin();
       i != top_level_widgets.end(); ++i)
  {
    find_object_from_path(*i, path_, path_size_, objects);
  }
  
  if (objects.empty()) return;
  if (is_size_one(objects)) return;
  
  // try to disambiguate between the duplicates:
  std::list<QObject *> unique;
  
  for (std::list<QObject *>::iterator jter = objects.begin();
       jter != objects.end(); ++jter)
  {
    QObject * obj = *jter;
    if (class_name_ != obj->metaObject()->className()) continue;
    
    if (obj->isWidgetType())
    {
      QWidget * widget = static_cast<QWidget *>(obj);
      if (widget->isVisible() != is_visible_) continue;
    }
    
    unique.push_back(obj);
  }
  
  objects = unique;
}

//----------------------------------------------------------------
// operator <<
// 
ostream &
operator << (ostream & ostr, const QObjectTraits & traits)
{
  traits.save(ostr);
  return ostr;
}

//----------------------------------------------------------------
// operator >>
// 
istream &
operator >> (istream & istr, QObjectTraits & traits)
{
  traits.load(istr);
  return istr;
}


//----------------------------------------------------------------
// operator <<
// 
static ostream &
operator << (ostream & ostr, const QEvent::Type & t)
{
  return ostr << (int)t;
}

//----------------------------------------------------------------
// operator <<
// 
static ostream &
operator << (ostream & ostr, const QPoint & p)
{
  return ostr << p.x() << ' ' << p.y();
}

//----------------------------------------------------------------
// operator <<
// 
static ostream &
operator << (ostream & ostr, const QPointF & p)
{
  return ostr << p.x() << ' ' << p.y();
}

//----------------------------------------------------------------
// operator <<
// 
static ostream &
operator << (ostream & ostr, const QSize & s)
{
  return ostr << s.width() << ' ' << s.height();
}

//----------------------------------------------------------------
// operator <<
// 
static ostream &
operator << (ostream & ostr, const QObject * object)
{
  save_address(ostr, object);
  return ostr;
}

//----------------------------------------------------------------
// operator >>
// 
static istream &
operator >> (istream & istr, QEvent::Type & t)
{
  int temp;
  istr >> temp;
  t = (QEvent::Type)temp;
  return istr;
}

//----------------------------------------------------------------
// operator >>
// 
static istream &
operator >> (istream & istr, QPoint & p)
{
  int x;
  int y;
  istr >> x >> y;
  p.setX(x);
  p.setY(y);
  return istr;
}

//----------------------------------------------------------------
// operator >>
// 
static istream &
operator >> (istream & istr, QPointF & p)
{
  istr >> p.rx() >> p.ry();
  return istr;
}

//----------------------------------------------------------------
// operator >>
// 
static istream &
operator >> (istream & istr, QSize & s)
{
  int width = 0;
  int height = 0;
  istr >> width >> height;
  s.setWidth(width);
  s.setHeight(height);
  return istr;
}


//----------------------------------------------------------------
// the_trail_line_id_t
// 
// Each line of the trail file will have an ID, which will simplify
// the parsing of the line.
// 
typedef enum
  {
    OBJECT_ID_E = 0,
    EVENT_E = 1,
    BYPASS_E = 2
  } the_trail_line_id_t;


//----------------------------------------------------------------
// the_qt_trail_t::the_qt_trail_t
// 
the_qt_trail_t::
the_qt_trail_t(int & argc, char ** argv, bool record_by_default):
  QApplication(argc, argv),
  the_trail_t(argc, argv, record_by_default)
{
  the_mouse_event_t::setup_transition_detectors(QEvent::MouseButtonPress,
						QEvent::MouseButtonRelease);
  mouse_.setup_buttons(Qt::LeftButton,
		       Qt::MidButton,
		       Qt::RightButton);
  
  the_keybd_event_t::setup_transition_detectors(QEvent::KeyPress,
						QEvent::KeyRelease);
  
  keybd_.init_key(the_keybd_t::SHIFT, Qt::Key_Shift, Qt::ShiftModifier);
  keybd_.init_key(the_keybd_t::ALT, Qt::Key_Alt, Qt::AltModifier);
  keybd_.init_key(the_keybd_t::CONTROL, Qt::Key_Control, Qt::ControlModifier);
  keybd_.init_key(the_keybd_t::META, Qt::Key_Meta, Qt::MetaModifier);
  
  keybd_.init_key(the_keybd_t::ARROW_UP, Qt::Key_Up);
  keybd_.init_key(the_keybd_t::ARROW_DOWN, Qt::Key_Down);
  keybd_.init_key(the_keybd_t::ARROW_LEFT, Qt::Key_Left);
  keybd_.init_key(the_keybd_t::ARROW_RIGHT, Qt::Key_Right);
  
  keybd_.init_key(the_keybd_t::PAGE_UP, Qt::Key_PageUp);
  keybd_.init_key(the_keybd_t::PAGE_DOWN, Qt::Key_PageDown);
  
  keybd_.init_key(the_keybd_t::HOME, Qt::Key_Home);
  keybd_.init_key(the_keybd_t::END, Qt::Key_End);
  
  keybd_.init_key(the_keybd_t::INSERT, Qt::Key_Insert);
  keybd_.init_key(the_keybd_t::DELETE, Qt::Key_Delete);
  
  keybd_.init_key(the_keybd_t::ESCAPE, Qt::Key_Escape);
  keybd_.init_key(the_keybd_t::TAB, Qt::Key_Tab);
  keybd_.init_key(the_keybd_t::BACKSPACE, Qt::Key_Backspace);
  keybd_.init_key(the_keybd_t::RETURN, Qt::Key_Return);
  keybd_.init_key(the_keybd_t::ENTER, Qt::Key_Enter);
  keybd_.init_key(the_keybd_t::SPACE, Qt::Key_Space);
  
  keybd_.init_key(the_keybd_t::F1, Qt::Key_F1);
  keybd_.init_key(the_keybd_t::F2, Qt::Key_F2);
  keybd_.init_key(the_keybd_t::F3, Qt::Key_F3);
  keybd_.init_key(the_keybd_t::F4, Qt::Key_F4);
  keybd_.init_key(the_keybd_t::F5, Qt::Key_F5);
  keybd_.init_key(the_keybd_t::F6, Qt::Key_F6);
  keybd_.init_key(the_keybd_t::F7, Qt::Key_F7);
  keybd_.init_key(the_keybd_t::F8, Qt::Key_F8);
  keybd_.init_key(the_keybd_t::F9, Qt::Key_F9);
  keybd_.init_key(the_keybd_t::F10, Qt::Key_F10);
  keybd_.init_key(the_keybd_t::F11, Qt::Key_F11);
  keybd_.init_key(the_keybd_t::F12, Qt::Key_F12);
  
  keybd_.init_key(the_keybd_t::NUMLOCK, Qt::Key_NumLock);
  keybd_.init_key(the_keybd_t::CAPSLOCK, Qt::Key_CapsLock);
  
  keybd_.init_ascii('~', Qt::Key_AsciiTilde);
  keybd_.init_ascii('-', Qt::Key_Minus);
  keybd_.init_ascii('=', Qt::Key_Equal);
  keybd_.init_ascii('\\', Qt::Key_Backslash);
  keybd_.init_ascii('\t', Qt::Key_Tab);
  keybd_.init_ascii('[', Qt::Key_BracketLeft);
  keybd_.init_ascii(']', Qt::Key_BracketRight);
  keybd_.init_ascii('\n', Qt::Key_Enter);
  keybd_.init_ascii('\r', Qt::Key_Return);
  keybd_.init_ascii(';', Qt::Key_Semicolon);
  keybd_.init_ascii('\'', Qt::Key_Apostrophe);
  keybd_.init_ascii(',', Qt::Key_Comma);
  keybd_.init_ascii('.', Qt::Key_Period);
  keybd_.init_ascii('/', Qt::Key_Slash);
  keybd_.init_ascii(' ', Qt::Key_Space);
  
  keybd_.init_ascii('0', Qt::Key_0);
  keybd_.init_ascii('1', Qt::Key_1);
  keybd_.init_ascii('2', Qt::Key_2);
  keybd_.init_ascii('3', Qt::Key_3);
  keybd_.init_ascii('4', Qt::Key_4);
  keybd_.init_ascii('5', Qt::Key_5);
  keybd_.init_ascii('6', Qt::Key_6);
  keybd_.init_ascii('7', Qt::Key_7);
  keybd_.init_ascii('8', Qt::Key_8);
  keybd_.init_ascii('9', Qt::Key_9);
  
  keybd_.init_ascii('A', Qt::Key_A);
  keybd_.init_ascii('B', Qt::Key_B);
  keybd_.init_ascii('C', Qt::Key_C);
  keybd_.init_ascii('D', Qt::Key_D);
  keybd_.init_ascii('E', Qt::Key_E);
  keybd_.init_ascii('F', Qt::Key_F);
  keybd_.init_ascii('G', Qt::Key_G);
  keybd_.init_ascii('H', Qt::Key_H);
  keybd_.init_ascii('I', Qt::Key_I);
  keybd_.init_ascii('J', Qt::Key_J);
  keybd_.init_ascii('K', Qt::Key_K);
  keybd_.init_ascii('L', Qt::Key_L);
  keybd_.init_ascii('M', Qt::Key_M);
  keybd_.init_ascii('N', Qt::Key_N);
  keybd_.init_ascii('O', Qt::Key_O);
  keybd_.init_ascii('P', Qt::Key_P);
  keybd_.init_ascii('Q', Qt::Key_Q);
  keybd_.init_ascii('R', Qt::Key_R);
  keybd_.init_ascii('S', Qt::Key_S);
  keybd_.init_ascii('T', Qt::Key_T);
  keybd_.init_ascii('U', Qt::Key_U);
  keybd_.init_ascii('V', Qt::Key_V);
  keybd_.init_ascii('W', Qt::Key_W);
  keybd_.init_ascii('X', Qt::Key_X);
  keybd_.init_ascii('Y', Qt::Key_Y);
  keybd_.init_ascii('Z', Qt::Key_Z);
}

//----------------------------------------------------------------
// the_qt_trail_t::~the_qt_trail_t
// 
the_qt_trail_t::~the_qt_trail_t()
{}

//----------------------------------------------------------------
// the_user_input_event
// 
// verify whether a given event is a direct result of user input:
static bool
the_user_input_event(const QObject * object, const QEvent * event)
{
  if ((event == NULL) || (object == NULL)) return false;
  
  switch (event->type())
  {
    case QEvent::MouseButtonPress:
    case QEvent::MouseButtonRelease:
    case QEvent::MouseButtonDblClick:
    case QEvent::MouseMove:
    case QEvent::KeyPress:
    case QEvent::KeyRelease:
    case QEvent::ShortcutOverride:
    case QEvent::Shortcut:
    case QEvent::Wheel:
    case QEvent::TabletMove:
    case QEvent::TabletPress:
    case QEvent::TabletRelease:
    case QEvent::Close:
    case QEvent::Quit:
    case QEvent::DragEnter:
    case QEvent::DragMove:
    case QEvent::DragLeave:
    case QEvent::Drop:
    case QEvent::LocaleChange:
    case QEvent::LanguageChange:
      /*
    case QEvent::Enter:
    case QEvent::Leave:
    case QEvent::FocusIn:
    case QEvent::FocusOut:
    case QEvent::WindowActivate:
    case QEvent::WindowDeactivate:
      */
      return true;
      
    case QEvent::Move:
    case QEvent::Resize:
    {
      // make sure that the event is genuinely user generated:
      const QWidget * widget = static_cast<const QWidget *>(object);
      return (widget->isWindow() && widget->isVisible());
    }
    
    default:
      break;
  }
  
  // cerr << "NOT USER EVENT: " << qevent_type_to_str(event->type()) << endl;
  return false;
}

//----------------------------------------------------------------
// lock_t
// 
class lock_t
{
public:
  lock_t(unsigned int & counter): counter_(counter) { counter++; }
  ~lock_t() { counter_--; }
  unsigned int & counter_;
};

//----------------------------------------------------------------
// the_qt_trail_t::notify
// 
bool
the_qt_trail_t::notify(QObject * object, QEvent * event)
{
  if (object == this)
  {
    // special case:
    return QApplication::notify(object, event);
  }
  
  static unsigned int recursion_depth_ = 0;
  lock_t lock(recursion_depth_);

  if (dont_post_events_ && the_user_input_event(object, event))
  {
#if 0
    cerr << recursion_depth_ << ". ignore: " << QObjectTraits(object)
	 << ", event: " << qevent_type_to_str(event->type()) << endl;
#endif
    
    // ignore any "new" events during trail playback:
    return true;
  }
  
#if 0
  // FIXME: if (!dont_save_events_)
  {
    cerr << recursion_depth_ << ". notify: " << QObjectTraits(object)
	 << ", event: " << qevent_type_to_str(event->type()) << endl;
  }
#endif
  
  // process the event:
  update_devices(object, event);
  
  // only certain events should be saved:
  if (the_user_input_event(object, event) &&
      record_stream.rdbuf()->is_open() &&
      (dont_save_events_ == false))
  {
    save_event(record_stream, object, event);
  }

#if 0
  if (the_user_input_event(object, event) && !dont_save_events_)
  {
    cerr << recursion_depth_ << ". user event: " << QObjectTraits(object)
	 << ", event: " << qevent_type_to_str(event->type())
	 << endl;
  }
#endif
  
  // WARNING: this call will block if the object processing the event
  // will start a modal dialog:
  bool res = false;
  try
  {
    res = QApplication::notify(object, event);
  }
  catch (std::exception & e)
  {
    cerr << "EXCEPTION: " << e.what() << endl;
  }
  catch (...)
  {
    cerr << "EXCEPTION: unknown exception intercepted" << endl;
  }
  
  return res;
}

//----------------------------------------------------------------
// REPLAY_EVENT_ID
// 
static const QEvent::Type REPLAY_EVENT_ID = QEvent::User;

//----------------------------------------------------------------
// the_replay_event_t
// 
class the_replay_event_t : public QEvent
{
public:
  the_replay_event_t():
    QEvent(REPLAY_EVENT_ID)
  {}
};

//----------------------------------------------------------------
// REPLAY_THREAD
//
class the_qt_trail_thread_t : public QThread
{
protected:
  void run()
  {
    while (THE_TRAIL.replay_stream.rdbuf()->is_open())
    {
      QThread::msleep(1);
      THE_TRAIL.timeout();
    }
  }
};

//----------------------------------------------------------------
// REPLAY_THREAD
// 
static the_qt_trail_thread_t REPLAY_THREAD;

//----------------------------------------------------------------
// the_qt_trail_t::timeout
// 
void
the_qt_trail_t::timeout()
{
  postEvent(this, new the_replay_event_t());
}

//----------------------------------------------------------------
// the_qt_trail_t::customEvent
// 
void
the_qt_trail_t::customEvent(QEvent * event)
{
  if (event->type() == REPLAY_EVENT_ID)
  {
    event->accept();
    replay_one();
    return;
  }
  
  event->ignore();
}

//----------------------------------------------------------------
// io_base_handler
// 
static void
io_base_handler(io_base_t *& loaded)
{
  call_t * call = dynamic_cast<call_t *>(loaded);
  if (call)
  {
    call->execute();
    QCoreApplication::processEvents();
  }
}

//----------------------------------------------------------------
// the_qt_trail_t::replay
// 
void
the_qt_trail_t::replay()
{
  // FIXME: REPLAY_THREAD.start();
  
  // register loaders:
  QCoreApplication::processEvents();
  io_base_t::loaders_["instance_t"] = &instance_t::create;
  io_base_t::loaders_["call_t"] = &call_t::create;
  io_base_t::load(replay_stream, &io_base_handler);
  replay_done();
  
  // FIXME:  ::exit(0);
}

//----------------------------------------------------------------
// the_qt_trail_t::replay_done
// 
void
the_qt_trail_t::replay_done()
{
  the_trail_t::replay_done();
  
#if 0
  QWidgetList top_level_widgets = QApplication::topLevelWidgets();
  for (QWidgetList::iterator i = top_level_widgets.begin();
       i != top_level_widgets.end(); ++i)
  {
    dump_children_tree(cerr, *i, 0);
  }
#endif
}

//----------------------------------------------------------------
// MILESTONE
// 
unsigned int MILESTONE = ~0;

//----------------------------------------------------------------
// the_qt_trail_t::replay_one
// 
void
the_qt_trail_t::replay_one()
{
  if (dont_load_events_) return;
  
  QObject * object = NULL;
  QEvent  * event  = NULL;
  
  // load the event milestone marker:
  {
    static bool waiting_for_milestone = false;
    static QTime timer;
    
    if (!waiting_for_milestone)
    {
      replay_stream >> MILESTONE;
    }
    
    if (MILESTONE > milestone_)
    {
      if (!waiting_for_milestone)
      {
	cerr << "NOTE: current milestone " << milestone_
	     << ", trail milestone " << MILESTONE
	     << ", waiting..." << endl;
	waiting_for_milestone = true;
	timer.start();
      }
      
      unsigned seconds_waiting = timer.elapsed() / 1000;
      if (seconds_waiting > seconds_to_wait_)
      {
	if (!ask_the_user_)
	{
	  // don't ask any questions -- terminate trail playback:
	  cerr << "NOTE: " << seconds_waiting
	       << " seconds passed -- trail may be out of sequence" << endl;
	  
	  replay_done();
	  return;
	}
	else
	{
	  dont_load_events_ = true;
	  dont_save_events_ = true;
	  dont_post_events_ = false;
	  
	  QMessageBox mb;
	  mb.setIcon(QMessageBox::Question);	
	  mb.setWindowTitle(QString::fromUtf8("trail may be out of sequence"));
	  
	  std::ostringstream os;
	  os << "Milestone " << MILESTONE << " hasn't arrived within "
	     << seconds_to_wait_ << " seconds." << endl;
	  mb.setText(QString::fromUtf8(os.str().c_str()));
	  
	  os.str("");
	  os << seconds_waiting
	     << " seconds passed -- trail may be out of sequence." << endl
	     << "Current trail line number is " << line_num_
	     << ", current milestone is " << milestone_ << "." << endl
	     << "Click [Stop] to stop trail playback immediately." << endl
	     << "Click [Skip] to ignore this milestone." << endl
	     << "Click [Wait] to continue waiting for the milestone." << endl;
	  mb.setDetailedText(QString::fromUtf8(os.str().c_str()));
	  
	  QAbstractButton * btn_stop =
	    mb.addButton(tr("Stop"), QMessageBox::ActionRole);
	  
	  QAbstractButton * btn_skip =
	    mb.addButton(tr("Skip"), QMessageBox::ActionRole);
	  
	  QAbstractButton * btn_wait =
	    mb.addButton(tr("Wait"), QMessageBox::ActionRole);
	  
	  mb.exec();
	  
	  dont_save_events_ = false;
	  dont_post_events_ = true;
	  
	  if (mb.clickedButton() == btn_stop)
	  {
	    replay_done();
	    return;
	  }
	  else if (mb.clickedButton() == btn_skip)
	  {
	    // fall through -- skip the milestone
	    milestone_ = MILESTONE;
	  }
	  else if (mb.clickedButton() == btn_wait)
	  {
	    // wait some more:
	    timer.start();
	    return;
	  }
	}
      }
      else
      {
	return;
      }
    }
    
    waiting_for_milestone = false;
  }
  
  unsigned int line_id = ~0;
  bool ok = true;
  while (ok)
  {
    // read the line id:
    if ((replay_stream >> line_id).eof())
    {
      // end of trail:
      ok = false;
      break;
    }
    
    if (line_id == OBJECT_ID_E)
    {
      ok = load_object(replay_stream);
    }
    else if (line_id == EVENT_E)
    {
      ok = load_event(replay_stream, object, event);
      break;
    }
    else if (line_id == BYPASS_E)
    {
      ok = load_bypass(replay_stream);
      break;
    }
    else
    {
      cerr << "ERROR: invalid line id: " << line_id
	   << ", line: " << line_num_
	   << ", current milestone: " << milestone_
	   << ", trail milestone: " << MILESTONE << endl;
      ok = false;
      break;
    }
  }
  
  if (!ok)
  {
    replay_done();
    return;
  }
  
  if (line_id == BYPASS_E)
  {
    return;
  }

#ifndef NDEBUG
  cout << line_num_ << ", " << qevent_type_to_str(event->type()) << endl;
#endif
  
  if (event->type() == QEvent::MouseButtonPress ||
      event->type() == QEvent::MouseButtonDblClick ||
      event->type() == QEvent::TabletPress)
  {
#ifndef NDEBUG
    static unsigned int critical_event_counter = 0;
    critical_event_counter++;
    cout << critical_event_counter << " critical event --------------" << endl;
#endif
    
    if (single_step_replay_)
    {
      dont_load_events_ = true;
      dont_save_events_ = true;
      dont_post_events_ = false;
      
      QMessageBox mb;
      mb.setIcon(QMessageBox::Question);	
      mb.setWindowTitle(QString::fromUtf8("trail arrived at a critical event"));
      
      std::ostringstream os;
      os << qevent_type_to_str(event->type()) << endl;
      mb.setText(QString::fromUtf8(os.str().c_str()));
      
      os.str("");
      os << "Critical event \"" << qevent_type_to_str(event->type()) << "\""
	 << " at line " << line_num_ << " is pending execution." << endl
	 << "Click [Next] to execute this event." << endl
	 << "Click [Don't ask] to execute all future critical events." << endl
	 << "Click [Stop] to stop trail playback immediately "
	 << "without executing the next event." << endl;
      mb.setDetailedText(QString::fromUtf8(os.str().c_str()));
      
      QAbstractButton * btn_next =
	mb.addButton(tr("Next"), QMessageBox::ActionRole);
      
      QAbstractButton * btn_dont =
	mb.addButton(tr("Don't ask"), QMessageBox::ActionRole);
      
      QAbstractButton * btn_stop =
	mb.addButton(tr("Stop"), QMessageBox::ActionRole);
      
      mb.exec();
      
      dont_save_events_ = false;
      dont_post_events_ = true;
      
      if (mb.clickedButton() == btn_next)
      {
	// fall through -- execute the event
      }
      else if (mb.clickedButton() == btn_dont)
      {
	single_step_replay_ = false;
      }
      else if (mb.clickedButton() == btn_stop)
      {
	delete event;
	replay_done();
	return;
      }
    }
  }
  
  dont_post_events_ = false;
  
  if (object)
  {
#if 0
    {
      static unsigned int prev_milestone = ~0;
      if (prev_milestone != milestone_)
      {
	prev_milestone = milestone_;
	cerr << milestone_ << '\t' << QObjectTraits(object)
	     << ", event: " << qevent_type_to_str(event->type())
	     << endl;
      }
    }
#endif
    
    // FIXME: not safe:
    QWidget * widget = dynamic_cast<QWidget *>(object);
    
    switch (event->type())
    {
      case QEvent::Move:
      {
	QMoveEvent * e_move = (QMoveEvent *)event;
	widget->setGeometry(QRect(e_move->pos().x(),
				  e_move->pos().y(),
				  widget->size().width(),
				  widget->size().height()));
      }
      break;
      
      case QEvent::Resize:
      {
	QResizeEvent * e_resize = (QResizeEvent *)event;
	widget->resize(e_resize->size().width(),
		       e_resize->size().height());
      }
      break;
      
      case QEvent::Close:
	widget->close();
	break;
	
      case QEvent::ShortcutOverride:
      case QEvent::KeyPress:
      case QEvent::KeyRelease:
	// NOTE: apparently, the QApplication::notify will deliver
	// an event to a disabled widget, so this is a workaround:
	// FIXME: actually, if this ever happens, it means the trail has
	// gone out of sequence:
	if (widget->isEnabled()) notify(object, event);
	break;
	
      default:
	notify(object, event);
	break;
    }
  }
  
  // FIXME: is this safe?
  delete event;
  dont_post_events_ = true;
  dont_load_events_ = false;
}

//----------------------------------------------------------------
// the_qt_trail_t::bypass_prolog
// 
bool
the_qt_trail_t::bypass_prolog(const char * name)
{
  if (is_recording())
  {
    // save the milestone marker:
    record_stream << milestone_ << '\t'
		  << BYPASS_E << ' '
		  << "bypass_prolog" << ' '
		  << encode_special_chars(std::string(name)).c_str()
		  << endl;
    dont_save_events_ = true;
    record_bypass_name_ = name;
  }
  
  if (is_replaying())
  {
    // wait for the bypass_prolog marker:
    QTime timer;
    timer.start();
    
    while (replay_bypass_name_.empty())
    {
      unsigned seconds_waiting = timer.elapsed() / 1000;
      if (seconds_waiting > seconds_to_wait_)
      {
	QMessageBox mb;
	mb.setIcon(QMessageBox::Question);	
	mb.setWindowTitle(QString::fromUtf8("trail may be out of sequence"));
	
	std::ostringstream os;
	os << "bypass_prolog " << name << " hasn't arrived within "
	   << seconds_to_wait_ << " seconds." << endl;
	mb.setText(QString::fromUtf8(os.str().c_str()));
	
	os.str("");
	os << seconds_waiting
	   << " seconds passed -- trail may be out of sequence." << endl
	   << "Current trail line number is " << line_num_
	   << ", current milestone is " << milestone_ << "." << endl
	   << "Click [Stop] to stop trail playback immediately." << endl
	   << "Click [Skip] to ignore this problem." << endl
	   << "Click [Wait] to continue waiting for the milestone." << endl;
	mb.setDetailedText(QString::fromUtf8(os.str().c_str()));
	
	QAbstractButton * btn_stop =
	  mb.addButton(tr("Stop"), QMessageBox::ActionRole);
	
	QAbstractButton * btn_skip =
	  mb.addButton(tr("Skip"), QMessageBox::ActionRole);
	
	QAbstractButton * btn_wait =
	  mb.addButton(tr("Wait"), QMessageBox::ActionRole);
	
	mb.exec();
	
	if (mb.clickedButton() == btn_stop)
	{
	  replay_done();
	  return false;
	}
	else if (mb.clickedButton() == btn_skip)
	{
	  // fall through -- skip the milestone
	  return false;
	}
	else if (mb.clickedButton() == btn_wait)
	{
	  // wait some more:
	  timer.start();
	}
      }
      
#ifdef WIN32
      Sleep(10);
#else
      usleep(10);
#endif
      QCoreApplication::processEvents();
    }
  }
  
  return true;
}

//----------------------------------------------------------------
// the_qt_trail_t::bypass_epilog
// 
void
the_qt_trail_t::bypass_epilog()
{
  if (is_replaying())
  {
    replay_bypass_name_.clear();
    dont_load_events_ = false;
  }
  
  if (is_recording())
  {
    // save the milestone marker:
    record_stream << milestone_ << '\t'
		  << BYPASS_E << ' '
		  << "bypass_epilog"
		  << endl;
    dont_save_events_ = false;
    record_bypass_name_.clear();
  }
}

//----------------------------------------------------------------
// the_qt_trail_t::stop
// 
void
the_qt_trail_t::stop()
{
  the_trail_t::stop();
  REPLAY_THREAD.wait();
}

//----------------------------------------------------------------
// the_qt_trail_t::update_devices
// 
void
the_qt_trail_t::update_devices(QObject * object, const QEvent * event)
{
  // FIXME:
  return;
  
  if ((object == NULL) || (event == NULL)) return;
  
  // process events originating from direct manipulation devices,
  // such as the keyboard, mouse, or tablet:
  switch (event->type())
  {
    case QEvent::MouseButtonPress:
    case QEvent::MouseButtonRelease:
    case QEvent::MouseButtonDblClick:
    case QEvent::MouseMove:
    {
      QWidget * widget = static_cast<QWidget *>(object);
      the_input_device_t::advance_time_stamp();
      mouse_.update(the_mouse_event(widget,
				    static_cast<const QMouseEvent *>(event)));
      keybd_.update(the_mouse_event(widget,
				    static_cast<const QMouseEvent *>(event)));
    }
    break;
    
    case QEvent::ShortcutOverride:
    case QEvent::KeyPress:
    case QEvent::KeyRelease:
    {
      const QKeyEvent * ke = static_cast<const QKeyEvent *>(event);
      QWidget * widget = static_cast<QWidget *>(object);
      
      the_input_device_t::advance_time_stamp();
      keybd_.update(the_keybd_event(widget, ke));
      
      // FIXME: if (!ke->isAutoRepeat()) ::dump(THE_KEYBD);
    }
    break;
    
    case QEvent::Shortcut:
      // FIXME: for some reason on Windows this is necessary:
      keybd_.forget_pressed_keys();
      break;
      
    case QEvent::TabletMove:
    case QEvent::TabletPress:
    case QEvent::TabletRelease:
    {
      QWidget * widget = static_cast<QWidget *>(object);
      the_input_device_t::advance_time_stamp();
      wacom_.update(the_wacom_event(widget,
				    static_cast<const QTabletEvent *>(event)));
    }
    break;
    
    default:
      return;
  }
}

//----------------------------------------------------------------
// the_qt_trail_t::save_event
// 
void
the_qt_trail_t::save_event(ostream &       ostr,
			   const QObject * object,
			   const QEvent *  event)
{
  // FIXME:
  return;
  
  if ((event == NULL) || (object == NULL)) return;
  
#if 0
  if (replay_stream.rdbuf()->is_open() == false)
  {
    static unsigned int prev_milestone = ~0;
    if (prev_milestone != milestone_)
    {
      prev_milestone = milestone_;
      cerr << milestone_ << '\t' << QObjectTraits(object)
	   << ", event: " << qevent_type_to_str(event->type())
	   << endl;
    }
  }
#endif
  
  // save the milestone marker:
  ostr << milestone_ << '\t';
  
  // save the destination object if necessary:
  QObjectTraits traits(object);
  the_bit_tree_leaf_t<the_traits_mapping_t> * leaf =
    tree_save_.get(object);
  
  if ((leaf == NULL) || (traits != leaf->elem.traits()))
  {
    leaf = tree_save_.add(object);
    leaf->elem.init(const_cast<QObject *>(object), traits);
    
    // this line will contain object information:
    ostr << OBJECT_ID_E << ' ';
    save_address(ostr, object);
    ostr << ' ' << traits << endl << '\t';
  }
  
  // save the event:
  switch (event->type())
  {
    case QEvent::MouseButtonPress:
    case QEvent::MouseButtonRelease:
    case QEvent::MouseButtonDblClick:
    case QEvent::MouseMove:
    {
      saveQMouseEvent(ostr, object, (QMouseEvent *)event);
    }
    break;
    
    case QEvent::Wheel:
    {
      saveQWheelEvent(ostr, object, (QWheelEvent *)event);
    }
    break;
    
    case QEvent::ShortcutOverride:
    case QEvent::KeyPress:
    case QEvent::KeyRelease:
    {
      saveQKeyEvent(ostr, object, (QKeyEvent *)event);
    }
    break;
    
    case QEvent::TabletMove:
    case QEvent::TabletPress:
    case QEvent::TabletRelease:
    {
      saveQTabletEvent(ostr, object, (QTabletEvent *)event);
    }
    break;
    
    case QEvent::Move:
    {
      saveQMoveEvent(ostr, object, (QMoveEvent *)event);
    }
    break;
    
    case QEvent::Resize:
    {
      saveQResizeEvent(ostr, object, (QResizeEvent *)event);
    }
    break;
    
    case QEvent::Close:
    {
      saveQCloseEvent(ostr, object, (QCloseEvent *)event);
    }
    break;
    
    case QEvent::Timer:
    {
      saveQTimerEvent(ostr, object, (QTimerEvent *)event);
    }
    break;
    
    case QEvent::Shortcut:
    {
      saveQShortcutEvent(ostr, object, (QShortcutEvent *)event);
    }
    break;
    
    default:
    {
      saveQEvent(ostr, object, event);
    }
    break;
  }
}

//----------------------------------------------------------------
// the_qt_trail_t::load_bypass
// 
bool
the_qt_trail_t::load_bypass(istream & istr)
{
  std::string bypass;
  istr >> bypass;
  line_num_++;
  
  if (bypass == "bypass_prolog")
  {
    std::string name;
    istr >> name;
    
    replay_bypass_name_ = decode_special_chars(name);
    dont_load_events_ = true;
  }
  else if (bypass != "bypass_epilog")
  {
    return false;
  }
  
  bool ok = !istr.eof();
  return ok;
}

//----------------------------------------------------------------
// the_qt_trail_t::load_object
// 
bool
the_qt_trail_t::load_object(istream & istr)
{
  uint64_t old_ptr = 0;
  if (!load_address(istr, old_ptr))
  {
    cerr << "ERROR: line: " << line_num_
	 << ", can not load old widget pointer"
	 << ", current milestone " << milestone_
	 << ", trail milestone " << MILESTONE << endl;
    return false;
  }
  
  QObjectTraits traits;
  istr >> traits;
  line_num_++;
  
  QObject * new_ptr = traits.object();
  
  if (new_ptr == NULL)
  {
    cerr << "WARNING: line: " << line_num_
	 << ", can not find corresponding object: " << endl
	 << traits
	 << ", current milestone " << milestone_
	 << ", trail milestone " << MILESTONE << endl;
    traits.object();
    
#if 0
    QWidgetList top_level_widgets = QApplication::topLevelWidgets();
    for (QWidgetList::iterator i = top_level_widgets.begin();
	 i != top_level_widgets.end(); ++i)
    {
      dump_children_tree(cerr, *i, 0);
    }
#endif
    
    return true;
  }
  
  the_bit_tree_leaf_t<the_traits_mapping_t> * leaf =
    tree_load_.add(old_ptr);
  leaf->elem.init(new_ptr, traits);
  
  return true;
}

//----------------------------------------------------------------
// the_qt_trail_t::load_event
// 
bool
the_qt_trail_t::load_event(istream &  istr,
			   QObject *& object,
			   QEvent *&  event)
{
  object = NULL;
  event = NULL;
  
  uint64_t old_object = 0;
  load_address(istr, old_object);
  
  object = NULL;
  the_bit_tree_leaf_t<the_traits_mapping_t> * old_object_traits =
    tree_load_.get(old_object);
  
  if (!old_object_traits)
  {
    cerr << "WARNING: unknown object pointer: ";
    save_address(cerr, old_object);
    cerr << ", line: " << line_num_
	 << ", current milestone " << milestone_
	 << ", trail milestone " << MILESTONE << endl;
  }
  else
  {
    object = (QObject *)(old_object_traits->elem.addr());
    
    const QObjectTraits & traits = old_object_traits->elem.traits();
    QObject * new_object = traits.object();
    
    if (new_object == NULL)
    {
      cerr << "WARNING: line: " << line_num_
	   << ", object no longer exists: " << endl
	   << traits
	   << ", current milestone " << milestone_
	   << ", trail milestone " << MILESTONE << endl;
      old_object_traits->elem.init(NULL, traits);
      object = NULL;
    }
    
    if (new_object != object)
    {
      cerr << "WARNING: line: " << line_num_
	   << ", outdated object pointer: " << endl
	   << traits
	   << ", current milestone " << milestone_
	   << ", trail milestone " << MILESTONE << endl;
      
      if (old_object_traits->elem.addr() == NULL)
      {
	old_object_traits->elem.init(new_object, traits);
      }
      
      object = new_object;
    }
  }
  
  QEvent::Type event_type;
  if ((istr >> event_type).eof())
  {
    cerr << "ERROR: missing event type, line: " << line_num_
	 << ", current milestone " << milestone_
	 << ", trail milestone " << MILESTONE << endl;
    return false;
  }
  
  switch (event_type)
  {
    case QEvent::MouseButtonPress:
    case QEvent::MouseButtonRelease:
    case QEvent::MouseButtonDblClick:
    case QEvent::MouseMove:
    {
      event = loadQMouseEvent(istr, event_type);
    }
    break;
    
    case QEvent::Wheel:
    {
      event = loadQWheelEvent(istr, event_type);
    }
    break;
    
    case QEvent::ShortcutOverride:
    case QEvent::KeyPress:
    case QEvent::KeyRelease:
    {
      event = loadQKeyEvent(istr, event_type);
    }
    break;
    
    case QEvent::TabletMove:
    case QEvent::TabletPress:
    case QEvent::TabletRelease:
    {
      event = loadQTabletEvent(istr, event_type);
    }
    break;
    
    case QEvent::Move:
    {
      event = loadQMoveEvent(istr, event_type);
    }
    break;
    
    case QEvent::Resize:
    {
      event = loadQResizeEvent(istr, event_type);
    }
    break;
    
    case QEvent::Close:
    {
      event = loadQCloseEvent(istr, event_type);
    }
    break;
    
    case QEvent::Timer:
    {
      event = loadQTimerEvent(istr, event_type);
    }
    break;
    
    case QEvent::Shortcut:
    {
      event = loadQShortcutEvent(istr, event_type);
    }
    break;
    
    default:
    {
      event = loadQEvent(istr, event_type);
    }
    break;
  }
  
  line_num_++;
  return true;
}

//----------------------------------------------------------------
// the_qt_trail_t::saveQEvent
// 
void
the_qt_trail_t::saveQEvent(ostream & ostr,
			   const QObject * object,
			   const QEvent * event)
{
  // this line will contain event information:
  ostr << EVENT_E << ' '
       << object << ' '
       << event->type() << endl;
}

//----------------------------------------------------------------
// the_qt_trail_t::saveQMouseEvent
// 
void
the_qt_trail_t::saveQMouseEvent(ostream & ostr,
				const QObject * object,
				const QMouseEvent * event)
{
  ostr << EVENT_E << ' '
       << object << ' '
       << event->type() << ' '
       << event->pos() << ' '
       << event->globalPos() << ' '
       << event->button() << ' '
       << event->buttons() << ' '
       << event->modifiers() << endl;
}

//----------------------------------------------------------------
// the_qt_trail_t::saveQWheelEvent
// 
void
the_qt_trail_t::saveQWheelEvent(ostream & ostr,
				const QObject * object,
				const QWheelEvent * event)
{
  ostr << EVENT_E << ' '
       << object << ' '
       << event->type() << ' '
       << event->pos() << ' '
       << event->globalPos() << ' '
       << event->delta() << ' '
       << event->buttons() << ' '
       << event->modifiers() << ' '
       << event->orientation() << endl;
}

//----------------------------------------------------------------
// the_qt_trail_t::saveQKeyEvent
// 
void
the_qt_trail_t::saveQKeyEvent(ostream & ostr,
			      const QObject * object,
			      const QKeyEvent * event)
{
  assert(event->key() != -1);
  
  ostr << EVENT_E << ' '
       << object << ' '
       << event->type() << ' '
       << event->key() << ' '
       << event->modifiers() << ' '
       << event->isAutoRepeat() << ' '
       << event->count() << ' ';
  
  if (event->text().size() > 0)
  {
    ostr << true << ' ' << event->text().toUtf8().toBase64().data() << endl;
  }
  else
  {
    ostr << false << endl;
  }
}

//----------------------------------------------------------------
// the_qt_trail_t::saveQTabletEvent
// 
void
the_qt_trail_t::saveQTabletEvent(ostream & ostr,
				 const QObject * object,
				 const QTabletEvent * event)
{
  ostr << EVENT_E << ' '
       << object << ' '
       << event->type() << ' '
       << event->pos() << ' '
       << event->globalPos() << ' '
       << event->hiResGlobalPos() << ' '
       << event->device() << ' '
       << event->pointerType() << ' '
       << event->pressure() << ' '
       << event->xTilt() << ' '
       << event->yTilt() << ' '
       << event->tangentialPressure() << ' '
       << event->rotation() << ' '
       << event->z() << ' '
       << event->modifiers() << ' '
       << event->uniqueId() << endl;
}

//----------------------------------------------------------------
// the_qt_trail_t::saveQMoveEvent
// 
void
the_qt_trail_t::saveQMoveEvent(ostream & ostr,
			       const QObject * object,
			       const QMoveEvent * event)
{
  ostr << EVENT_E << ' '
       << object << ' '
       << event->type() << ' '
       << event->pos() << ' '
       << event->oldPos() << endl;
}

//----------------------------------------------------------------
// the_qt_trail_t::saveQResizeEvent
// 
void
the_qt_trail_t::saveQResizeEvent(ostream & ostr,
				 const QObject * object,
				 const QResizeEvent * event)
{
  ostr << EVENT_E << ' '
       << object << ' '
       << event->type() << ' '
       << event->size() << ' '
       << event->oldSize() << endl;
}

//----------------------------------------------------------------
// the_qt_trail_t::saveQCloseEvent
// 
void
the_qt_trail_t::saveQCloseEvent(ostream & ostr,
				const QObject * object,
				const QCloseEvent * event)
{
  ostr << EVENT_E << ' '
       << object << ' '
       << event->type() << endl;
}

//----------------------------------------------------------------
// the_qt_trail_t::saveQTimerEvent
// 
void
the_qt_trail_t::saveQTimerEvent(ostream & ostr,
				const QObject * object,
				const QTimerEvent * event)
{
  ostr << EVENT_E << ' '
       << object << ' '
       << event->type() << ' '
       << event->timerId() << endl;
}

//----------------------------------------------------------------
// the_qt_trail_t::saveQShortcutEvent
// 
void
the_qt_trail_t::saveQShortcutEvent(ostream & ostr,
				   const QObject * object,
				   const QShortcutEvent * event)
{
  QShortcutEvent * fake = const_cast<QShortcutEvent *>(event);
  ostr << EVENT_E << ' '
       << object << ' '
       << fake->type() << ' '
       << fake->shortcutId() << ' '
       << fake->isAmbiguous() << ' '
       << fake->key().toString() << endl;
}

//----------------------------------------------------------------
// the_qt_trail_t::loadQEvent
// 
QEvent *
the_qt_trail_t::loadQEvent(istream &, QEvent::Type t)
{
  QEvent * event = new QEvent(t);
  return event;
}

//----------------------------------------------------------------
// the_qt_trail_t::loadQMouseEvent
// 
QMouseEvent *
the_qt_trail_t::loadQMouseEvent(istream & istr, QEvent::Type t)
{
  QPoint p;
  QPoint g;
  int btn;
  int btns;
  int mods;
  istr >> p >> g >> btn >> btns >> mods;
  QMouseEvent * event = new QMouseEvent(t,
					p,
					g,
					Qt::MouseButton(btn),
					QFlags<Qt::MouseButton>(btns),
					QFlags<Qt::KeyboardModifier>(mods));
  return event;
}

//----------------------------------------------------------------
// the_qt_trail_t::loadQWheelEvent
// 
QWheelEvent *
the_qt_trail_t::loadQWheelEvent(istream & istr, QEvent::Type)
{
  QPoint p;
  QPoint g;
  int    d;
  int    btns;
  int    mods;
  int    o;
  istr >> p >> g >> d >> btns >> mods >> o;
  QWheelEvent * event = new QWheelEvent(p,
					g,
					d,
					QFlags<Qt::MouseButton>(btns),
					QFlags<Qt::KeyboardModifier>(mods),
					Qt::Orientation(o));
  return event;
}

//----------------------------------------------------------------
// the_qt_trail_t::loadQKeyEvent
// 
QKeyEvent *
the_qt_trail_t::loadQKeyEvent(istream & istr, QEvent::Type t)
{
  int    k;
  int    mods;
  bool   autorepeat;
  int    count;
  bool   has_txt;
  istr >> k >> mods >> autorepeat >> count >> has_txt;
  QKeyEvent * event = NULL;
  
  QString txt = QString::null;
  if (has_txt)
  {
    QString txt_base64;
    istr >> txt_base64;
    
    txt = QString(QByteArray::fromBase64(txt_base64.toUtf8()));
  }
  
  event = new QKeyEvent(t,
			k,
			QFlags<Qt::KeyboardModifier>(mods),
			txt,
			autorepeat,
			count);
  
  return event;
}

//----------------------------------------------------------------
// the_qt_trail_t::loadQTabletEvent
// 
QTabletEvent *
the_qt_trail_t::loadQTabletEvent(istream & istr, QEvent::Type event_type)
{
  QPoint position;
  QPoint global_position;
  QPointF highres_global_pos;
  
  int	 device		= 0;
  int    pointer        = 0;
  qreal  pressure	= 0;
  int    x_tilt		= 0;
  int    y_tilt		= 0;
  qreal  tan_pressure   = 0;
  qreal  rotation       = 0;
  int    z              = 0;
  int    mods           = 0;
  qint64 unique_id	= 0;
  
  istr >> position
       >> global_position
       >> highres_global_pos
       >> device
       >> pointer
       >> pressure
       >> x_tilt
       >> y_tilt
       >> tan_pressure
       >> rotation
       >> z
       >> mods
       >> unique_id;
  
  QTabletEvent * event = new QTabletEvent(event_type,
					  position,
					  global_position,
					  highres_global_pos,
					  device,
					  pointer,
					  pressure,
					  x_tilt,
					  y_tilt,
					  tan_pressure,
					  rotation,
					  z,
					  QFlags<Qt::KeyboardModifier>(mods),
					  unique_id);
  return event;
}

//----------------------------------------------------------------
// the_qt_trail_t::loadQMoveEvent
// 
QMoveEvent *
the_qt_trail_t::loadQMoveEvent(istream & istr, QEvent::Type)
{
  QPoint p;
  QPoint oldp;
  istr >> p >> oldp;
  QMoveEvent * event = new QMoveEvent(p, oldp);
  return event;
}

//----------------------------------------------------------------
// the_qt_trail_t::loadQResizeEvent
// 
QResizeEvent *
the_qt_trail_t::loadQResizeEvent(istream & istr, QEvent::Type)
{
  QSize s;
  QSize olds;
  istr >> s >> olds;
  QResizeEvent * event = new QResizeEvent(s, olds);
  return event;
}

//----------------------------------------------------------------
// the_qt_trail_t::loadQCloseEvent
// 
QCloseEvent *
the_qt_trail_t::loadQCloseEvent(istream &, QEvent::Type)
{
  QCloseEvent * event = new QCloseEvent();
  return event;
}

//----------------------------------------------------------------
// the_qt_trail_t::loadQTimerEvent
// 
QTimerEvent *
the_qt_trail_t::loadQTimerEvent(istream & istr, QEvent::Type)
{
  int timer_id = 0;
  istr >> timer_id;
  QTimerEvent * event = new QTimerEvent(timer_id);
  return event;
}

//----------------------------------------------------------------
// the_qt_trail_t::loadQShortcutEvent
// 
QShortcutEvent *
the_qt_trail_t::loadQShortcutEvent(istream & istr, QEvent::Type)
{
  int shortcut_id = 0;
  bool ambiguous = false;
  QString shortcut;
  istr >> shortcut_id >> ambiguous >> shortcut;
  QKeySequence key = QKeySequence::fromString(shortcut);
  QShortcutEvent * event = new QShortcutEvent(key, shortcut_id, ambiguous);
  return event;
}


//----------------------------------------------------------------
// save
// 
bool
save(ostream & stream, const QSize & data)
{
  stream << data.width() << ' ' << data.height() << ' ';
  return true;
}

//----------------------------------------------------------------
// load
// 
bool
load(istream & stream, QSize & data)
{
  int w, h;
  stream >> w >> h;
  data = QSize(w, h);
  return true;
}

//----------------------------------------------------------------
// save
// 
bool
save(ostream & stream, const QRect & data)
{
  stream << data.x() << ' '
	 << data.y() << ' '
	 << data.width() << ' '
	 << data.height() << ' ';
  return true;
}

//----------------------------------------------------------------
// load
// 
bool
load(istream & stream, QRect & data)
{
  int x, y, w, h;
  stream >> x >> y >> w >> h;
  data = QRect(x, y, w, h);
  return true;
}
