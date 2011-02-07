cmake_minimum_required(VERSION 2.8.0)

project(yae)
set(PROGNAME "yap")

if (EXISTS ${THE_DIR})
  set(THE_SRC_DIR ${THE_DIR})
endif (EXISTS ${THE_DIR})

if (NOT THE_SRC_DIR)
  file(TO_CMAKE_PATH "$ENV{THE_SRC_DIR}" THE_SRC_DIR)
endif (NOT THE_SRC_DIR)

if (NOT THE_BIN_DIR)
  file(TO_CMAKE_PATH "$ENV{THE_BIN_DIR}" THE_BIN_DIR)
endif (NOT THE_BIN_DIR)

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${THE_SRC_DIR})

find_package(THE REQUIRED)
find_package(OpenGL REQUIRED)

set(Boost_ADDITIONAL_VERSIONS "1.41" "1.41.0")
set(Boost_USE_MULTITHREAD ON)
find_package(Boost 1.35 COMPONENTS thread REQUIRED)
find_package(Boost 1.35 COMPONENTS date_time REQUIRED)

find_package(Qt4)
if (QT4_FOUND)
else (QT4_FOUND)
  message(FATAL_ERROR "Cannot build without Qt4.  Please set QT4_DIR.")
endif (QT4_FOUND)

set(QT_USE_OPENGL TRUE)
set(QT_USE_QTMAIN TRUE)
set(QT_USE_QTMULTIMEDIA TRUE)
include(${QT_USE_FILE})

add_definitions(-DBOOST_ALL_NO_LIB)
if (WIN32)
  add_definitions(-D_USE_MATH_DEFINES -DNOMINMAX)
endif (WIN32)

add_definitions(-DYAE_STATIC)

include_directories(AFTER 
  .
  ${CMAKE_CURRENT_BINARY_DIR}
  ${THE_SRC_DIR}
  ${Boost_INCLUDE_DIR}
  ${OPENGL_INCLUDE_DIR}
  ${QT_INCLUDE_DIR}
  ${QT_QTOPENGL_INCLUDE_DIR}
  ${QT_QTMULTIMEDIA_INCLUDE_DIR}
  )

if (WIN32)
  include_directories(AFTER msIntTypes)
endif (WIN32)

set(FFMPEG_HEADERS_PATH "" CACHE PATH "search path for ffmpeg headers")

find_path(FFMPEG_INCLUDE_DIR 
  libavutil/avutil.h
  ${FFMPEG_HEADERS_PATH}
  /usr/include
  /usr/local/include
  /opt/local/include)

if (FFMPEG_INCLUDE_DIR)
  include_directories(AFTER ${FFMPEG_INCLUDE_DIR})
endif (FFMPEG_INCLUDE_DIR)

find_library(QT4_PLUGINS_QGIF 
  qgif ${QT_PLUGINS_DIR}/imageformats 
  DOC "Qt image IO plugin for GIF")

find_library(QT4_PLUGINS_QTIFF 
  qtiff ${QT_PLUGINS_DIR}/imageformats 
  DOC "Qt image IO plugin for TIFF")

find_library(QT4_PLUGINS_QICO
  qico ${QT_PLUGINS_DIR}/imageformats
  DOC "Qt image IO plugin for ICO")

find_library(QT4_PLUGINS_QJPEG
  qjpeg ${QT_PLUGINS_DIR}/imageformats
  DOC "Qt image IO plugin for JPEG")

find_library(QT4_PLUGINS_QMNG
  qmng ${QT_PLUGINS_DIR}/imageformats
  DOC "Qt image IO plugin for MNG")

set(FFMPEG_LIBS_PATH "" CACHE PATH "search path for ffmpeg libraries")

