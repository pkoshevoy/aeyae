// -*- Mode: c++; tab-width: 8; c-basic-offset: 2; indent-tabs-mode: nil -*-
// NOTE: the first line of this file sets up source code indentation rules
// for Emacs; it is also a hint to anyone modifying this file.

// Created   : Sun Aug  2 21:44:50 MDT 2020
// Copyright : Pavel Koshevoy
// License   : MIT -- http://www.opensource.org/licenses/mit-license.php

#ifndef YAE_APPLICATION_H_
#define YAE_APPLICATION_H_

// std includes:
#include <string>

// Qt includes:
#include <QApplication>
#include <QEvent>
#include <QString>


#if (QT_VERSION >= QT_VERSION_CHECK(4, 6, 0) && \
     QT_VERSION < QT_VERSION_CHECK(5, 0, 0))
#define YAE_QT4 1
#define YAE_QT5 0
#else
#define YAE_QT4 0
#define YAE_QT5 1
#endif

#if YAE_QT4
#include <QDesktopServices>
#elif YAE_QT5
#include <QStandardPaths>
#endif


//----------------------------------------------------------------
// YAE_STANDARD_LOCATION
//
#if YAE_QT4
#define YAE_STANDARD_LOCATION(x) \
  QDesktopServices::storageLocation(QDesktopServices::x)
#endif

//----------------------------------------------------------------
// YAE_STANDARD_LOCATION
//
#if YAE_QT5
#define YAE_STANDARD_LOCATION(x) \
  QStandardPaths::writableLocation(QStandardPaths::x)
#endif


namespace yae
{
    //----------------------------------------------------------------
  // kOrganization
  //
  // default: PavelKoshevoy, except on mac it's sourceforge.net
  //
  extern QString kOrganization;

  //----------------------------------------------------------------
  // kApplication
  //
  // default: ApprenticeVideo
  //
  extern QString kApplication;


  //----------------------------------------------------------------
  // Application
  //
  // Application
  //
  class Application : public QApplication
  {
    Q_OBJECT;

  public:
    Application(int & argc, char ** argv);
    ~Application();

    static Application & singleton();

    // virtual: overridden to propagate custom events to the parent:
    bool notify(QObject * receiver, QEvent * event);

  signals:
    // emitted when QFileOpenEvent is processed:
    void file_open(const QString & filename);

    // emitted on AppleInterfaceThemeChangedNotification, etc...
    void theme_changed(const yae::Application & app);

  protected:
    // virtual: overridden to hadle QFileOpenEvent:
    bool event(QEvent * event);

  public:
    inline void set_appearance(const char * appearance)
    { set_appearance(std::string(appearance ? appearance : "auto")); }

    void set_appearance(const std::string & appearance);

    bool query_dark_mode() const;

    // platform specific implementation details:
    struct Private
    {
      virtual ~Private() {}
      virtual bool query_dark_mode() const = 0;
    };

    Private * private_;
    std::string appearance_;
  };


  //----------------------------------------------------------------
  // QueuedCallEvent
  //
  struct QueuedCallEvent : public QEvent
  {
    QueuedCallEvent(): QEvent(QEvent::User) {}
    virtual void execute() = 0;
  };

  //----------------------------------------------------------------
  // QueuedCall
  //
  template <typename TObj, typename TFunc>
  struct QueuedCall : public QueuedCallEvent
  {
    QueuedCall(TObj & obj,
               TFunc TObj::* const func):
      obj_(obj),
      func_(func)
    {}

    // virtual:
    void execute()
    {
      (obj_.*func_)();
    }

    TObj & obj_;
    TFunc TObj::* const func_;
  };

  //----------------------------------------------------------------
  // queue_call
  //
  template <typename TObj, typename TFunc>
  void
  queue_call(TObj & obj, TFunc TObj::* const func)
  {
    qApp->postEvent(&obj, new QueuedCall<TObj, TFunc>(obj, func));
  }

  //----------------------------------------------------------------
  // QueuedCallArgs1
  //
  template <typename TObj, typename TFunc, typename TArg1>
  struct QueuedCallArgs1 : public QueuedCallEvent
  {
    QueuedCallArgs1(TObj & obj,
                    TFunc TObj::* const func,
                    TArg1 arg1):
      obj_(obj),
      func_(func),
      arg1_(arg1)
    {}

    // virtual:
    void execute()
    {
      (obj_.*func_)(arg1_);
    }

    TObj & obj_;
    TFunc TObj::* const func_;
    TArg1 arg1_;
  };

  //----------------------------------------------------------------
  // queue_call
  //
  template <typename TObj, typename TFunc, typename TArg1>
  void
  queue_call(TObj & obj,
             TFunc TObj::* const func,
             TArg1 arg1)
  {
    qApp->postEvent
      (&obj, new QueuedCallArgs1<TObj, TFunc, TArg1>(obj, func, arg1));
  }

  //----------------------------------------------------------------
  // QueuedCallArgs1
  //
  template <typename TObj, typename TFunc, typename TArg1, typename TArg2>
  struct QueuedCallArgs2 : public QueuedCallEvent
  {
    QueuedCallArgs2(TObj & obj,
                    TFunc TObj::* const func,
                    TArg1 arg1,
                    TArg2 arg2):
      obj_(obj),
      func_(func),
      arg1_(arg1),
      arg2_(arg2)
    {}

    // virtual:
    void execute()
    {
      (obj_.*func_)(arg1_, arg2_);
    }

    TObj & obj_;
    TFunc TObj::* const func_;
    TArg1 arg1_;
    TArg2 arg2_;
  };

  //----------------------------------------------------------------
  // queue_call
  //
  template <typename TObj, typename TFunc, typename TArg1, typename TArg2>
  void
  queue_call(TObj & obj,
             TFunc TObj::* const func,
             TArg1 arg1,
             TArg2 arg2)
  {
    qApp->postEvent
      (&obj,
       new QueuedCallArgs2<TObj, TFunc, TArg1, TArg2>
       (obj, func, arg1, arg2));
  }

  //----------------------------------------------------------------
  // handle_queued_call_event
  //
  bool
  handle_queued_call_event(QEvent * event);

  //----------------------------------------------------------------
  // ThemeChangedEvent
  //
  struct ThemeChangedEvent : public QEvent
  {
    ThemeChangedEvent(): QEvent(QEvent::User) {}
  };

}


#endif // YAE_APPLICATION_H_
