cmake_minimum_required(VERSION 3.1.0)

set(PROGNAME "yaeui")
project("yaeui")

find_package(OpenGL REQUIRED)

set(CMAKE_PREFIX_PATH "$ENV{CMAKE_PREFIX_PATH}" CACHE PATH
  "search path for .cmake files")

find_package(Qt6 COMPONENTS Core Gui Widgets OpenGL OpenGLWidgets)
if (Qt6_FOUND)
  add_definitions(-DYAE_USE_QT6)
  if (NOT (APPLE OR WIN32))
    find_package(Qt6DBus)
  endif ()

  option(YAE_USE_QOPENGL_WIDGET "Use QOpenGLWidget instead of QGLWidget" OFF)
  if (YAE_USE_QOPENGL_WIDGET)
    add_definitions(-DYAE_USE_QOPENGL_WIDGET)
  endif ()

else (Qt6_FOUND)
  find_package(Qt5 COMPONENTS Core Gui Widgets OpenGL)

  if (Qt5_FOUND)
    add_definitions(-DYAE_USE_QT5)
    if (NOT (APPLE OR WIN32))
      find_package(Qt5DBus)
    endif ()

    option(YAE_USE_QOPENGL_WIDGET "Use QOpenGLWidget instead of QGLWidget" OFF)
    if (YAE_USE_QOPENGL_WIDGET)
      add_definitions(-DYAE_USE_QOPENGL_WIDGET)
    endif ()

  else (Qt5_FOUND)
    find_package(Qt4 REQUIRED)
    if (QT4_FOUND)
      add_definitions(-DYAE_USE_QT4)
      if (NOT (APPLE OR WIN32))
        if (QT_QTDBUS_FOUND)
          set(QT_USE_QTDBUS TRUE)
        endif ()
      endif ()

      set(QT_USE_QTOPENGL TRUE)
      set(QT_USE_QTMAIN TRUE)
      include(${QT_USE_FILE})
    endif (QT4_FOUND)
  endif (Qt5_FOUND)
endif (Qt6_FOUND)

# add_definitions(-DYAE_STATIC)

include_directories(AFTER
  .
  ..
  ../jsoncpp/include
  ${PROJECT_BINARY_DIR}
  ${Boost_INCLUDE_DIR}
  ${OPENGL_INCLUDE_DIR}
  ${QT_INCLUDE_DIR}
  ${QT_QTOPENGL_INCLUDE_DIR}
  )

if (WIN32 AND NOT (MINGW OR MSYS))
  include_directories(AFTER msIntTypes)
endif ()

set(project_moc_hxx
  # apprenticevideo:
  yaeCanvasWidget.h
  yaeFrameCropItem.h
  yaeItemView.h
  yaeScreenSaverInhibitor.h
  yaeTextInput.h
  yaeTimelineItem.h
  yaeTimelineModel.h

  # remux:
  yaeSpinnerView.h
  yaePlayerItem.h
  yaePlayerUxItem.h

  # yaetv:
  yaeAspectRatioItem.h
  yaeOptionItem.h
  yaePlayerUxItem.h
  yaePlayerWidget.h

  # yaeui:
  yaeApplication.h
  yaeUtilsQt.h
  )

set(project_uis
  # ../apprenticevideo/yaeAspectRatioDialog.ui
  )

set(project_sources
  yaeApplication.cpp
  yaeApplication.h

  # apprenticevideo:
  yaeArrowItem.cpp
  yaeArrowItem.h
  yaeBBox.cpp
  yaeBBox.h
  yaeCanvas.h
  yaeCanvas.cpp
  yaeCanvasWidget.cpp
  yaeCanvasWidget.h
  yaeCanvasQPainterUtils.h
  yaeCanvasQPainterUtils.cpp
  yaeCanvasRenderer.h
  yaeCanvasRenderer.cpp
  yaeColor.h
  yaeDashedRect.cpp
  yaeDashedRect.h
  yaeDonutRect.cpp
  yaeDonutRect.h
  yaeExpression.h
  yaeFlickableArea.cpp
  yaeFlickableArea.h
  yaeFrameCropItem.cpp
  yaeFrameCropItem.h
  yaeFrameCropView.cpp
  yaeFrameCropView.h
  yaeGradient.cpp
  yaeGradient.h
  yaeImage.cpp
  yaeImage.h
  yaeImageLive.cpp
  yaeImageLive.h
  yaeImageProvider.cpp
  yaeImageProvider.h
  yaeInputArea.cpp
  yaeInputArea.h
  yaeItem.cpp
  yaeItem.h
  yaeItemFocus.cpp
  yaeItemFocus.h
  yaeItemRef.h
  yaeItemView.cpp
  yaeItemView.h
  yaeItemViewStyle.cpp
  yaeItemViewStyle.h
  yaeProperty.h
  yaeRectangle.cpp
  yaeRectangle.h
  yaeRoundRect.cpp
  yaeRoundRect.h
  yaeScreenSaverInhibitor.h
  yaeScreenSaverInhibitor.cpp
  yaeScrollview.cpp
  yaeScrollview.h
  yaeSegment.cpp
  yaeSegment.h
  yaeText.cpp
  yaeText.h
  yaeTextInput.cpp
  yaeTextInput.h
  yaeTexture.cpp
  yaeTexture.h
  yaeTexturedRect.cpp
  yaeTexturedRect.h
  yaeThumbnailProvider.h
  yaeThumbnailProvider.cpp
  yaeTimelineItem.h
  yaeTimelineItem.cpp
  yaeTimelineModel.h
  yaeTimelineModel.cpp
  yaeTransform.cpp
  yaeTransform.h
  yaeUtilsQt.h
  yaeUtilsQt.cpp
  yaeVec.h

  # remux:
  yaeAxisItem.cpp
  yaeAxisItem.h
  yaeCheckboxItem.cpp
  yaeCheckboxItem.h
  yaeInputProxyItem.cpp
  yaeInputProxyItem.h
  yaeLShape.cpp
  yaeLShape.h
  yaePlayerItem.cpp
  yaePlayerItem.h
  yaePlayerUxItem.cpp
  yaePlayerUxItem.h
  yaePlotItem.cpp
  yaePlotItem.h
  yaeSpinnerItem.cpp
  yaeSpinnerItem.h
  yaeSpinnerView.cpp
  yaeSpinnerView.h
  yaeTabRect.cpp
  yaeTabRect.h

  # yaetv:
  yaeAspectRatioItem.cpp
  yaeAspectRatioItem.h
  yaeAspectRatioView.cpp
  yaeAspectRatioView.h
  yaeConfirmItem.cpp
  yaeConfirmItem.h
  yaeConfirmView.cpp
  yaeConfirmView.h
  yaeOptionItem.cpp
  yaeOptionItem.h
  yaeOptionView.cpp
  yaeOptionView.h
  yaePlayerShortcuts.cpp
  yaePlayerShortcuts.h
  yaePlayerStyle.cpp
  yaePlayerStyle.h
  yaePlayerView.cpp
  yaePlayerView.h
  yaePlayerWidget.cpp
  yaePlayerWidget.h
  )