find_library(JPEG_LIBRARY jpeg PATHS ${FFMPEG_LIBS_PATH} DOC "jpeg library")
find_library(TIFF_LIBRARY tiff PATHS ${FFMPEG_LIBS_PATH} DOC "tiff library")
find_library(PNG_LIBRARY png PATHS ${FFMPEG_LIBS_PATH} DOC "png library")
find_library(MNG_LIBRARY mng PATHS ${FFMPEG_LIBS_PATH} DOC "mng library")
find_library(AVFORMAT_LIBRARY avformat PATHS ${FFMPEG_LIBS_PATH} DOC "ffmpeg avformat library")
find_library(AVCODEC_LIBRARY avcodec PATHS ${FFMPEG_LIBS_PATH} DOC "ffmpeg avutil library")
find_library(AVUTIL_LIBRARY avutil PATHS ${FFMPEG_LIBS_PATH} DOC "ffmpeg avutil library")
find_library(SWSCALE_LIBRARY swscale PATHS ${FFMPEG_LIBS_PATH} DOC "ffmpeg swscale library")
find_library(FAAD_LIBRARY faad PATHS ${FFMPEG_LIBS_PATH} DOC "faad library")
find_library(GSM_LIBRARY gsm PATHS ${FFMPEG_LIBS_PATH} DOC "gsm library")
find_library(MP3LAME_LIBRARY mp3lame PATHS ${FFMPEG_LIBS_PATH} DOC "mp3lame library")
find_library(OPENCORE_AMRNB_LIBRARY opencore-amrnb PATHS ${FFMPEG_LIBS_PATH} DOC "opencore-amrnb library")
find_library(OPENCORE_AMRWB_LIBRARY opencore-amrwb PATHS ${FFMPEG_LIBS_PATH} DOC "opencore-amrwb library")
find_library(OPENJPEG_LIBRARY openjpeg PATHS ${FFMPEG_LIBS_PATH} DOC "openjpeg library")
find_library(SCHROEDINGER_LIBRARY schroedinger-1.0 PATHS ${FFMPEG_LIBS_PATH} DOC "shroedinger library")
find_library(ORC_LIBRARY orc-0.4 PATHS ${FFMPEG_LIBS_PATH} DOC "orc library")
find_library(SPEEX_LIBRARY speex PATHS ${FFMPEG_LIBS_PATH} DOC "Xiph.org speex library")
find_library(THEORAENC_LIBRARY theoraenc PATHS ${FFMPEG_LIBS_PATH} DOC "Xiph.org theoraenc library")
find_library(THEORADEC_LIBRARY theoradec PATHS ${FFMPEG_LIBS_PATH} DOC "Xiph.org theoradec library")
find_library(OGG_LIBRARY ogg PATHS ${FFMPEG_LIBS_PATH} DOC "Xiph.org ogg library")
find_library(VORBISENC_LIBRARY vorbisenc PATHS ${FFMPEG_LIBS_PATH} DOC "Xiph.org vorbisenc library")
find_library(VORBIS_LIBRARY vorbis PATHS ${FFMPEG_LIBS_PATH} DOC "Xiph.org vorbis library")
find_library(VPX_LIBRARY vpx PATHS ${FFMPEG_LIBS_PATH} DOC "Google vpx library (On2 VP8)")
find_library(X264_LIBRARY x264 PATHS ${FFMPEG_LIBS_PATH} DOC "x264 library")
find_library(XVIDCODE_LIBRARY xvidcore PATHS ${FFMPEG_LIBS_PATH} DOC "xvidcore library")
find_library(NUT_LIBRARY nut PATHS ${FFMPEG_LIBS_PATH} DOC "nut library")
find_library(ZLIB_LIBRARY z PATHS ${FFMPEG_LIBS_PATH} DOC "zlib library")
find_library(BZIP2_LIBRARY bz2 PATHS ${FFMPEG_LIBS_PATH} DOC "bzip2 library")

if (WIN32)
  find_library(LIBGCC_LIBRARY libgcc PATHS ${FFMPEG_LIBS_PATH} DOC "mingw libgcc library")
  find_library(LIBMINGW32_LIBRARY libmingw32 PATHS ${FFMPEG_LIBS_PATH} DOC "mingw libmingw32 library")
  find_library(LIBMINGWEX_LIBRARY libmingwex PATHS ${FFMPEG_LIBS_PATH} DOC "mingw libmingwex library")
  find_library(LIBCOLDNAME_LIBRARY libcoldname PATHS ${FFMPEG_LIBS_PATH} DOC "mingw libcoldname library")
  find_library(LIBMOLDNAME_RELEASE_LIBRARY libmoldname PATHS ${FFMPEG_LIBS_PATH} DOC "mingw libmoldname library")
  find_library(LIBMOLDNAME_DEBUG_LIBRARY libmoldnamed PATHS ${FFMPEG_LIBS_PATH} DOC "mingw libmoldnamed library")
  find_library(AVICAP32_LIBRARY avicap32 PATHS ${FFMPEG_LIBS_PATH} DOC "avicap32 library")
  find_library(AVIFIL32_LIBRARY avifil32 PATHS ${FFMPEG_LIBS_PATH} DOC "avifil32 library")
else (WIN32)
  find_library(ASOUND_LIBRARY asound PATHS ${FFMPEG_LIBS_PATH} DOC "alsa sound library required by QtMultimedia")
endif (WIN32)

