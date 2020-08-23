// -*- Mode: c++; tab-width: 8; c-basic-offset: 2; indent-tabs-mode: nil -*-
// NOTE: the first line of this file sets up source code indentation rules
// for Emacs; it is also a hint to anyone modifying this file.

// Created      : Sat Jan 13 15:53:35 MST 2018
// Copyright    : Pavel Koshevoy
// License      : MIT -- http://www.opensource.org/licenses/mit-license.php

// system includes:
#include <iostream>
#include <sstream>
#include <list>
#include <math.h>

// boost includes:
#include <boost/algorithm/string.hpp>

// Qt includes:
#include <QApplication>
#include <QCloseEvent>
#include <QDragEnterEvent>
#include <QFileDialog>
#include <QFileInfo>
#include <QMimeData>
#include <QShortcut>
#include <QUrl>
#include <QVBoxLayout>

// yae includes:
#include "yae/api/yae_version.h"
#include "yae/ffmpeg/yae_remux.h"

// local includes:
#include "yaeMainWindow.h"
#include "yaeUtilsQt.h"


namespace yae
{

  //----------------------------------------------------------------
  // AboutDialog::AboutDialog
  //
  AboutDialog::AboutDialog(QWidget * parent):
    QDialog(parent),
    Ui::AboutDialog()
  {
    Ui::AboutDialog::setupUi(this);

    textBrowser->setSearchPaths(QStringList() << ":/");
    textBrowser->setSource(QUrl("qrc:///yaeAbout.html"));
  }


  //----------------------------------------------------------------
  // context_toggle_fullscreen
  //
  static void
  context_toggle_fullscreen(void * context)
  {
    MainWindow * mainWindow = (MainWindow *)context;
    mainWindow->requestToggleFullScreen();
  }


  //----------------------------------------------------------------
  // context_query_fullscreen
  //
  static bool
  context_query_fullscreen(void * context, bool & fullscreen)
  {
    MainWindow * mainWindow = (MainWindow *)context;
    fullscreen = mainWindow->isFullScreen();
    return true;
  }


