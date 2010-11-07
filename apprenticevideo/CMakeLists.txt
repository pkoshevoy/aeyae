project(yae)
set(PROGNAME "yae")

cmake_minimum_required(VERSION 2.6.0)
include_directories(AFTER .)

find_package(OpenGL REQUIRED)

set(Boost_ADDITIONAL_VERSIONS "1.41" "1.41.0")
set(Boost_USE_MULTITHREAD ON)
find_package(Boost 1.35 COMPONENTS thread REQUIRED)
find_package(Boost 1.35 COMPONENTS date_time REQUIRED)
include_directories(AFTER ${Boost_INCLUDE_DIR})

include_directories(AFTER
  ${OPENGL_INCLUDE_DIR}
  )

find_package(Qt4)
if (QT4_FOUND)
else (QT4_FOUND)
  message(FATAL_ERROR "Cannot build without Qt4.")
endif (QT4_FOUND)

set(QT_USE_OPENGL TRUE)
set(QT_USE_QTMAIN TRUE)
include(${QT_USE_FILE})

include_directories(AFTER ${QT_INCLUDE_DIR}
			  ${QT_QTOPENGL_INCLUDE_DIR}
			  )

find_path(FFMPEG_INCLUDE_DIR libavutil/avutil.h /usr/include)
if (FFMPEG_INCLUDE_DIR)
  include_directories(AFTER ${FFMPEG_INCLUDE_DIR})
endif (FFMPEG_INCLUDE_DIR)

find_library(JPEG_LIBRARY jpeg DOC "jpeg library")
find_library(TIFF_LIBRARY tiff DOC "tiff library")
find_library(PNG_LIBRARY png DOC "png library")
find_library(MNG_LIBRARY mng DOC "mng library")
find_library(AVFORMAT_LIBRARY avformat DOC "ffmpeg avformat library")
find_library(AVCODEC_LIBRARY avcodec DOC "ffmpeg avutil library")
find_library(AVUTIL_LIBRARY avutil DOC "ffmpeg avutil library")
find_library(SWSCALE_LIBRARY swscale DOC "ffmpeg swscale library")
find_library(FAAD_LIBRARY faad DOC "faad library")
find_library(GSM_LIBRARY gsm DOC "gsm library")
find_library(MP3LAME_LIBRARY mp3lame DOC "mp3lame library")
find_library(OPENCORE_AMRNB_LIBRARY opencore-amrnb DOC "opencore-amrnb library")
find_library(OPENCORE_AMRWB_LIBRARY opencore-amrwb DOC "opencore-amrwb library")
find_library(OPENJPEG_LIBRARY openjpeg DOC "openjpeg library")
find_library(SCHROEDINGER_LIBRARY schroedinger-1.0 DOC "shroedinger library")
find_library(ORC_LIBRARY orc-0.4 DOC "orc library")
find_library(SPEEX_LIBRARY speex DOC "Xiph.org speex library")
find_library(THEORAENC_LIBRARY theoraenc DOC "Xiph.org theoraenc library")
find_library(THEORADEC_LIBRARY theoradec DOC "Xiph.org theoradec library")
find_library(OGG_LIBRARY ogg DOC "Xiph.org ogg library")
find_library(VORBISENC_LIBRARY vorbisenc DOC "Xiph.org vorbisenc library")
find_library(VORBIS_LIBRARY vorbis DOC "Xiph.org vorbis library")
find_library(VPX_LIBRARY vpx DOC "Google vpx library (On2 VP8)")
find_library(X264_LIBRARY x264 DOC "x264 library")
find_library(XVIDCODE_LIBRARY xvidcore DOC "xvidcore library")
find_library(NUT_LIBRARY nut DOC "nut library")
find_library(ZLIB_LIBRARY z DOC "zlib library")
find_library(BZIP2_LIBRARY bz2 DOC "bzip2 library")

#set(project_moc_hxx .....)
#qt4_wrap_cpp(project_moc_cxx ${iv_moc_hxx})

#set(project_uis ......)
#qt4_wrap_ui(project_uis_hxx ${project_uis})

include_directories(AFTER ${CMAKE_CURRENT_BINARY_DIR})

#qt4_add_resources(project_qrc_cxx ${PROGNAME}.qrc)

set (project_sources
  yaeMain.cpp
  yaeAPI.cpp
  yaeReaderFFMPEG.cpp
  yaeViewer.cpp
#  ${project_moc_cxx}
#  ${project_uis_hxx}
#  ${project_qrc_cxx}
  )

# Set some Win32 Specific Settings
if(WIN32)
  add_definitions(-D_USE_MATH_DEFINES -DNOMINMAX)
  set(GUI_TYPE WIN32)
#  set(project_sources ${project_sources} ${PROGNAME}.rc)
endif(WIN32)

# Set some Apple MacOS Specific settings
if (APPLE)
  set(GUI_TYPE MACOSX_BUNDLE)
#  set(project_sources ${project_sources} Info.plist ${PROGNAME}.icns)
endif (APPLE)

add_executable(${PROGNAME}
  ${GUI_TYPE}
  ${project_sources}
  )

set(TARGET_LIBS
  ${QT_QTOPENGL_LIBRARY}
  ${QT_LIBRARIES}
  ${OPENGL_LIBRARIES}
  ${Boost_LIBRARIES}
)

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
  set(TARGET_LIBS
    ${TARGET_LIBS}
    ws2_32
    avicap32
    avifil32
    psapi
    )
endif (WIN32)


if (QT_CONFIG MATCHES "static" OR QT4_PLUGINS_QTIFF MATCHES ".a$")
  add_definitions(-DQT_LIBS_ARE_STATIC)
  set(TARGET_LIBS
    ${TARGET_LIBS}
    ${QT4_PLUGINS_QGIF}
    ${QT4_PLUGINS_QTIFF}
    ${QT4_PLUGINS_QICO}
    ${QT4_PLUGINS_QJPEG}
    ${QT4_PLUGINS_QMNG}
  )
endif (QT_CONFIG MATCHES "static" OR QT4_PLUGINS_QTIFF MATCHES ".a$")

target_link_libraries(${PROGNAME}
  ${TARGET_LIBS}
  )

#if(APPLE)
#  set_target_properties(${PROGNAME}
#	PROPERTIES
#	MACOSX_BUNDLE_INFO_PLIST ${CMAKE_CURRENT_SOURCE_DIR}/Info.plist)
#
#  set_source_files_properties(${CMAKE_CURRENT_SOURCE_DIR}/${PROGNAME}.icns
#	PROPERTIES
#	MACOSX_PACKAGE_LOCATION Resources)
#
#else(APPLE)
#endif(APPLE)

install(TARGETS ${PROGNAME} DESTINATION bin)