find_library(PORTAUDIO_LIBRARY portaudio PATHS ${FFMPEG_LIBS_PATH} DOC "portaudio library")

set(project_moc_hxx 
  yaeMainWindow.h
  yaeViewer.h
  yaeAudioRendererQt.h
  )

qt4_wrap_cpp(project_moc_cxx ${project_moc_hxx})

set(project_uis 
  yaeMainWindow.ui
  )

qt4_wrap_ui(project_uis_hxx ${project_uis})

#qt4_add_resources(project_qrc_cxx ${PROGNAME}.qrc)

set(project_sources 
  yaeMain.cpp
  yaeMainWindow.h
  yaeMainWindow.cpp
  yaeAPI.h
  yaeAPI.cpp
  yaeQueue.h
  yaeReaderFFMPEG.h
  yaeReaderFFMPEG.cpp
  yaeThreading.h
  yaeUtils.h
  yaeUtils.cpp
  yaeViewer.h
  yaeViewer.cpp
  yaeAudioRenderer.h
  yaeAudioRenderer.cpp
  yaeAudioRendererQt.h
  yaeAudioRendererQt.cpp
  ${project_moc_cxx}
  ${project_uis_hxx}
#  ${project_qrc_cxx}
  )

find_path(PORTAUDIO_INCLUDE_DIR 
  portaudio.h
  ${FFMPEG_HEADERS_PATH}
  /usr/include
  /usr/local/include
  /opt/local/include)

if (PORTAUDIO_INCLUDE_DIR)
  include_directories(AFTER ${PORTAUDIO_INCLUDE_DIR})
endif (PORTAUDIO_INCLUDE_DIR)

if (PORTAUDIO_LIBRARY)
  add_definitions(-DYAE_HAS_PORTAUDIO)
  set(project_sources 
    ${project_sources} 
    yaeAudioRendererPortaudio.h
    yaeAudioRendererPortaudio.cpp
    )
endif (PORTAUDIO_LIBRARY)

# Set some Win32 Specific Settings
if (WIN32)
  set(GUI_TYPE WIN32)
  set(project_sources ${project_sources} ${PROGNAME}.rc)

  if (MSYS OR MINGW)
    set(CMAKE_RC_COMPILER_INIT windres)
    enable_language(RC)
    set(CMAKE_RC_COMPILE_OBJECT "<CMAKE_RC_COMPILER> <FLAGS> -O coff <DEFINES> -i <SOURCE> -o <OBJECT>")
  endif (MSYS OR MINGW)

endif (WIN32)

# Set some Apple MacOS Specific settings
if (APPLE)
  set(GUI_TYPE MACOSX_BUNDLE)
  set(project_sources ${project_sources} ${PROGNAME}Info.plist ${PROGNAME}.icns)
endif (APPLE)

add_executable(${PROGNAME}
  ${GUI_TYPE}
  ${project_sources}
  )

set(TARGET_LIBS
  ${THE_CORE_LIBRARY}
  ${QT_QTOPENGL_LIBRARY}
  ${QT_QTMULTIMEDIA_LIBRARY}
  ${QT_LIBRARIES}
  ${OPENGL_LIBRARIES}
  ${Boost_LIBRARIES}
  )

if (PORTAUDIO_LIBRARY)
  set(TARGET_LIBS ${TARGET_LIBS} ${PORTAUDIO_LIBRARY})
endif (PORTAUDIO_LIBRARY)

if (AVFORMAT_LIBRARY)
  set(TARGET_LIBS ${TARGET_LIBS} ${AVFORMAT_LIBRARY})
endif (AVFORMAT_LIBRARY)

if (AVCODEC_LIBRARY)
  set(TARGET_LIBS ${TARGET_LIBS} ${AVCODEC_LIBRARY})
endif (AVCODEC_LIBRARY)

if (AVUTIL_LIBRARY)
  set(TARGET_LIBS ${TARGET_LIBS} ${AVUTIL_LIBRARY})
endif (AVUTIL_LIBRARY)

if (SWSCALE_LIBRARY)
  set(TARGET_LIBS ${TARGET_LIBS} ${SWSCALE_LIBRARY})
endif (SWSCALE_LIBRARY)

if (FAAD_LIBRARY)
  set(TARGET_LIBS ${TARGET_LIBS} ${FAAD_LIBRARY})
endif (FAAD_LIBRARY)

if (GSM_LIBRARY)
  set(TARGET_LIBS ${TARGET_LIBS} ${GSM_LIBRARY})
endif (GSM_LIBRARY)