  //----------------------------------------------------------------
  // MainWindow::MainWindow
  //
  MainWindow::MainWindow():
    QMainWindow(NULL, 0),
    canvasWidget_(NULL),
    canvas_(NULL)
  {
    setupUi(this);
    setAcceptDrops(true);

    contextMenu_ = new QMenu(this);
    contextMenu_->setObjectName(QString::fromUtf8("contextMenu_"));

#if !defined(__APPLE__) && !defined(_WIN32)
    QString fnIcon =
      QString::fromUtf8(":/images/aeyae-remux-logo.png");
    this->setWindowIcon(QIcon(fnIcon));
#endif

    QVBoxLayout * canvasLayout = new QVBoxLayout(canvasContainer_);
    canvasLayout->setMargin(0);
    canvasLayout->setSpacing(0);

    // setup the canvas widget (QML quick widget):
#ifdef __APPLE__
    QString clickOrTap = tr("click");
#else
    QString clickOrTap = tr("tap");
#endif

    QString greeting = tr("drop video files here");

#ifdef YAE_USE_QOPENGL_WIDGET
    canvasWidget_ = new TCanvasWidget(this);
    canvasWidget_->setUpdateBehavior(QOpenGLWidget::NoPartialUpdate);
#else
    // request vsync if available:
    QGLFormat contextFormat;
    contextFormat.setSwapInterval(1);
    contextFormat.setSampleBuffers(false);
    canvasWidget_ = new TCanvasWidget(contextFormat, this, canvasWidget_);
#endif
    canvasWidget_->setGreeting(greeting);

    view_.toggle_fullscreen_.reset(&context_toggle_fullscreen, this);
    view_.query_fullscreen_.reset(&context_query_fullscreen, this);

    canvasWidget_->setFocusPolicy(Qt::StrongFocus);
    canvasWidget_->setAcceptDrops(true);

    // insert canvas widget into the main window layout:
    canvasLayout->addWidget(canvasWidget_);

#if 1
    actionFullScreen->setShortcut(tr("Ctrl+F"));
#elif defined(__APPLE__)
    actionFullScreen->setShortcut(tr("Ctrl+Shift+F"));
#else
    actionFullScreen->setShortcut(tr("F11"));
#endif

    // when in fullscreen mode the menubar is hidden and all actions
    // associated with it stop working (tested on OpenSUSE 11.4 KDE 4.6),
    // so I am creating these shortcuts as a workaround:
    shortcutSave_ = new QShortcut(this);
    shortcutExit_ = new QShortcut(this);
    shortcutFullScreen_ = new QShortcut(this);

    shortcutSave_->setContext(Qt::ApplicationShortcut);
    shortcutExit_->setContext(Qt::ApplicationShortcut);
    shortcutFullScreen_->setContext(Qt::ApplicationShortcut);

    // when in fullscreen mode the menubar is hidden and all actions
    // associated with it stop working (tested on OpenSUSE 11.4 KDE 4.6),
    // so I am creating these shortcuts as a workaround:
    bool ok = true;

    ok = connect(actionOpen, SIGNAL(triggered()),
                 this, SLOT(fileOpen()));
    YAE_ASSERT(ok);

    ok = connect(actionSave, SIGNAL(triggered()),
                 this, SLOT(fileSave()));
    YAE_ASSERT(ok);

    ok = connect(shortcutSave_, SIGNAL(activated()),
                 actionSave, SLOT(trigger()));
    YAE_ASSERT(ok);

    ok = connect(actionSaveAs, SIGNAL(triggered()),
                 this, SLOT(fileSaveAs()));
    YAE_ASSERT(ok);

    ok = connect(actionImport, SIGNAL(triggered()),
                 this, SLOT(fileImport()));
    YAE_ASSERT(ok);

    ok = connect(actionExport, SIGNAL(triggered()),
                 this, SLOT(fileExport()));
    YAE_ASSERT(ok);

    ok = connect(&view_, SIGNAL(remux()),
                 this, SLOT(fileExport()));
    YAE_ASSERT(ok);

    ok = connect(actionExit, SIGNAL(triggered()),
                 this, SLOT(fileExit()));
    YAE_ASSERT(ok);

    ok = connect(shortcutExit_, SIGNAL(activated()),
                 actionExit, SLOT(trigger()));
    YAE_ASSERT(ok);

    ok = connect(actionFullScreen, SIGNAL(triggered()),
                 this, SLOT(enterFullScreen()));
    YAE_ASSERT(ok);

    ok = connect(shortcutFullScreen_, SIGNAL(activated()),
                 actionFullScreen, SLOT(trigger()));
    YAE_ASSERT(ok);

    ok = connect(actionAbout, SIGNAL(triggered()),
                 this, SLOT(helpAbout()));
    YAE_ASSERT(ok);

    ok = connect(&(canvasWidget_->sigs_), SIGNAL(toggleFullScreen()),
                 this, SLOT(requestToggleFullScreen()));
    YAE_ASSERT(ok);
  }

  //----------------------------------------------------------------
  // MainWindow::~MainWindow
  //
  MainWindow::~MainWindow()
  {
    canvas_->cropAutoDetectStop();
    delete canvasWidget_;
  }

  //----------------------------------------------------------------
  // MainWindow::canvas
  //
  Canvas *
  MainWindow::canvas() const
  {
    return canvas_;
  }

  //----------------------------------------------------------------
  // MainWindow::initCanvasWidget
  //
  void
  MainWindow::initCanvasWidget()
  {
    // get a shortcut to the Canvas (owned by the QML canvas widget):
    canvas_ = canvasWidget_;
    YAE_ASSERT(canvas_);
  }

  //----------------------------------------------------------------
  // MainWindow::initItemViews
  //
  void
  MainWindow::initItemViews()
  {
    canvasWidget_->append(&view_);

    view_.setModel(&model_);
    view_.setEnabled(true);
    view_.layoutChanged();

    spinner_.toggle_fullscreen_.reset(&context_toggle_fullscreen, this);
    spinner_.query_fullscreen_.reset(&context_query_fullscreen, this);

    canvasWidget_->append(&spinner_);
    spinner_.setStyle(view_.style());
    spinner_.setEnabled(false);
  }


  //----------------------------------------------------------------
  // LoaderProgressObserver
  //
  struct LoaderProgressObserver : rx::Loader::IProgressObserver
  {
    //----------------------------------------------------------------
    // Began
    //
    struct Began : public QEvent
    {
      Began(const std::string & source):
        QEvent(QEvent::User),
        source_(source)
      {}

      std::string source_;
    };

    //----------------------------------------------------------------
    // Loaded
    //
    struct Loaded : public QEvent
    {
      Loaded(const std::string & source,
             const TDemuxerInterfacePtr & demuxer,
             const TClipPtr & clip):
        QEvent(QEvent::User),
        source_(source),
        demuxer_(demuxer),
        clip_(clip)
      {}

