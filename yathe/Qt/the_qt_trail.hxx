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


// File         : the_qt_trail.hxx
// Author       : Pavel Aleksandrovich Koshevoy
// Created      : 2001/06/24 16:47:24
// Copyright    : (C) 2001, 2002, 2003
// License      : MIT
// Description  : An event trail recoring/playback mechanism used
//                for regression testing and debugging.

#ifndef THE_QT_TRAIL_HXX_
#define THE_QT_TRAIL_HXX_

// local includes:
#include "ui/the_trail.hxx"

// Qt includes:
#include <QApplication>
#include <QObject>
#include <QEvent>
#include <QRect>
#include <QPoint>
#include <QSize>
#include <QTimer>
#include <QMouseEvent>
#include <QTabletEvent>
#include <QTimerEvent>
#include <QWheelEvent>
#include <QMoveEvent>
#include <QKeyEvent>
#include <QResizeEvent>
#include <QCloseEvent>
#include <QShortcutEvent>


/*
  from qapplication documentation:
  
  1. Reimplementing QApplication::notify virtual function is one of five
  ways to process an event. Very powerful, you get complete control,
  but of course only one subclass can be qApp.
  
  2. Installing an event filter on qApp. Such an event filter gets to
  process all events for all widgets, so it's just as powerful as
  reimplementing notify(), and in this way it's possible to have more
  than one application-global event filter. Global event filter get to
  see even mouse events for disabled widgets, and if global mouse
  tracking is enabled, mouse move events for all widgets.
  
  3. Reimplementing QObject::event() (as QWidget does). If you do this
  you get tab key-presses, and you get to see the events before any
  widget-specific event filters.
  
  4. Installing an event filter on the object. Such an event filter gets
  all the events except Tab and Shift-Tab key presses.
  
  5. Finally, reimplementing paintEvent(), mousePressEvent() and so
  on. This is the normal, easiest and least powerful way.
*/


//----------------------------------------------------------------
// QObjectTraits
// 
// The goal of QObjectTraits class is to help disambiguate between
// objects with identical path names. Additional information is used,
// such as class name and visibility to differentiate between objects.
// 
class QObjectTraits
{
public:
  QObjectTraits();
  QObjectTraits(const QObject * obj);
  QObjectTraits(const char * full_path,
		const char * class_name,
		const unsigned int & index,
		const bool & is_visible);
  QObjectTraits(const QObjectTraits & traits);
  ~QObjectTraits();
  
  QObjectTraits & operator = (const QObjectTraits & traits);
  
  // equality/inequality test operators:
  inline bool operator == (const QObjectTraits & traits) const
  { return object() == traits.object(); }
  
  inline bool operator != (const QObjectTraits & traits) const
  { return !(*this == traits); }
  
  // object accessor (will return NULL if the traits are not unique enough
  // to single out one object):
  QObject * object() const;
  
  // widget accessor:
  QWidget * widget() const;
  
  // const accessors:
  inline const QString * path() const
  { return path_; }
  
  inline const unsigned int & path_size() const
  { return path_size_; }
  
  inline const QString & class_name() const
  { return class_name_; }
  
  inline const unsigned int & index() const
  { return index_; }
  
  inline const bool & is_visible() const
  { return is_visible_; }
  
  // helper functions:
  static void
  convert_object_ptr_to_full_path(const QObject * object,
				  QString & full_path);
  
  static void
  split_the_path_into_components(const QString & full_path,
				 QList<QString> & path_names);
  
  void save(ostream & ostr) const;
  void load(istream & istr);
  
  static const std::list<char> & special_chars();
  
private:
  // helper functions:
  void init_path(const QString & full_path);
  void matching_objects(std::list<QObject *> & objects) const;
  
  // the object traits:
  QString *    path_;
  unsigned int path_size_;
  QString      class_name_;
  unsigned int index_;
  bool         is_visible_;
};

//----------------------------------------------------------------
// operator <<
// 
extern ostream &
operator << (ostream & ostr, const QObjectTraits & traits);

