cmake_minimum_required(VERSION 3.1.0)

project(yaetv)
set(PROGNAME "yaetv")

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
  ../yaeui
  ../jsoncpp/include
  ${PROJECT_BINARY_DIR}
  ${PROJECT_BINARY_DIR}/../apprenticevideo
  ${Boost_INCLUDE_DIR}
  ${OPENGL_INCLUDE_DIR}
  ${QT_INCLUDE_DIR}
  ${QT_QTOPENGL_INCLUDE_DIR}
  )

if (WIN32 AND NOT (MINGW OR MSYS))
  include_directories(AFTER ../apprenticevideo/msIntTypes)
endif ()

include("${aeyae_SOURCE_DIR}/add_hdhomerun.cmake")

set(project_moc_hxx

  yaeAppView.h
  yaeMainWindow.h
  yaePlayerWindow.h
  )

set(project_uis
  yaeAbout.ui
  yaeMainWindow.ui
  yaePlayerWindow.ui
  yaePreferencesDialog.ui
  )

set(project_sources
  main.cpp
  yae_dvr.cpp
  yae_dvr.h
  yae_hdhomerun.cpp
  yae_hdhomerun.h
  yae_signal_handler.cpp
  yae_signal_handler.h

  yaeAppStyle.cpp
  yaeAppStyle.h
  yaeAppView.cpp
  yaeAppView.h
  yaeMainWindow.cpp
  yaeMainWindow.h
  yaePlayerWindow.cpp
  yaePlayerWindow.h
  )

if (QT4_FOUND)
  add_definitions(-DYAE_USE_QT4)
  qt4_add_resources(project_qrc_cxx ${PROGNAME}.qrc)
endif ()

if (Qt5_FOUND)
  add_definitions(-DYAE_USE_QT5)
  qt5_add_resources(project_qrc_cxx ${PROGNAME}.qrc)
endif ()

if (Qt6_FOUND)
  add_definitions(-DYAE_USE_QT6)
  qt6_add_resources(project_qrc_cxx ${PROGNAME}.qrc)
endif ()

if (WIN32)
  set (YAE_VERSIONED_FILE "${PROJECT_BINARY_DIR}/${PROGNAME}.rc")
endif ()

add_custom_target("update_resource_files_yaetv" ALL
  COMMAND "${CMAKE_COMMAND}"
  -DAEYAE_SOURCE_DIR="${aeyae_SOURCE_DIR}"
  -DAEYAE_BINARY_DIR="${aeyae_BINARY_DIR}"
  -DPROJECT_SOURCE_DIR="${PROJECT_SOURCE_DIR}"
  -DPROJECT_BINARY_DIR="${PROJECT_BINARY_DIR}"
  -DPROGNAME="${PROGNAME}"
  -DGIT_EXECUTABLE="${GIT_EXECUTABLE}"
  -DCMAKE_INSTALL_PREFIX="${CMAKE_INSTALL_PREFIX}"
  -P "${aeyae_SOURCE_DIR}/aeyae_update_resource_files_to_revision.cmake")

add_custom_command(OUTPUT "${YAE_VERSIONED_FILE}"
  DEPENDS "update_resource_files_yaetv"
  COMMAND "${CMAKE_COMMAND}" -E echo "updating ${YAE_VERSIONED_FILE}")

set(project_sources ${project_sources} ${YAE_VERSIONED_FILE})

# Set some Win32 Specific Settings
if (WIN32)
  set(GUI_TYPE WIN32)

  if (MSYS OR MINGW)
    set(CMAKE_RC_COMPILER_INIT windres)
    enable_language(RC)
    set(CMAKE_RC_COMPILE_OBJECT
      "<CMAKE_RC_COMPILER> <FLAGS> -O coff <DEFINES> -i <SOURCE> -o <OBJECT>")
  endif ()
endif ()

# Set some Apple MacOS Specific settings
if (APPLE)
  set(GUI_TYPE MACOSX_BUNDLE)

  set(project_sources
    ${project_sources}
    ${PROGNAME}.icns
    InfoPlist.strings
    )
endif ()

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

