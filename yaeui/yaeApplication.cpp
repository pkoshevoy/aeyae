// -*- Mode: c++; tab-width: 8; c-basic-offset: 2; indent-tabs-mode: nil -*-
// NOTE: the first line of this file sets up source code indentation rules
// for Emacs; it is also a hint to anyone modifying this file.

// Created   : Mon Aug  3 19:16:19 MDT 2020
// Copyright : Pavel Koshevoy
// License   : MIT -- http://www.opensource.org/licenses/mit-license.php

// standard:
#include <set>

// Qt:
#include <QDir>
#include <QFileOpenEvent>
#include <QMouseEvent>

// aeyae:
#include "yae/api/yae_assert.h"

// yaeui:
#ifdef __APPLE__
#include "yaeAppleUtils.h"
#endif
#include "yaeApplication.h"


namespace yae
{

  //----------------------------------------------------------------
  // Application::Application
  //
  Application::Application(int & argc, char ** argv):
    QApplication(argc, argv),
    private_(NULL)
  {
#ifdef __APPLE__
    QString appDir = QApplication::applicationDirPath();
    QString plugInsDir = QDir::cleanPath(appDir + "/../PlugIns");
    QApplication::addLibraryPath(plugInsDir);

    private_ = new AppleApp();
#endif
  }

  //----------------------------------------------------------------
  // Application::~Application
  //
  Application::~Application()
  {
    if (private_)
    {
      delete private_;
      private_ = NULL;
    }
  }

  //----------------------------------------------------------------
  // Application::singleton
  //
  Application &
  Application::singleton()
  {
    Application * yae_app = dynamic_cast<Application *>(qApp);
    YAE_THROW_IF(!yae_app);
    return *yae_app;
  }

  //----------------------------------------------------------------
  // Application::notify
  //
  bool
  Application::notify(QObject * receiver, QEvent * event)
  {
    YAE_ASSERT(receiver && event);
    bool result = false;

    QEvent::Type et = event ? event->type() : QEvent::None;
    if (et >= QEvent::User)
    {
      if (yae::handle_queued_call_event(event))
      {
        return true;
      }

      event->ignore();
    }

    while (receiver)
    {
      result = QApplication::notify(receiver, event);

#if 0 // ndef NDEBUG
      if (et == QEvent::MouseButtonPress ||
          et == QEvent::MouseButtonRelease ||
          et == QEvent::MouseButtonDblClick ||
          et == QEvent::MouseMove)
      {
        QMouseEvent * e = static_cast<QMouseEvent *>(event);
        yae_warn
          << "QApplication::notify(" << receiver << ", " << e << ") = "
          << result
          << ", receiver \"" << receiver->objectName().toUtf8().constData()
          << "\", " << e->button() << " button, "
          << e->buttons() << " buttons, spontaneous: "
          << (e->spontaneous() ? "true" : "false")
          << ", " << yae::to_str(et)
          << ", " << (e->isAccepted() ? "accept" : "ignore");
      }
#endif
      if (et < QEvent::User || (result && event->isAccepted()))
      {
        break;
      }

      receiver = receiver->parent();
    }

    return result;
  }

  //----------------------------------------------------------------
  // Application::event
  //
  bool
  Application::event(QEvent * event)
  {
    QEvent::Type et = event ? event->type() : QEvent::None;
    if (et == QEvent::FileOpen)
    {
      // handle the apple event to open a document:
      QFileOpenEvent * e = static_cast<QFileOpenEvent *>(event);
      QString filename = e->file();
      emit file_open(filename);

      e->accept();
      return true;
    }

    if (et == QEvent::User)
    {
      ThemeChangedEvent * theme_changed_event =
        dynamic_cast<ThemeChangedEvent *>(event);

      if (theme_changed_event)
      {
        emit theme_changed(*this);
        event->accept();
        return true;
      }
    }

    return QApplication::event(event);
  }

  //----------------------------------------------------------------
  // Application::set_appearance
  //
  void
  Application::set_appearance(const std::string & appearance)
  {
    if (appearance_ == appearance)
    {
      return;
    }

    appearance_ = appearance;
    emit theme_changed(*this);
  }

  //----------------------------------------------------------------
  // Application::query_dark_mode
  //
  bool
  Application::query_dark_mode() const
  {
    bool dark = false;

    if (appearance_ == "dark")
    {
      return true;
    }

    if (appearance_ == "light")
    {
      return false;
    }

    if (private_)
    {
      dark = private_->query_dark_mode();
    }
#ifndef __APPLE__
    else
    {
      dark = true;
    }
#endif

    return dark;
  }

}