      std::string source_;
      TDemuxerInterfacePtr demuxer_;
      TClipPtr clip_;
    };

    //----------------------------------------------------------------
    // Done
    //
    struct Done : public QEvent
    {
      Done(): QEvent(QEvent::User) {}
    };

    //----------------------------------------------------------------
    // LoaderProgressObserver
    //
    LoaderProgressObserver(QObject * target):
      target_(target)
    {}

    // virtual:
    void began(const std::string & source)
    {
      qApp->postEvent(target_, new Began(source));
    }

    // virtual:
    void loaded(const std::string & source,
                const TDemuxerInterfacePtr & demuxer,
                const TClipPtr & clip)
    {
      qApp->postEvent(target_, new Loaded(source, demuxer, clip));
    }

    // virtual:
    void done()
    {
      qApp->postEvent(target_, new Done());
    }

  protected:
    QObject * target_;
  };

  //----------------------------------------------------------------
  // MainWindow::add
  //
  void
  MainWindow::add(const std::set<std::string> & sources,
                  const std::list<ClipInfo> & src_clips)
  {
    rx::Loader::TProgressObserverPtr cb(new LoaderProgressObserver(this));
    TAsyncTaskPtr t(new rx::Loader(model_.demuxer_, sources, src_clips, cb));
    tasks_.push_back(t);
    async_.push_back(t);
  }

  //----------------------------------------------------------------
  // MainWindow::load
  //
  bool
  MainWindow::load(const QString & path)
  {
    std::set<std::string> sources;
    sources.insert(std::string(path.toUtf8().constData()));
    add(sources);
    return true;
  }

  //----------------------------------------------------------------
  // MainWindow::loadFile
  //
  void
  MainWindow::loadFile(const QString & filename)
  {
    if (filename.endsWith(".yaerx", Qt::CaseInsensitive))
    {
      this->fileOpen(filename);
    }
    else
    {
      std::string source = filename.toUtf8().constData();
      std::set<std::string> sources;
      sources.insert(source);
      this->add(sources);
    }
  }

  //----------------------------------------------------------------
  // MainWindow::fileOpen
  //
  void
  MainWindow::fileOpen()
  {
    static const QString filter = tr("Aeyae Remux (*.yaerx)");

    QString startHere = YAE_STANDARD_LOCATION(MoviesLocation);
    startHere = yae::get(startHere_, "docs", startHere);

#ifndef __APPLE__
    QString filename = QFileDialog::getOpenFileName(this,
                                                    tr("Open document"),
                                                    startHere,
                                                    filter);
#else
    QFileDialog dialog(this, tr("Open document"), startHere, filter);
    int r = dialog.exec();
    if (r != QDialog::Accepted)
    {
      return;
    }

    QStringList filenames = dialog.selectedFiles();

    if (filenames.empty())
    {
      return;
    }

    QString filename = filenames.back();
#endif

    if (filename.isEmpty())
    {
      return;
    }

    put(startHere_, "docs", QFileInfo(filename).absoluteDir().canonicalPath());
    fileOpen(filename);
  }

  //----------------------------------------------------------------
  // MainWindow::fileOpen
  //
  void
  MainWindow::fileOpen(const QString & filename)
  {
    std::string json_str =
      TOpenFile(filename.toUtf8().constData(), "rb").read();

    std::set<std::string> sources;
    std::list<ClipInfo> src_clips;

    if (RemuxModel::parse_json_str(json_str, sources, src_clips))
    {
      filename_ = filename;
      model_ = RemuxModel();
      view_.selected_ = 0;
      view_.layoutChanged();
      this->add(sources, src_clips);
    }
  }

  //----------------------------------------------------------------
  // MainWindow::fileSave
  //
  void
  MainWindow::fileSave()
  {
    if (filename_.isEmpty())
    {
      fileSaveAs();
    }
    else
    {
      fileSave(filename_);
    }
  }

  //----------------------------------------------------------------
  // MainWindow::fileSave
  //
  void
  MainWindow::fileSave(const QString & filename)
  {
    filename_ = filename;
    std::string json_str = model_.to_json_str();
    bool ok = TOpenFile(filename.toUtf8().constData(), "wb").write(json_str);
    YAE_ASSERT(ok);
  }

