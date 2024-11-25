// -*- Mode: c++; tab-width: 8; c-basic-offset: 2; indent-tabs-mode: nil -*-
// NOTE: the first line of this file sets up source code indentation rules
// for Emacs; it is also a hint to anyone modifying this file.

// Created   : Fri Jul 24 22:10:26 PDT 2015
// Copyright : Pavel Koshevoy
// License   : MIT -- http://www.opensource.org/licenses/mit-license.php

// aeyae:
#include "yae/api/yae_assert.h"

// system:
#ifdef _WIN32
#include <windows.h>
#endif

#ifdef __APPLE__
#include <AvailabilityMacros.h>
#include <CoreServices/CoreServices.h>
#include <IOKit/pwr_mgt/IOPMLib.h>
#elif !defined(_WIN32)
#include <QtDBus/QtDBus>
#endif

// standard:
#include <iostream>

// Qt:
#include <QApplication>
#include <QTimer>

// yaeui:
#include "yaeScreenSaverInhibitor.h"


namespace yae
{
  static const int inhibit_interval_msec = 29000;
  static const int uninhibit_interval_msec = 59000;

  //----------------------------------------------------------------
  // ScreenSaverInhibitor::ScreenSaverInhibitor
  //
  ScreenSaverInhibitor::ScreenSaverInhibitor():
    cookie_(0)
  {
    timerScreenSaver_.setSingleShot(true);
    timerScreenSaver_.setInterval(inhibit_interval_msec);

    timerScreenSaverUnInhibit_.setSingleShot(true);
    timerScreenSaverUnInhibit_.setInterval(uninhibit_interval_msec);

    bool ok = true;
    ok = connect(&timerScreenSaver_, SIGNAL(timeout()),
                 this, SLOT(screenSaverInhibit()));
    YAE_ASSERT(ok);

    ok = connect(&timerScreenSaverUnInhibit_, SIGNAL(timeout()),
                 this, SLOT(screenSaverUnInhibit()));
    YAE_ASSERT(ok);
  }

  //----------------------------------------------------------------
  // ScreenSaverInhibitor::screenSaverInhibit
  //
  void
  ScreenSaverInhibitor::screenSaverInhibit()
  {
#ifdef __APPLE__

#  if (MAC_OS_X_VERSION_MIN_REQUIRED < 1070)
    UpdateSystemActivity(UsrActivity);
#  else
    static const CFStringRef assertion_name = CFSTR("video playback");
    IOReturn r = kIOReturnSuccess;

    if (cookie_)
    {
      r = IOPMAssertionRelease(cookie_);
      YAE_ASSERT(r == kIOReturnSuccess);
      cookie_ = 0;
    }

    r =
      IOPMAssertionCreateWithDescription
      (kIOPMAssertPreventUserIdleDisplaySleep,
       assertion_name,
       NULL, // CFStringRef Details
       NULL, // CFStringRef HumanReadableReason
       NULL, // CFStringRef LocalizationBundlePath
       (inhibit_interval_msec * 1e-3), // CFTimeInterval Timeout
       NULL, // CFStringRef TimeoutAction
       &cookie_);

    YAE_ASSERT(r == kIOReturnSuccess);
#  endif

#elif defined(_WIN32)
    // http://www.codeproject.com/KB/system/disablescreensave.aspx
    //
    // Call the SystemParametersInfo function to query and reset the
    // screensaver time-out value.  Use the user's default settings
    // in case your application terminates abnormally.
    //

    static UINT spiGetter[] = { SPI_GETLOWPOWERTIMEOUT,
                                SPI_GETPOWEROFFTIMEOUT,
                                SPI_GETSCREENSAVETIMEOUT };

    static UINT spiSetter[] = { SPI_SETLOWPOWERTIMEOUT,
                                SPI_SETPOWEROFFTIMEOUT,
                                SPI_SETSCREENSAVETIMEOUT };

    std::size_t numParams = sizeof(spiGetter) / sizeof(spiGetter[0]);
    for (std::size_t i = 0; i < numParams; i++)
    {
      UINT val = 0;
      BOOL ok = SystemParametersInfo(spiGetter[i], 0, &val, 0);
      YAE_ASSERT(ok);

      if (ok)
      {
        ok = SystemParametersInfo(spiSetter[i], val, NULL, 0);
        YAE_ASSERT(ok);
      }
    }

#else
    // try using DBUS to talk to the screensaver...
    bool done = false;

    if (QDBusConnection::sessionBus().isConnected())
    {
      QDBusInterface screensaver("org.freedesktop.ScreenSaver",
                                 "/ScreenSaver");
      if (screensaver.isValid())
      {
        // apparently SimulateUserActivity is not enough to keep Ubuntu
        // from starting the screensaver
        screensaver.call(QDBus::NoBlock, "SimulateUserActivity");

        // try to inhibit the screensaver as well:
        if (!cookie_)
        {
          QDBusMessage out =
            screensaver.call(QDBus::Block,
                             "Inhibit",
                             QVariant(QApplication::applicationName()),
                             QVariant("video playback"));

          if (out.type() == QDBusMessage::ReplyMessage &&
              !out.arguments().empty())
          {
            cookie_ = out.arguments().front().toUInt();
          }
        }

        if (cookie_)
        {
          timerScreenSaverUnInhibit_.start();
        }

        done = true;
      }
    }

    if (!done)
    {
      // FIXME: not sure how to do this yet
      yae_debug << "screenSaverInhibit";
    }
#endif
  }

  //----------------------------------------------------------------
  // ScreenSaverInhibitor::screenSaverUnInhibit
  //
  void
  ScreenSaverInhibitor::screenSaverUnInhibit()
  {
#if !defined(__APPLE__) && !defined(_WIN32)
    if (cookie_ && QDBusConnection::sessionBus().isConnected())
    {
      QDBusInterface screensaver("org.freedesktop.ScreenSaver",
                                 "/ScreenSaver");
      if (screensaver.isValid())
      {
        screensaver.call(QDBus::NoBlock, "UnInhibit", QVariant(cookie_));
        cookie_ = 0;
      }
    }
#endif
  }

}
