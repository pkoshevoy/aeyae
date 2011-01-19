// -*- Mode: c++; tab-width: 8; c-basic-offset: 2; indent-tabs-mode: nil -*-
// NOTE: the first line of this file sets up source code indentation rules
// for Emacs; it is also a hint to anyone modifying this file.

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
#include <QPoint>
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

// system includes:
#include <list>

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
  
  inline const std::size_t & path_size() const
  { return path_size_; }
  
  inline const QString & class_name() const
  { return class_name_; }
  
  inline const std::size_t & index() const
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
  
  void save(std::ostream & ostr) const;
  void load(std::istream & istr);
  
private:
  // helper functions:
  void init_path(const QString & full_path);
  void matching_objects(std::list<QObject *> & objects) const;
  
  // the object traits:
  QString *    path_;
  std::size_t  path_size_;
  QString      class_name_;
  std::size_t  index_;
  bool         is_visible_;
  
  static const char * special_chars_;
};

//----------------------------------------------------------------
// operator <<
// 
extern std::ostream &
operator << (std::ostream & ostr, const QObjectTraits & traits);

//----------------------------------------------------------------
// operator >>
// 
extern std::istream &
operator >> (std::istream & istr, QObjectTraits & traits);


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
  
  void save_event(std::ostream &  ostr,
		  const QObject * object,
		  const QEvent *  event);
  
  bool load_object(std::istream & istr);
  bool load_event(std::istream &  istr,
		  QObject *& object,
		  QEvent *&  event);
  
  void saveQEvent(std::ostream & ostr,
		  const QObject * object,
		  const QEvent * event);
  
  void saveQMouseEvent(std::ostream & ostr,
		       const QObject * object,
		       const QMouseEvent * event);
  
  void saveQWheelEvent(std::ostream & ostr,
		       const QObject * object,
		       const QWheelEvent * event);
  
  void saveQKeyEvent(std::ostream & ostr,
		     const QObject * object,
		     const QKeyEvent * event);
  
  void saveQTabletEvent(std::ostream & ostr,
			const QObject * object,
			const QTabletEvent * event);
  
  void saveQMoveEvent(std::ostream & ostr,
		      const QObject * object,
		      const QMoveEvent * event);
  
  void saveQResizeEvent(std::ostream & ostr,
			const QObject * object,
			const QResizeEvent * event);
  
  void saveQCloseEvent(std::ostream & ostr,
		       const QObject * object,
		       const QCloseEvent * event);
  
  void saveQTimerEvent(std::ostream & ostr,
		       const QObject * object,
		       const QTimerEvent * event);
  
  void saveQShortcutEvent(std::ostream & ostr,
			  const QObject * object,
			  const QShortcutEvent * event);
  
  QEvent *         loadQEvent(std::istream & istr,         QEvent::Type t);
  QMouseEvent *    loadQMouseEvent(std::istream & istr,    QEvent::Type t);
  QWheelEvent *    loadQWheelEvent(std::istream & istr,    QEvent::Type t);
  QKeyEvent *      loadQKeyEvent(std::istream & istr,      QEvent::Type t);
  QTabletEvent *   loadQTabletEvent(std::istream & istr,   QEvent::Type t);
  QMoveEvent *     loadQMoveEvent(std::istream & istr,     QEvent::Type t);
  QResizeEvent *   loadQResizeEvent(std::istream & istr,   QEvent::Type t);
  QCloseEvent *    loadQCloseEvent(std::istream & istr,    QEvent::Type t);
  QTimerEvent *    loadQTimerEvent(std::istream & istr,    QEvent::Type t);
  QShortcutEvent * loadQShortcutEvent(std::istream & istr, QEvent::Type t);
  
  // trees used to map pointers to QObjects during trail playback/recording:
  the_bit_tree_t<the_traits_mapping_t> tree_load_;
  the_bit_tree_t<the_traits_mapping_t> tree_save_;
};

class QSize;
extern bool save(std::ostream & stream, const QSize & data);
extern bool load(std::istream & stream, QSize & data);

class QRect;
extern bool save(std::ostream & stream, const QRect & data);
extern bool load(std::istream & stream, QRect & data);


//----------------------------------------------------------------
// the_qt_signal_blocker_t
// 
struct the_qt_signal_blocker_t
{
  the_qt_signal_blocker_t(QObject * obj = NULL);
  ~the_qt_signal_blocker_t();

  the_qt_signal_blocker_t & operator << (QObject * obj);
  
  inline the_qt_signal_blocker_t & operator << (QObject & obj)
  { return *this << &obj; }
  
private:
  the_qt_signal_blocker_t(const the_qt_signal_blocker_t &);
  the_qt_signal_blocker_t & operator = (const the_qt_signal_blocker_t &);
  
  std::list<QObject *> blocked_;
};


#endif // THE_QT_TRAIL_HXX_