  //----------------------------------------------------------------
  // MainWindow::fileSaveAs
  //
  void
  MainWindow::fileSaveAs()
  {
    static const QString filter = tr("Aeyae Remux (*.yaerx)");

    QString startHere = YAE_STANDARD_LOCATION(MoviesLocation);
    startHere = yae::get(startHere_, "docs", startHere);

#ifndef __APPLE__
    QString filename = QFileDialog::getSaveFileName(this,
                                                    tr("Save As"),
                                                    startHere,
                                                    filter);
#else
    QFileDialog dialog(this, tr("Save As"), startHere, filter);
    dialog.setAcceptMode(QFileDialog::AcceptSave);
    int r = dialog.exec();
    if (r != QDialog::Accepted)
    {
      return;
    }

    QStringList filenames = dialog.selectedFiles();
    if (filenames.empty())
    {
      return;
    }

    QString filename = filenames.back();
#endif

    if (filename.isEmpty())
    {
      return;
    }

    static const QString doc_suffix = tr(".yaerx");
    if (!filename.endsWith(doc_suffix, Qt::CaseInsensitive))
    {
      filename += doc_suffix;
    }

    put(startHere_, "docs", QFileInfo(filename).absoluteDir().canonicalPath());
    fileSave(filename);
  }

  //----------------------------------------------------------------
  // MainWindow::fileImport
  //
  void
  MainWindow::fileImport()
  {
    static const QString filter =
      tr("movies ("
         "*.avi "
         "*.asf "
         "*.divx "
         "*.eyetv "
         "*.flv "
         "*.f4v "
         "*.m2t "
         "*.m2ts "
         "*.m4v "
         "*.mkv "
         "*.mod "
         "*.mov "
         "*.mpg "
         "*.mp4 "
         "*.mpeg "
         "*.mpts "
         "*.ogm "
         "*.ogv "
         "*.ts "
         "*.wmv "
         "*.webm "
         ")");

    QString startHere = YAE_STANDARD_LOCATION(MoviesLocation);
    startHere = yae::get(startHere_, "import", startHere);

#ifndef __APPLE__
    QStringList filenames =
      QFileDialog::getOpenFileNames(this,
                                    tr("Select one or more files"),
                                    startHere,
                                    filter);
#else
    QFileDialog dialog(this,
                       tr("Select one or more files"),
                       startHere,
                       filter);
    int r = dialog.exec();
    if (r != QDialog::Accepted)
    {
      return;
    }

    QStringList filenames = dialog.selectedFiles();
#endif

    if (filenames.empty())
    {
      return;
    }

    put(startHere_, "import",
        QFileInfo(filenames.back()).absoluteDir().canonicalPath());

    std::set<std::string> sources;
    for (QStringList::const_iterator i = filenames.begin();
         i != filenames.end(); ++i)
    {
      const QString & source = *i;
      sources.insert(std::string(source.toUtf8().constData()));
    }

    add(sources);
  }

  //----------------------------------------------------------------
  // ExportTask
  //
  struct ExportTask : AsyncTaskQueue::Task
  {

    //----------------------------------------------------------------
    // ExportTask
    //
    ExportTask(QObject * target,
               const RemuxModel & source,
               const QString & output):
      target_(target),
      source_(source),
      output_(output)
    {}

    //----------------------------------------------------------------
    // Began
    //
    struct Began : public QEvent
    {
      Began(const QString & filename):
        QEvent(QEvent::User),
        filename_(filename)
      {}

      QString filename_;
    };

    //----------------------------------------------------------------
    // Done
    //
    struct Done : public QEvent
    {
      Done(): QEvent(QEvent::User) {}
    };

    //----------------------------------------------------------------
    // run
    //
    void run()
    {
      try
      {
        qApp->postEvent(target_, new Began(QFileInfo(output_).fileName()));
        TSerialDemuxerPtr demuxer = source_.make_serial_demuxer();

        std::string fn = output_.toUtf8().constData();
        yae::remux(fn.c_str(), *demuxer);
      }
      catch (const std::exception & e)
      {
       yae_wlog("ExportTask::run exception: %s", e.what());
      }
      catch (...)
      {
       yae_wlog("ExportTask::run unknown exception");
      }

      qApp->postEvent(target_, new Done());
    }

    QObject * target_;
    RemuxModel source_;
    QString output_;
  };