if (MP3LAME_LIBRARY)
  set(TARGET_LIBS ${TARGET_LIBS} ${MP3LAME_LIBRARY})
endif (MP3LAME_LIBRARY)

if (OPENCORE_AMRNB_LIBRARY)
  set(TARGET_LIBS ${TARGET_LIBS} ${OPENCORE_AMRNB_LIBRARY})
endif (OPENCORE_AMRNB_LIBRARY)

if (OPENCORE_AMRWB_LIBRARY)
  set(TARGET_LIBS ${TARGET_LIBS} ${OPENCORE_AMRWB_LIBRARY})
endif (OPENCORE_AMRWB_LIBRARY)

if (OPENJPEG_LIBRARY)
  set(TARGET_LIBS ${TARGET_LIBS} ${OPENJPEG_LIBRARY})
endif (OPENJPEG_LIBRARY)

if (SCHROEDINGER_LIBRARY)
  set(TARGET_LIBS ${TARGET_LIBS} ${SCHROEDINGER_LIBRARY})
endif (SCHROEDINGER_LIBRARY)

if (ORC_LIBRARY)
  set(TARGET_LIBS ${TARGET_LIBS} ${ORC_LIBRARY})
endif (ORC_LIBRARY)

if (SPEEX_LIBRARY)
  set(TARGET_LIBS ${TARGET_LIBS} ${SPEEX_LIBRARY})
endif (SPEEX_LIBRARY)

if (THEORAENC_LIBRARY)
  set(TARGET_LIBS ${TARGET_LIBS} ${THEORAENC_LIBRARY})
endif (THEORAENC_LIBRARY)

if (THEORADEC_LIBRARY)
  set(TARGET_LIBS ${TARGET_LIBS} ${THEORADEC_LIBRARY})
endif (THEORADEC_LIBRARY)

if (OGG_LIBRARY)
  set(TARGET_LIBS ${TARGET_LIBS} ${OGG_LIBRARY})
endif (OGG_LIBRARY)

if (VORBISENC_LIBRARY)
  set(TARGET_LIBS ${TARGET_LIBS} ${VORBISENC_LIBRARY})
endif (VORBISENC_LIBRARY)

if (VORBIS_LIBRARY)
  set(TARGET_LIBS ${TARGET_LIBS} ${VORBIS_LIBRARY})
endif (VORBIS_LIBRARY)

if (VPX_LIBRARY)
  set(TARGET_LIBS ${TARGET_LIBS} ${VPX_LIBRARY})
endif (VPX_LIBRARY)

if (X264_LIBRARY)
  set(TARGET_LIBS ${TARGET_LIBS} ${X264_LIBRARY})
endif (X264_LIBRARY)

if (XVIDCODE_LIBRARY)
  set(TARGET_LIBS ${TARGET_LIBS} ${XVIDCODE_LIBRARY})
endif (XVIDCODE_LIBRARY)

if (NUT_LIBRARY)
  set(TARGET_LIBS ${TARGET_LIBS} ${NUT_LIBRARY})
endif (NUT_LIBRARY)

if (TIFF_LIBRARY)
  set(TARGET_LIBS ${TARGET_LIBS} ${TIFF_LIBRARY})
endif (TIFF_LIBRARY)

if (JPEG_LIBRARY)
  set(TARGET_LIBS ${TARGET_LIBS} ${JPEG_LIBRARY})
endif (JPEG_LIBRARY)

if (PNG_LIBRARY)
  set(TARGET_LIBS ${TARGET_LIBS} ${PNG_LIBRARY})
endif (PNG_LIBRARY)

if (MNG_LIBRARY)
  set(TARGET_LIBS ${TARGET_LIBS} ${MNG_LIBRARY})
endif (MNG_LIBRARY)

if (ZLIB_LIBRARY)
  set(TARGET_LIBS ${TARGET_LIBS} ${ZLIB_LIBRARY})
endif (ZLIB_LIBRARY)

if (BZIP2_LIBRARY)
  set(TARGET_LIBS ${TARGET_LIBS} ${BZIP2_LIBRARY})
endif (BZIP2_LIBRARY)