//----------------------------------------------------------------
// operator >>
// 
extern istream &
operator >> (istream & istr, QObjectTraits & traits);


//----------------------------------------------------------------
// the_traits_mapping_t
// 
class the_traits_mapping_t
{
public:
  the_traits_mapping_t():
    addr_(NULL),
    traits_()
  {}
  
  inline void init(void * addr, const QObjectTraits & traits)
  {
    addr_ = addr;
    traits_ = traits;
  }
  
  inline void * addr()
  { return addr_; }
  
  inline const QObjectTraits & traits() const
  { return traits_; }
  
private:
  void * addr_;
  QObjectTraits traits_;
};


//----------------------------------------------------------------
// the_qt_trail_t
//
// This class implements the first method described in the above
// event handling documentation excerpt.
// 
class the_qt_trail_t : public QApplication,
		       public the_trail_t
{
  Q_OBJECT
  
public:
  the_qt_trail_t(int & argc, char ** argv, bool record_by_default = false);
  ~the_qt_trail_t();
  
  // virtual:
  bool notify(QObject * object, QEvent * event);
  
  // virtual:
  void timeout();
  
protected:
  void customEvent(QEvent * event);
  
public slots:
  // virtual:
  void replay();
  void replay_done();
  void replay_one();

public:
  // virtual:
  void stop();
  
  // update the input devices (mouse, keyboard, tablet, etc...):
  void update_devices(QObject * object, const QEvent * event);
  
  void save_event(ostream &       ostr,
		  const QObject * object,
		  const QEvent *  event);
  
  bool load_event(istream &  istr,
		  QObject *& object,
		  QEvent *&  event);
  
  void saveQEvent(ostream & ostr,
		  const QObject * object,
		  const QEvent * event);
  
  void saveQMouseEvent(ostream & ostr,
		       const QObject * object,
		       const QMouseEvent * event);
  
  void saveQWheelEvent(ostream & ostr,
		       const QObject * object,
		       const QWheelEvent * event);
  
  void saveQKeyEvent(ostream & ostr,
		     const QObject * object,
		     const QKeyEvent * event);
  
  void saveQTabletEvent(ostream & ostr,
			const QObject * object,
			const QTabletEvent * event);
  
  void saveQMoveEvent(ostream & ostr,
		      const QObject * object,
		      const QMoveEvent * event);
  
  void saveQResizeEvent(ostream & ostr,
			const QObject * object,
			const QResizeEvent * event);
  
  void saveQCloseEvent(ostream & ostr,
		       const QObject * object,
		       const QCloseEvent * event);
  
  void saveQTimerEvent(ostream & ostr,
		       const QObject * object,
		       const QTimerEvent * event);
  
  void saveQShortcutEvent(ostream & ostr,
			  const QObject * object,
			  const QShortcutEvent * event);
  
  QEvent *         loadQEvent(istream & istr,         QEvent::Type t);
  QMouseEvent *    loadQMouseEvent(istream & istr,    QEvent::Type t);
  QWheelEvent *    loadQWheelEvent(istream & istr,    QEvent::Type t);
  QKeyEvent *      loadQKeyEvent(istream & istr,      QEvent::Type t);
  QTabletEvent *   loadQTabletEvent(istream & istr,   QEvent::Type t);
  QMoveEvent *     loadQMoveEvent(istream & istr,     QEvent::Type t);
  QResizeEvent *   loadQResizeEvent(istream & istr,   QEvent::Type t);
  QCloseEvent *    loadQCloseEvent(istream & istr,    QEvent::Type t);
  QTimerEvent *    loadQTimerEvent(istream & istr,    QEvent::Type t);
  QShortcutEvent * loadQShortcutEvent(istream & istr, QEvent::Type t);
  
  // trees used to map pointers to QObjects during trail playback/recording:
  the_bit_tree_t<the_traits_mapping_t> tree_load_;
  the_bit_tree_t<the_traits_mapping_t> tree_save_;
};


#endif // THE_QT_TRAIL_HXX_