  //----------------------------------------------------------------
  // MainWindow::fileExport
  //
  void
  MainWindow::fileExport()
  {
    static const QString filter =
      tr("movies ("
         "*.avi "
         "*.mkv "
         "*.mov "
         "*.mp4 "
         "*.nut "
         "*.ogm "
         "*.ts "
         ")");

    QString startHere = YAE_STANDARD_LOCATION(MoviesLocation);
    startHere = yae::get(startHere_, "export", startHere);

#ifndef __APPLE__
    QString filename = QFileDialog::getSaveFileName(this,
                                                    tr("Export As"),
                                                    startHere,
                                                    filter);
#else
    QFileDialog dialog(this,
                       tr("Export As"),
                       startHere,
                       filter);
    dialog.setAcceptMode(QFileDialog::AcceptSave);
    int r = dialog.exec();
    if (r != QDialog::Accepted)
    {
      return;
    }

    QStringList filenames = dialog.selectedFiles();
    if (filenames.empty())
    {
      return;
    }

    QString filename = filenames.back();
#endif

    if (filename.isEmpty())
    {
      return;
    }

    put(startHere_, "export",
        QFileInfo(filename).absoluteDir().canonicalPath());

    TAsyncTaskPtr t(new ExportTask(this, model_, filename));
    tasks_.push_back(t);
    async_.push_back(t);
  }

  //----------------------------------------------------------------
  // MainWindow::fileExit
  //
  void
  MainWindow::fileExit()
  {
    MainWindow::close();
    qApp->quit();
  }

  //----------------------------------------------------------------
  // MainWindow::helpAbout
  //
  void
  MainWindow::helpAbout()
  {
    static AboutDialog * about = NULL;
    if (!about)
    {
      about = new AboutDialog(this);
      about->setWindowTitle(tr("Aeyae Remux (%1)").
                            arg(QString::fromUtf8(YAE_REVISION)));
    }

    about->show();
  }

  //----------------------------------------------------------------
  // MainWindow::requestToggleFullScreen
  //
  void
  MainWindow::requestToggleFullScreen()
  {
    // all this to work-around apparent QML bug where
    // toggling full-screen on double-click leaves Flickable in
    // a state where it never receives the button-up event
    // and ends up interpreting all mouse movement as dragging,
    // very annoying...
    //
    // The workaround is to delay fullscreen toggle to allow
    // Flickable time to receive the button-up event

    QTimer::singleShot(178, this, SLOT(toggleFullScreen()));
  }

  //----------------------------------------------------------------
  // MainWindow::toggleFullScreen
  //
  void
  MainWindow::toggleFullScreen()
  {
    if (isFullScreen())
    {
      exitFullScreen();
    }
    else
    {
      enterFullScreen();
    }
  }

  //----------------------------------------------------------------
  // MainWindow::enterFullScreen
  //
  void
  MainWindow::enterFullScreen()
  {
    if (isFullScreen())
    {
      exitFullScreen();
      return;
    }

    SignalBlocker blockSignals;
    blockSignals << actionFullScreen;

    actionFullScreen->setChecked(true);
    canvas_->setRenderMode(Canvas::kScaleToFit);

    if (isFullScreen())
    {
      return;
    }

    // enter full screen rendering:
    menuBar()->hide();
    showFullScreen();

    this->swapShortcuts();
  }

  //----------------------------------------------------------------
  // MainWindow::exitFullScreen
  //
  void
  MainWindow::exitFullScreen()
  {
    if (!isFullScreen())
    {
      return;
    }

    // exit full screen rendering:
    SignalBlocker blockSignals;
    blockSignals << actionFullScreen;

    actionFullScreen->setChecked(false);
    menuBar()->show();
    showNormal();
    canvas_->setRenderMode(Canvas::kScaleToFit);
    this->swapShortcuts();
  }

  //----------------------------------------------------------------
  // MainWindow::processDropEventUrls
  //
  void
  MainWindow::processDropEventUrls(const QList<QUrl> & urls)
  {
    std::set<std::string> sources;
    for (QList<QUrl>::const_iterator i = urls.begin(); i != urls.end(); ++i)
    {
      QUrl url = *i;

#ifdef __APPLE__
      if (url.toString().startsWith("file:///.file/id="))
      {
        std::string strUrl = url.toString().toUtf8().constData();
        strUrl = yae::absoluteUrlFrom(strUrl.c_str());
        url = QUrl::fromEncoded(QByteArray(strUrl.c_str()));
      }
#endif

      QString fullpath = QFileInfo(url.toLocalFile()).canonicalFilePath();
      if (fullpath.endsWith(tr(".yaerx"), Qt::CaseInsensitive))
      {
        fileOpen(fullpath);
      }
      else
      {
        sources.insert(std::string(fullpath.toUtf8().constData()));
      }
    }

    add(sources);
  }