if (NOT (WIN32 OR APPLE))
  # qmake -query QT_INSTALL_LIBS
  execute_process(COMMAND ${QT_QMAKE_EXECUTABLE} -query QT_INSTALL_LIBS
    OUTPUT_VARIABLE QT_LIBRARIES_DIR
    OUTPUT_STRIP_TRAILING_WHITESPACE)
  set(CMAKE_INSTALL_RPATH
    ${CMAKE_INSTALL_RPATH};${QT_LIBRARIES_DIR};
    )
endif ()

add_executable(${PROGNAME}
  ${GUI_TYPE}
  ${project_sources}
  )

add_dependencies(${PROGNAME} "update_resource_files_yaetv")

set(TARGET_LIBS
  yaeui
  aeyae
  ${TARGET_LIBS}
  ${Boost_LIBRARIES}
  ${LIBASS_LIBRARY}
  ${GLEW_LIBRARY}
  ${QT_QTOPENGL_LIBRARY}
  ${QT_LIBRARIES}
  ${OPENGL_LIBRARIES}
  ${CMAKE_THREAD_LIBS_INIT}
  )

if (PORTAUDIO_LIBRARY)
  set(TARGET_LIBS ${TARGET_LIBS} ${PORTAUDIO_LIBRARY})
endif ()

if (APPLE)
  find_library(CORE_AUDIO_FRAMEWORK CoreAudio)
  mark_as_advanced(CORE_AUDIO_FRAMEWORK)
  if (CORE_AUDIO_FRAMEWORK)
    set(TARGET_LIBS ${TARGET_LIBS} ${CORE_AUDIO_FRAMEWORK})
  endif ()

  find_library(AUDIO_TOOLBOX_FRAMEWORK AudioToolbox)
  mark_as_advanced(AUDIO_TOOLBOX_FRAMEWORK)
  if (AUDIO_TOOLBOX_FRAMEWORK)
    set(TARGET_LIBS ${TARGET_LIBS} ${AUDIO_TOOLBOX_FRAMEWORK})
  endif ()

  find_library(AUDIO_UNIT_FRAMEWORK AudioUnit)
  mark_as_advanced(AUDIO_UNIT_FRAMEWORK)
  if (AUDIO_UNIT_FRAMEWORK)
    set(TARGET_LIBS ${TARGET_LIBS} ${AUDIO_UNIT_FRAMEWORK})
  endif ()
endif ()

if (WIN32)

  if (LIBGCC_LIBRARY)
    set(TARGET_LIBS ${TARGET_LIBS} ${LIBGCC_LIBRARY})
  endif ()

  if (LIBMINGW32_LIBRARY)
    set(TARGET_LIBS ${TARGET_LIBS} ${LIBMINGW32_LIBRARY})
  endif ()

  if (LIBMINGWEX_LIBRARY)
    set(TARGET_LIBS ${TARGET_LIBS} ${LIBMINGWEX_LIBRARY})
  endif ()

  if (LIBCOLDNAME_LIBRARY)
    set(TARGET_LIBS ${TARGET_LIBS} ${LIBCOLDNAME_LIBRARY})
  endif ()

  if (LIBMOLDNAME_RELEASE_LIBRARY)
    set(TARGET_LIBS ${TARGET_LIBS} ${LIBMOLDNAME_RELEASE_LIBRARY})
  endif ()

  if (LIBMOLDNAME_DEBUG_LIBRARY)
    set(TARGET_LIBS ${TARGET_LIBS} ${LIBMOLDNAME_DEBUG_LIBRARY})
  endif ()

  if (AVICAP32_LIBRARY)
    set(TARGET_LIBS ${TARGET_LIBS} ${AVICAP32_LIBRARY})
  endif ()

  if (AVIFIL32_LIBRARY)
    set(TARGET_LIBS ${TARGET_LIBS} ${AVIFIL32_LIBRARY})
  endif ()

  if (ICUUC_LIBRARY)
    set(TARGET_LIBS ${TARGET_LIBS} ${ICUUC_LIBRARY})
  endif ()

  if (ICUDT_LIBRARY)
    set(TARGET_LIBS ${TARGET_LIBS} ${ICUDT_LIBRARY})
  endif ()

  if (MSYS OR MINGW)
    set(TARGET_LIBS ${TARGET_LIBS}
      -L/Developer/ffmpeg/lib
      vo-aacenc
      vo-amrwbenc
      orc-0.4
      xavs
      pthread
      wsock32
      winmm)
  endif ()