if (WIN32)

  if (LIBGCC_LIBRARY)
    set(TARGET_LIBS ${TARGET_LIBS} ${LIBGCC_LIBRARY})
  endif (LIBGCC_LIBRARY)
  
  if (LIBMINGW32_LIBRARY)
    set(TARGET_LIBS ${TARGET_LIBS} ${LIBMINGW32_LIBRARY})
  endif (LIBMINGW32_LIBRARY)
  
  if (LIBMINGWEX_LIBRARY)
    set(TARGET_LIBS ${TARGET_LIBS} ${LIBMINGWEX_LIBRARY})
  endif (LIBMINGWEX_LIBRARY)
  
  if (LIBCOLDNAME_LIBRARY)
    set(TARGET_LIBS ${TARGET_LIBS} ${LIBCOLDNAME_LIBRARY})
  endif (LIBCOLDNAME_LIBRARY)
  
  if (LIBMOLDNAME_RELEASE_LIBRARY)
    set(TARGET_LIBS ${TARGET_LIBS} ${LIBMOLDNAME_RELEASE_LIBRARY})
  endif (LIBMOLDNAME_RELEASE_LIBRARY)
  
  if (LIBMOLDNAME_DEBUG_LIBRARY)
    set(TARGET_LIBS ${TARGET_LIBS} ${LIBMOLDNAME_DEBUG_LIBRARY})
  endif (LIBMOLDNAME_DEBUG_LIBRARY)
  
  if (AVICAP32_LIBRARY)
    set(TARGET_LIBS ${TARGET_LIBS} ${AVICAP32_LIBRARY})
  endif (AVICAP32_LIBRARY)
  
  if (AVIFIL32_LIBRARY)
    set(TARGET_LIBS ${TARGET_LIBS} ${AVIFIL32_LIBRARY})
  endif (AVIFIL32_LIBRARY)
else (WIN32)
  if (ASOUND_LIBRARY)
    set(TARGET_LIBS ${TARGET_LIBS} ${ASOUND_LIBRARY})
  endif (ASOUND_LIBRARY)
endif (WIN32)

#message("QT_CONFG: "${QT_CONFIG})
if (QT_CONFIG MATCHES "static" OR QT4_PLUGINS_QTIFF MATCHES ".a$")
  add_definitions(-DQT_LIBS_ARE_STATIC)

  if (QT4_PLUGINS_QGIF)
    set(TARGET_LIBS ${TARGET_LIBS} ${QT4_PLUGINS_QGIF})
  endif (QT4_PLUGINS_QGIF)
  
  if (QT4_PLUGINS_QTIFF)
    set(TARGET_LIBS ${TARGET_LIBS} ${QT4_PLUGINS_QTIFF})
  endif (QT4_PLUGINS_QTIFF)

  if (QT4_PLUGINS_QICO)
    set(TARGET_LIBS ${TARGET_LIBS} ${QT4_PLUGINS_QICO})
  endif (QT4_PLUGINS_QICO)
  
  if (QT4_PLUGINS_QJPEG)
    set(TARGET_LIBS ${TARGET_LIBS} ${QT4_PLUGINS_QJPEG})
  endif (QT4_PLUGINS_QJPEG)
  
  if (QT4_PLUGINS_QMNG)
    set(TARGET_LIBS ${TARGET_LIBS} ${QT4_PLUGINS_QMNG})
  endif (QT4_PLUGINS_QMNG)

endif (QT_CONFIG MATCHES "static" OR QT4_PLUGINS_QTIFF MATCHES ".a$")

if (WIN32)
  set(TARGET_LIBS
    ${TARGET_LIBS}
    ws2_32
    psapi
    )
endif (WIN32)

if (APPLE)
  find_library(CORE_SERVICES_FRAMEWORK CoreServices)
  mark_as_advanced(CORE_SERVICES_FRAMEWORK)
  if (CORE_SERVICES_FRAMEWORK)
    set(TARGET_LIBS ${TARGET_LIBS} ${CORE_SERVICES_FRAMEWORK})
  endif (CORE_SERVICES_FRAMEWORK)
endif (APPLE)

target_link_libraries(${PROGNAME}
  ${TARGET_LIBS}
  )

if (APPLE)
  set_target_properties(${PROGNAME}
	PROPERTIES
	MACOSX_BUNDLE_INFO_PLIST ${CMAKE_CURRENT_SOURCE_DIR}/${PROGNAME}Info.plist)

  set_source_files_properties(${CMAKE_CURRENT_SOURCE_DIR}/${PROGNAME}.icns
	PROPERTIES
	MACOSX_PACKAGE_LOCATION Resources)

  add_custom_command(TARGET ${PROGNAME} POST_BUILD
	COMMAND ${THE_SRC_DIR}/DeployOSX.sh 
	ARGS ${CMAKE_CURRENT_BINARY_DIR}/${CMAKE_CFG_INTDIR}/${PROGNAME}.app
	COMMENT "pull in external dependencies"
	)
endif (APPLE)

install(TARGETS ${PROGNAME} DESTINATION bin)