  //----------------------------------------------------------------
  // MainWindow::swapShortcuts
  //
  void
  MainWindow::swapShortcuts()
  {
    yae::swapShortcuts(shortcutSave_, actionSave);
    yae::swapShortcuts(shortcutExit_, actionExit);
    yae::swapShortcuts(shortcutFullScreen_, actionFullScreen);
  }

  //----------------------------------------------------------------
  // MainWindow::event
  //
  bool
  MainWindow::event(QEvent * e)
  {
    if (e->type() == QEvent::User)
    {
      LoaderProgressObserver::Began * load_began =
        dynamic_cast<LoaderProgressObserver::Began *>(e);
      if (load_began)
      {
        std::string dirname;
        std::string basename;
        parse_file_path(load_began->source_, dirname, basename);

        spinner_.setEnabled(true);
        spinner_.setText(tr("loading: %1").
                         arg(QString::fromUtf8(basename.c_str())));

        load_began->accept();
        return true;
      }

      LoaderProgressObserver::Loaded * loaded =
        dynamic_cast<LoaderProgressObserver::Loaded *>(e);
      if (loaded)
      {
        // update the model and the view:
        view_.append_source(loaded->source_, loaded->demuxer_);
        view_.append_clip(loaded->clip_);
        view_.selected_ = model_.clips_.empty() ? 0 : model_.clips_.size() - 1;
        loaded->accept();
        return true;
      }

      LoaderProgressObserver::Done * load_done =
        dynamic_cast<LoaderProgressObserver::Done *>(e);
      if (load_done)
      {
        tasks_.pop_front();
        spinner_.setEnabled(!tasks_.empty());

        load_done->accept();
        return true;
      }

      ExportTask::Began * export_began = dynamic_cast<ExportTask::Began *>(e);
      if (export_began)
      {
        spinner_.setEnabled(true);
        spinner_.setText(tr("exporting: %1").arg(export_began->filename_));
        export_began->accept();
        return true;
      }

      ExportTask::Done * export_done = dynamic_cast<ExportTask::Done *>(e);
      if (export_done)
      {
        tasks_.pop_front();
        spinner_.setEnabled(!tasks_.empty());

        export_done->accept();
        return true;
      }
    }

    return QMainWindow::event(e);
  }

  //----------------------------------------------------------------
  // MainWindow::closeEvent
  //
  void
  MainWindow::closeEvent(QCloseEvent * e)
  {
    e->accept();
    fileExit();
  }

  //----------------------------------------------------------------
  // MainWindow::dragEnterEvent
  //
  void
  MainWindow::dragEnterEvent(QDragEnterEvent * e)
  {
    if (!e->mimeData()->hasUrls())
    {
      e->ignore();
      return;
    }

    e->acceptProposedAction();
  }

  //----------------------------------------------------------------
  // MainWindow::dropEvent
  //
  void
  MainWindow::dropEvent(QDropEvent * e)
  {
    if (!e->mimeData()->hasUrls())
    {
      e->ignore();
      return;
    }

    e->acceptProposedAction();
    processDropEventUrls(e->mimeData()->urls());
  }

  //----------------------------------------------------------------
  // MainWindow::keyPressEvent
  //
  void
  MainWindow::keyPressEvent(QKeyEvent * event)
  {
    int key = event->key();
    if (key == Qt::Key_Escape)
    {
      if (isFullScreen())
      {
        exitFullScreen();
      }
    }
#if 0
    else if (key == Qt::Key_I)
    {
      emit setInPoint();
    }
    else if (key == Qt::Key_O)
    {
      emit setOutPoint();
    }
#endif
    else
    {
      QMainWindow::keyPressEvent(event);
    }
  }

  //----------------------------------------------------------------
  // MainWindow::mousePressEvent
  //
  void
  MainWindow::mousePressEvent(QMouseEvent * e)
  {
    if (e->button() == Qt::RightButton)
    {
      QPoint localPt = e->pos();
      QPoint globalPt = QWidget::mapToGlobal(localPt);

      // populate the context menu:
      contextMenu_->clear();

      std::size_t items = 0;
      if (view_.actionSetInPoint_.isEnabled())
      {
        contextMenu_->addAction(&view_.actionSetInPoint_);
        items++;
      }

      if (view_.actionSetOutPoint_.isEnabled())
      {
        contextMenu_->addAction(&view_.actionSetOutPoint_);
        items++;
      }

      if (items)
      {
        contextMenu_->addSeparator();
      }

      contextMenu_->addAction(actionFullScreen);
      contextMenu_->popup(globalPt);
    }
  }

}