endif ()

if (QT4_FOUND)
  #message("QT_CONFG: "${QT_CONFIG})
  if (QT_CONFIG MATCHES "static" OR QT4_PLUGINS_QTIFF MATCHES ".a$")
    add_definitions(-DQT_LIBS_ARE_STATIC)

    if (QT4_PLUGINS_QGIF)
      set(TARGET_LIBS ${TARGET_LIBS} ${QT4_PLUGINS_QGIF})
    endif ()

    if (QT4_PLUGINS_QTIFF)
      set(TARGET_LIBS ${TARGET_LIBS} ${QT4_PLUGINS_QTIFF})
    endif ()

    if (QT4_PLUGINS_QICO)
      set(TARGET_LIBS ${TARGET_LIBS} ${QT4_PLUGINS_QICO})
    endif ()

    if (QT4_PLUGINS_QJPEG)
      set(TARGET_LIBS ${TARGET_LIBS} ${QT4_PLUGINS_QJPEG})
    endif ()

    if (QT4_PLUGINS_QMNG)
      set(TARGET_LIBS ${TARGET_LIBS} ${QT4_PLUGINS_QMNG})
    endif ()

  endif ()
endif ()

if (WIN32)
  set(TARGET_LIBS
    ${TARGET_LIBS}
    ws2_32
    psapi
    )
endif ()

if (APPLE)
  find_library(CORE_SERVICES_FRAMEWORK CoreServices)
  mark_as_advanced(CORE_SERVICES_FRAMEWORK)
  if (CORE_SERVICES_FRAMEWORK)
    set(TARGET_LIBS ${TARGET_LIBS} ${CORE_SERVICES_FRAMEWORK})
  endif ()

  find_library(CORE_VIDEO_FRAMEWORK CoreVideo)
  mark_as_advanced(CORE_VIDEO_FRAMEWORK)
  if (CORE_VIDEO_FRAMEWORK)
    set(TARGET_LIBS ${TARGET_LIBS} ${CORE_VIDEO_FRAMEWORK})
  endif ()

  find_library(CORE_VDA_FRAMEWORK VideoDecodeAcceleration)
  mark_as_advanced(CORE_VDA_FRAMEWORK)
  if (CORE_VDA_FRAMEWORK)
    set(TARGET_LIBS ${TARGET_LIBS} ${CORE_VDA_FRAMEWORK})
  endif ()

  find_library(COCOA_FRAMEWORK Cocoa)
  mark_as_advanced(COCOA_FRAMEWORK)
  if (COCOA_FRAMEWORK)
    set(TARGET_LIBS ${TARGET_LIBS} ${COCOA_FRAMEWORK})
  endif ()

  find_library(IOKIT_FRAMEWORK IOKit)
  mark_as_advanced(IOKIT_FRAMEWORK)
  if (IOKIT_FRAMEWORK)
    set(TARGET_LIBS ${TARGET_LIBS} ${IOKIT_FRAMEWORK})
  endif ()
endif ()

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

#set_property(TARGET ${PROGNAME} PROPERTY CXX_STANDARD 98)
#set_property(TARGET ${PROGNAME} PROPERTY CXX_STANDARD 11)

#target_compile_definitions(${PROGNAME} PRIVATE -DJSON_DLL -DJSON_NO_AUTOLINK)
target_compile_definitions(${PROGNAME} PRIVATE -DJSON_DLL)

target_link_libraries(${PROGNAME}
  ${TARGET_LIBS}
  )

if (Qt4_FOUND)
  get_filename_component(QT_QMAKE_EXECUTABLE_DIR
    ${QT_QMAKE_EXECUTABLE} DIRECTORY)
endif ()