if (LIBASS_FOUND OR LIBASS_LIBRARY)
  add_definitions(-DYAE_USE_LIBASS)
  set(project_sources
    ${project_sources}
    yaeLibass.h
    yaeLibass.cpp
    )
endif ()

if (APPLE)
  add_definitions(-D__ASSERT_MACROS_DEFINE_VERSIONS_WITHOUT_UNDERSCORES=0)
  add_definitions(-D__ASSERTMACROS__)
  include_directories(AFTER martinkahr-apple_remote_control)

  set(project_sources
    ${project_sources}
    yaeAppleRemoteControl.h
    yaeAppleRemoteControl.mm
    yaeAppleUtils.h
    yaeAppleUtils.mm
    yaeAudioUnitRenderer.cpp
    yaeAudioUnitRenderer.h
    yaeAudioUnitRenderer.m
    martinkahr-apple_remote_control/AppleRemote.h
    martinkahr-apple_remote_control/AppleRemote.m
    martinkahr-apple_remote_control/HIDRemoteControlDevice.h
    martinkahr-apple_remote_control/HIDRemoteControlDevice.m
    martinkahr-apple_remote_control/MultiClickRemoteBehavior.h
    martinkahr-apple_remote_control/MultiClickRemoteBehavior.m
    martinkahr-apple_remote_control/RemoteControl.h
    martinkahr-apple_remote_control/RemoteControl.m)
else ()
  include("${aeyae_SOURCE_DIR}/add_portaudio.cmake")
  set(project_sources
    ${project_sources}
    yaePortaudioRenderer.h
    yaePortaudioRenderer.cpp)
endif ()

include("${aeyae_SOURCE_DIR}/add_ffmpeg.cmake")
include("${aeyae_SOURCE_DIR}/add_glew.cmake")
include("${aeyae_SOURCE_DIR}/add_libass.cmake")

# Set some Apple MacOS Specific settings
if (QT4_FOUND)
  qt4_wrap_cpp(project_moc_cxx ${project_moc_hxx})
  qt4_wrap_ui(project_uis_hxx ${project_uis})
endif ()

if (Qt5_FOUND)
  qt5_wrap_cpp(project_moc_cxx ${project_moc_hxx})
  qt5_wrap_ui(project_uis_hxx ${project_uis})
endif ()

if (Qt6_FOUND)
  qt6_wrap_cpp(project_moc_cxx ${project_moc_hxx})
  qt6_wrap_ui(project_uis_hxx ${project_uis})
endif ()

set(project_sources
  ${project_sources}
  ${project_moc_cxx}
  ${project_uis_hxx}
  ${project_qrc_cxx}
  )

add_library(${PROGNAME} STATIC
  ${project_sources}
  )

if (QT4_FOUND)
  #message("QT_CONFG: "${QT_CONFIG})
  if (QT_CONFIG MATCHES "static" OR QT4_PLUGINS_QTIFF MATCHES ".a$")
    add_definitions(-DQT_LIBS_ARE_STATIC)
  endif ()
endif ()

#set_property(TARGET ${PROGNAME} PROPERTY CXX_STANDARD 98)
#set_property(TARGET ${PROGNAME} PROPERTY CXX_STANDARD 17)

target_compile_definitions(${PROGNAME} PRIVATE -DJSON_DLL)

if (Qt5_FOUND)
  set(TARGET_LIBS ${TARGET_LIBS}
    Qt5::Core
    Qt5::Gui
    Qt5::OpenGL
    Qt5::Widgets
    )

  if (Qt5DBus_FOUND)
    set(TARGET_LIBS ${TARGET_LIBS}
      Qt5::DBus
      )
  endif ()
endif ()

if (Qt6_FOUND)
  set(TARGET_LIBS ${TARGET_LIBS}
    Qt6::Core
    Qt6::Gui
    Qt6::OpenGL
    Qt6::OpenGLWidgets
    Qt6::Widgets
    )

  if (Qt6DBus_FOUND)
    set(TARGET_LIBS ${TARGET_LIBS}
      Qt6::DBus
      )
  endif ()
endif ()

target_link_libraries(${PROGNAME} PRIVATE ${TARGET_LIBS})