if (Qt5_FOUND)
  get_filename_component(QT_QMAKE_EXECUTABLE_DIR
    ${Qt5_DIR}/../../../bin ABSOLUTE)
endif ()

if (Qt6_FOUND)
  get_filename_component(QT_QMAKE_EXECUTABLE_DIR
    ${Qt6_DIR}/../../../bin ABSOLUTE)
endif ()

if (QT_QMAKE_EXECUTABLE_DIR)
  # message("QT_QMAKE_EXECUTABLE_DIR: " ${QT_QMAKE_EXECUTABLE_DIR})
  find_program(QT_QMAKE_EXECUTABLE NAMES qmake
    HINTS ${QT_QMAKE_EXECUTABLE_DIR}
    DOC
    "qmake - Qt build configuration utility")
endif ()

if (APPLE)
  set_target_properties(${PROGNAME}
    PROPERTIES
    XCODE_ATTRIBUTE_CODE_SIGN_IDENTITY ""
    MACOSX_BUNDLE_SHORT_VERSION_STRING "${PROJ_YYMMDD_HHMMSS}"
    MACOSX_BUNDLE_LONG_VERSION_STRING "${PROJ_YYMMDD_HHMMSS}"
    MACOSX_BUNDLE_INFO_STRING "${PROGNAME} by Pavel Koshevoy, build ${PROJ_WC_REVISION}"
    MACOSX_BUNDLE_COPYRIGHT "${PROGNAME} by Pavel Koshevoy, build ${PROJ_WC_REVISION}"
    MACOSX_BUNDLE_ICON_FILE "${PROGNAME}.icns"
    MACOSX_BUNDLE_GUI_IDENTIFIER "com.pavelkoshevoy.${PROGNAME}"
    MACOSX_BUNDLE_BUNDLE_NAME "${PROGNAME}"
    MACOSX_BUNDLE_INFO_PLIST "${PROJECT_SOURCE_DIR}/${PROGNAME}.plist.in")

  set_source_files_properties("${PROJECT_SOURCE_DIR}/${PROGNAME}.icns"
    PROPERTIES
    MACOSX_PACKAGE_LOCATION Resources)

  set_source_files_properties("${PROJECT_SOURCE_DIR}/InfoPlist.strings"
    PROPERTIES
    MACOSX_PACKAGE_LOCATION Resources/en.lproj)

  find_program(MACDEPLOYQT NAMES macdeployqt
    HINTS ${QT_QMAKE_EXECUTABLE_DIR}
    DOC
    "macdeployqt - Qt utility used to include frameworks required by the application into its bundle")

  add_custom_command(TARGET ${PROGNAME} POST_BUILD
    COMMAND ${PROJECT_SOURCE_DIR}/../apprenticevideo/DeployOSX.sh
    ARGS
    ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/${CMAKE_CFG_INTDIR}/${PROGNAME}.app
    ${MACDEPLOYQT}
    ${CMAKE_INSTALL_PREFIX}
    ${CMAKE_CURRENT_BINARY_DIR}
    ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}
    ${CMAKE_CFG_INTDIR}
    COMMENT "pull in external dependencies"
    )
endif ()

if (WIN32)
  if (Qt5_FOUND OR Qt6_FOUND)
    execute_process(COMMAND ${QT_QMAKE_EXECUTABLE} -query QT_INSTALL_PLUGINS
      OUTPUT_VARIABLE QT_PLUGINS_DIR
      OUTPUT_STRIP_TRAILING_WHITESPACE)
    set(DEPLOY_QT_PLUGINS
      "-deploy-to" "platforms" "${QT_PLUGINS_DIR}/platforms" "qwindows\.dll"
      "-deploy-to" "imageformats" "${QT_PLUGINS_DIR}/imageformats" ".*\.dll"
      )
  endif ()

  if (MINGW OR MSYS)
    string(REPLACE "/" "\\\\" NATIVE_PROGRAM_ICON "${PROJECT_SOURCE_DIR}/${PROGNAME}.ico")
    string(REPLACE "/" "\\\\" NATIVE_DEPENDS_EXE "${DEPENDENCY_WALKER}")
    string(REPLACE "/" "\\\\" NATIVE_ALLOWED_PATHS "${ALLOWED_PATHS}")
    string(REPLACE "/" "\\\\" NATIVE_CANDLE_EXE "${WIX_CANDLE}")
    string(REPLACE "/" "\\\\" NATIVE_LIGHT_EXE "${WIX_LIGHT}")
    string(REPLACE "/" "\\\\" NATIVE_VC_CRT_MSM "${VC_CRT_MSM}")
  else (MINGW OR MSYS)
    file(TO_NATIVE_PATH "${PROJECT_SOURCE_DIR}/${PROGNAME}.ico" NATIVE_PROGRAM_ICON)
    file(TO_NATIVE_PATH "${DEPENDENCY_WALKER}" NATIVE_DEPENDS_EXE)
    file(TO_NATIVE_PATH "${ALLOWED_PATHS}" NATIVE_ALLOWED_PATHS)
    file(TO_NATIVE_PATH "${WIX_CANDLE}" NATIVE_CANDLE_EXE)
    file(TO_NATIVE_PATH "${WIX_LIGHT}" NATIVE_LIGHT_EXE)
    file(TO_NATIVE_PATH "${VC_CRT_MSM}" NATIVE_VC_CRT_MSM)
  endif ()

  add_custom_target("create_yaetv_installer"
    ALL DEPENDS
    wixWrapper
    "${PROJECT_BINARY_DIR}/depends-exe-log.txt"
    )

  add_custom_command(OUTPUT "${PROJECT_BINARY_DIR}/depends-exe-log.txt"
    COMMAND wixWrapper
    ARGS
    "-what" "yaetv"
    "-dep-walker" "${NATIVE_DEPENDS_EXE}"
    "-allow" "$<TARGET_FILE_DIR:${PROGNAME}>;${NATIVE_ALLOWED_PATHS}"
    "-wix-candle" "${NATIVE_CANDLE_EXE}"
    "-wix-light" "${NATIVE_LIGHT_EXE}"
    "-vc-redist" "${NATIVE_VC_CRT_MSM}"
    "-icon" "${NATIVE_PROGRAM_ICON}"
    "-url" "https://github.com/pkoshevoy/aeyae"
    "-deploy" $<TARGET_FILE:${PROGNAME}>
    "-deploy" $<TARGET_FILE:aeyae>
    ${DEPLOY_QT_PLUGINS}
    DEPENDS ${PROGNAME} wixWrapper
    COMMENT "create yaetv installer package"
    )
endif ()

if (NOT (WIN32 OR APPLE))
  install(TARGETS ${PROGNAME} DESTINATION bin)

  # install locally:
  set(DESKTOP_INSTALL_PREFIX "$ENV{HOME}/.local")
  configure_file(
    "${PROJECT_SOURCE_DIR}/${PROGNAME}.desktop.in"
    "${PROJECT_BINARY_DIR}/${PROGNAME}.desktop.local"
    @ONLY
    )

  install(FILES "${PROJECT_BINARY_DIR}/${PROGNAME}.desktop.local"
    DESTINATION "$ENV{HOME}/.local/share/applications"
    RENAME "${PROGNAME}.desktop"
    )

  install(FILES "${PROJECT_SOURCE_DIR}/images/yaetv-logo-512.png"
    DESTINATION "$ENV{HOME}/.local/share/icons"
    RENAME "${PROGNAME}.png"
    )

  # install globally:
  set(DESKTOP_INSTALL_PREFIX "${CMAKE_INSTALL_PREFIX}")
  configure_file(
    "${PROJECT_SOURCE_DIR}/${PROGNAME}.desktop.in"
    "${PROJECT_BINARY_DIR}/${PROGNAME}.desktop"
    @ONLY
    )

  install(FILES "${PROJECT_BINARY_DIR}/${PROGNAME}.desktop"
    DESTINATION share/applications
    )

  install(FILES "${PROJECT_SOURCE_DIR}/images/yaetv-logo-512.png"
    DESTINATION share/icons
    RENAME "${PROGNAME}.png"
    )
endif ()

add_dependencies(${PROGNAME} aeyae yaeui)
