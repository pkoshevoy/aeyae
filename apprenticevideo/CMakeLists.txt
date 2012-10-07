cmake_minimum_required(VERSION 2.8.0)

project("apprenticevideo")
set(PROGNAME "apprenticevideo")

include("${PROJECT_SOURCE_DIR}/updateRevision.cmake")

find_package(OpenGL REQUIRED)

set(Boost_USE_MULTITHREAD ON)
find_package(Boost 1.35 COMPONENTS thread REQUIRED)
find_package(Boost 1.35 COMPONENTS date_time REQUIRED)
find_package(Boost 1.35 COMPONENTS system REQUIRED)

find_package(Qt4)
if (QT4_FOUND)
else (QT4_FOUND)
  message(FATAL_ERROR "Cannot build without Qt4.  Please set QT4_DIR.")
endif (QT4_FOUND)

if (QT_QTDBUS_FOUND)
  set(QT_USE_QTDBUS TRUE)
endif (QT_QTDBUS_FOUND)

set(QT_USE_QTOPENGL TRUE)
set(QT_USE_QTMAIN TRUE)
include(${QT_USE_FILE})

add_definitions(-DBOOST_ALL_NO_LIB)

if (MSYS OR MINGW)
  add_definitions(-DBOOST_THREAD_USE_LIB)
endif (MSYS OR MINGW)

if (WIN32)
  add_definitions(-D_USE_MATH_DEFINES -DNOMINMAX)
endif (WIN32)

add_definitions(-DYAE_STATIC)

# NOTE: this is necessary when linking to static GLEW libs:
add_definitions(-DGLEW_STATIC=1)

include_directories(AFTER
  .
  ${PROJECT_BINARY_DIR}
  ${Boost_INCLUDE_DIR}
  ${OPENGL_INCLUDE_DIR}
  ${QT_INCLUDE_DIR}
  ${QT_QTOPENGL_INCLUDE_DIR}
  )

if (WIN32 AND NOT (MINGW OR MSYS))
  include_directories(AFTER msIntTypes)
endif (WIN32 AND NOT (MINGW OR MSYS))

set(FFMPEG_HEADERS_PATH "$ENV{FFMPEG_HEADERS_PATH}" CACHE PATH "search path for ffmpeg headers")

find_path(FFMPEG_INCLUDE_DIR
  libavutil/avutil.h
  PATHS
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

set(FFMPEG_LIBS_PATH "$ENV{FFMPEG_LIBS_PATH}" CACHE PATH "search path for ffmpeg libraries")

find_library(JPEG_LIBRARY jpeg PATHS ${FFMPEG_LIBS_PATH} DOC "jpeg library")
find_library(TIFF_LIBRARY tiff PATHS ${FFMPEG_LIBS_PATH} DOC "tiff library")
find_library(PNG_LIBRARY png PATHS ${FFMPEG_LIBS_PATH} DOC "png library")
find_library(MNG_LIBRARY mng PATHS ${FFMPEG_LIBS_PATH} DOC "mng library")
find_library(AVFORMAT_LIBRARY avformat PATHS ${FFMPEG_LIBS_PATH} DOC "ffmpeg avformat library")
find_library(AVCODEC_LIBRARY avcodec PATHS ${FFMPEG_LIBS_PATH} DOC "ffmpeg avutil library")
find_library(AVUTIL_LIBRARY avutil PATHS ${FFMPEG_LIBS_PATH} DOC "ffmpeg avutil library")
find_library(AVFILTER_LIBRARY avfilter PATHS ${FFMPEG_LIBS_PATH} DOC "ffmpeg avfilter library")
find_library(SWSCALE_LIBRARY swscale PATHS ${FFMPEG_LIBS_PATH} DOC "ffmpeg swscale library")
find_library(SWRESAMPLE_LIBRARY swresample PATHS ${FFMPEG_LIBS_PATH} DOC "ffmpeg swresample library")
find_library(POSTPROC_LIBRARY postproc PATHS ${FFMPEG_LIBS_PATH} DOC "ffmpeg postproc library")
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
find_library(CELT_LIBRARY celt0 PATHS ${FFMPEG_LIBS_PATH} DOC "celt library")
find_library(MODPLUG_LIBRARY modplug PATHS ${FFMPEG_LIBS_PATH} DOC "modplug library")
#find_library(FREETYPE_LIBRARY freetype PATHS ${FFMPEG_LIBS_PATH} DOC "freetype library")
find_library(GNUTLS_LIBRARY gnutls PATHS ${FFMPEG_LIBS_PATH} DOC "gnutls library")
find_library(DIRACENC_LIBRARY dirac_encoder PATHS ${FFMPEG_LIBS_PATH} DOC "dirac_encoder library")
find_library(RTMP_LIBRARY rtmp PATHS ${FFMPEG_LIBS_PATH} DOC "rtmp library")
find_library(ZLIB_LIBRARY z PATHS ${FFMPEG_LIBS_PATH} DOC "zlib library")
find_library(BZIP2_LIBRARY bz2 PATHS ${FFMPEG_LIBS_PATH} DOC "bzip2 library")
find_library(VA_LIBRARY va PATHS ${FFMPEG_LIBS_PATH} DOC "va library")
find_library(ALSA_LIBRARY asound PATHS ${FFMPEG_LIBS_PATH} DOC "alsa library")
find_library(JACK_LIBRARY jack PATHS ${FFMPEG_LIBS_PATH} DOC "jack library")

if (WIN32)
  find_library(LIBGCC_LIBRARY libgcc PATHS ${FFMPEG_LIBS_PATH} DOC "mingw libgcc library")
  find_library(LIBMINGW32_LIBRARY libmingw32 PATHS ${FFMPEG_LIBS_PATH} DOC "mingw libmingw32 library")
  find_library(LIBMINGWEX_LIBRARY libmingwex PATHS ${FFMPEG_LIBS_PATH} DOC "mingw libmingwex library")
  find_library(LIBCOLDNAME_LIBRARY libcoldname PATHS ${FFMPEG_LIBS_PATH} DOC "mingw libcoldname library")
  find_library(LIBMOLDNAME_RELEASE_LIBRARY libmoldname PATHS ${FFMPEG_LIBS_PATH} DOC "mingw libmoldname library")
  find_library(LIBMOLDNAME_DEBUG_LIBRARY libmoldnamed PATHS ${FFMPEG_LIBS_PATH} DOC "mingw libmoldnamed library")
  find_library(AVICAP32_LIBRARY avicap32 PATHS ${FFMPEG_LIBS_PATH} DOC "avicap32 library")
  find_library(AVIFIL32_LIBRARY avifil32 PATHS ${FFMPEG_LIBS_PATH} DOC "avifil32 library")
  find_library(ICUUC_LIBRARY icuuc PATHS ${FFMPEG_LIBS_PATH} DOC "icuuc library")
  find_library(ICUDT_LIBRARY icudt PATHS ${FFMPEG_LIBS_PATH} DOC "icudt library")
endif (WIN32)

set(LIBASS_DIR "$ENV{LIBASS_DIR}" CACHE PATH "root path for libass lib/ and include/ folders")
find_library(LIBASS_LIBRARY ass libass
  PATHS ${LIBASS_DIR}/lib ${FFMPEG_LIBS_PATH}
  DOC "libass (for ssa/ass subtitle renderering)")
find_library(LIBHARFBUZZ_LIBRARY harfbuzz libharfbuzz
  PATHS ${LIBASS_DIR}/lib ${FFMPEG_LIBS_PATH}
  DOC "libharfbuzz (for ssa/ass subtitle renderering)")
find_library(LIBFRIBIDI_LIBRARY fribidi libfribidi
  PATHS ${LIBASS_DIR}/lib ${FFMPEG_LIBS_PATH}
  DOC "libfribidi (for ssa/ass subtitle renderering)")
find_library(LIBFONTCONFIG_LIBRARY fontconfig libfontconfig
  PATHS ${LIBASS_DIR}/lib ${FFMPEG_LIBS_PATH}
  DOC "libfontconfig (for ssa/ass subtitle renderering)")
find_library(LIBFREETYPE_LIBRARY freetype libfreetype
  PATHS ${LIBASS_DIR}/lib ${FFMPEG_LIBS_PATH}
  DOC "libfreetype (for ssa/ass subtitle renderering)")
find_library(LIBICONV_LIBRARY iconv libiconv
  PATHS ${LIBASS_DIR}/lib ${FFMPEG_LIBS_PATH}
  DOC "libiconv (for ssa/ass subtitle renderering)")
find_library(LIBEXPAT_LIBRARY expat libexpat
  PATHS ${LIBASS_DIR}/lib ${FFMPEG_LIBS_PATH}
  DOC "libexpat (for ssa/ass subtitle renderering)")
find_library(LIBENCA_LIBRARY enca libenca
  PATHS ${LIBASS_DIR}/lib ${FFMPEG_LIBS_PATH}
  DOC "libenca (for ssa/ass subtitle renderering)")
find_library(LIBICUUC_LIBRARY icuuc libicuuc
  PATHS ${LIBASS_DIR}/lib ${FFMPEG_LIBS_PATH}
  DOC "libicuuc (for ssa/ass subtitle renderering)")



set(PORTAUDIO_DIR "$ENV{PORTAUDIO_DIR}" CACHE PATH "root path for portaudio lib/ and include/ folders")
find_library(PORTAUDIO_LIBRARY portaudio PATHS ${PORTAUDIO_DIR}/lib ${FFMPEG_LIBS_PATH} DOC "portaudio library")

set(GLEW_DIR "$ENV{GLEW_DIR}" CACHE PATH "root path for GLEW lib/ and include/ folders")
find_library(GLEW_LIBRARY GLEW PATHS ${GLEW_DIR}/lib ${FFMPEG_LIBS_PATH} DOC "GLEW library")

set(project_moc_hxx
  yaeCanvas.h
  yaeMainWindow.h
  yaeTimelineControls.h
  yaePlaylistWidget.h
  yaeLineEdit.h
  )

set(project_uis
  yaeAbout.ui
  yaeAspectRatioDialog.ui
  yaeMainWindow.ui
  yaeOpenUrlDialog.ui
  )

qt4_add_resources(project_qrc_cxx ${PROGNAME}.qrc)

set(project_sources
  yaeAPI.h
  yaeAPI.cpp
  yaeAudioFragment.h
  yaeAudioRenderer.h
  yaeAudioRenderer.cpp
  yaeAudioRendererPortaudio.h
  yaeAudioRendererPortaudio.cpp
  yaeAudioTempoFilter.h
  yaeAutoCrop.h
  yaeAutoCrop.cpp
  yaeCanvas.h
  yaeCanvas.cpp
  yaeLineEdit.h
  yaeLineEdit.cpp
  yaeMain.cpp
  yaeMainWindow.h
  yaeMainWindow.cpp
  yaePixelFormats.h
  yaePixelFormatFFMPEG.h
  yaePixelFormatFFMPEG.cpp
  yaePixelFormatTraits.h
  yaePixelFormatTraits.cpp
  yaeTimelineControls.h
  yaeTimelineControls.cpp
  yaePlaylistWidget.h
  yaePlaylistWidget.cpp
  yaeQueue.h
  yaeReader.h
  yaeReaderFFMPEG.h
  yaeReaderFFMPEG.cpp
  yaeRemix.cpp
  yaeSynchronous.h
  yaeSynchronous.cpp
  yaeThreading.h
  yaeTree.h
  yaeUtils.h
  yaeUtils.cpp
  yaeVideoRenderer.h
  yaeVideoRenderer.cpp
  )

if (APPLE)
  include_directories(AFTER martinkahr-apple_remote_control)

  set(project_sources
    ${project_sources}
    yaeAppleRemoteControl.h
    yaeAppleRemoteControl.mm
    martinkahr-apple_remote_control/AppleRemote.h
    martinkahr-apple_remote_control/AppleRemote.m
    martinkahr-apple_remote_control/HIDRemoteControlDevice.h
    martinkahr-apple_remote_control/HIDRemoteControlDevice.m
    martinkahr-apple_remote_control/MultiClickRemoteBehavior.h
    martinkahr-apple_remote_control/MultiClickRemoteBehavior.m
    martinkahr-apple_remote_control/RemoteControl.h
    martinkahr-apple_remote_control/RemoteControl.m)
endif (APPLE)

find_path(LIBASS_INCLUDE_DIR
  ass/ass.h
  PATHS
  ${LIBASS_DIR}/include
  ${FFMPEG_HEADERS_PATH}
  /usr/include
  /usr/local/include
  /opt/local/include)

if (LIBASS_INCLUDE_DIR)
  include_directories(AFTER ${LIBASS_INCLUDE_DIR})
endif (LIBASS_INCLUDE_DIR)

find_path(PORTAUDIO_INCLUDE_DIR
  portaudio.h
  PATHS
  ${PORTAUDIO_DIR}/include
  ${FFMPEG_HEADERS_PATH}
  /usr/include
  /usr/local/include
  /opt/local/include)

if (PORTAUDIO_INCLUDE_DIR)
  include_directories(AFTER ${PORTAUDIO_INCLUDE_DIR})
endif (PORTAUDIO_INCLUDE_DIR)

find_path(GLEW_INCLUDE_DIR
  GL/glew.h
  PATHS
  ${GLEW_DIR}/include
  ${FFMPEG_HEADERS_PATH}
  /usr/include
  /usr/local/include
  /opt/local/include)

if (GLEW_INCLUDE_DIR)
  include_directories(AFTER ${GLEW_INCLUDE_DIR})
endif (GLEW_INCLUDE_DIR)

set (YAE_VERSIONED_FILES "${PROJECT_BINARY_DIR}/yaeVersion.h")

if (WIN32)
  set (YAE_VERSIONED_FILES
    ${YAE_VERSIONED_FILES}
    "${PROJECT_BINARY_DIR}/${PROGNAME}.rc")
endif (WIN32)

if (APPLE)
  set (YAE_VERSIONED_FILES
    ${YAE_VERSIONED_FILES}
    "${PROJECT_BINARY_DIR}/${PROGNAME}.plist")
endif (APPLE)

add_custom_target("update revision" ALL
  DEPENDS
#  ${YAE_VERSIONED_FILES}
  "${PROJECT_BINARY_DIR}/yaeVersion.h.tmp"
)

add_custom_command(OUTPUT
#  ${YAE_VERSIONED_FILES}
  "${PROJECT_BINARY_DIR}/yaeVersion.h.tmp"
  COMMAND "${CMAKE_COMMAND}"
  -DPROJECT_SOURCE_DIR="${PROJECT_SOURCE_DIR}"
  -DPROJECT_BINARY_DIR="${PROJECT_BINARY_DIR}"
  -DPROGNAME="${PROGNAME}"
  -P "${PROJECT_SOURCE_DIR}/updateRevision.cmake")

set(project_sources ${project_sources} ${YAE_VERSIONED_FILES})

# Set some Win32 Specific Settings
if (WIN32)
  set(GUI_TYPE WIN32)

  if (MSYS OR MINGW)
    set(CMAKE_RC_COMPILER_INIT windres)
    enable_language(RC)
    set(CMAKE_RC_COMPILE_OBJECT "<CMAKE_RC_COMPILER> <FLAGS> -O coff <DEFINES> -i <SOURCE> -o <OBJECT>")
  endif (MSYS OR MINGW)

endif (WIN32)

# Set some Apple MacOS Specific settings
if (APPLE)
  set(GUI_TYPE MACOSX_BUNDLE)

  set(project_sources
    ${project_sources}
    ${PROGNAME}.icns
    InfoPlist.strings)
endif (APPLE)

qt4_wrap_cpp(project_moc_cxx ${project_moc_hxx})
qt4_wrap_ui(project_uis_hxx ${project_uis})

set(project_sources
  ${project_sources}
  ${project_moc_cxx}
  ${project_uis_hxx}
  ${project_qrc_cxx}
  )

add_executable(${PROGNAME}
  ${GUI_TYPE}
  ${project_sources}
  )

add_dependencies(${PROGNAME} "update revision")

set(TARGET_LIBS
  ${GLEW_LIBRARY}
  ${QT_QTOPENGL_LIBRARY}
  ${QT_LIBRARIES}
  ${OPENGL_LIBRARIES}
  ${Boost_LIBRARIES}
  )

if (LIBASS_LIBRARY)
  set(TARGET_LIBS ${TARGET_LIBS} ${LIBASS_LIBRARY})
  add_definitions(-DYAE_USE_LIBASS)
endif (LIBASS_LIBRARY)
if (LIBHARFBUZZ_LIBRARY)
  set(TARGET_LIBS ${TARGET_LIBS} ${LIBHARFBUZZ_LIBRARY})
endif (LIBHARFBUZZ_LIBRARY)
if (LIBFRIBIDI_LIBRARY)
  set(TARGET_LIBS ${TARGET_LIBS} ${LIBFRIBIDI_LIBRARY})
endif (LIBFRIBIDI_LIBRARY)
if (LIBFONTCONFIG_LIBRARY)
  set(TARGET_LIBS ${TARGET_LIBS} ${LIBFONTCONFIG_LIBRARY})
endif (LIBFONTCONFIG_LIBRARY)
if (LIBFREETYPE_LIBRARY)
  set(TARGET_LIBS ${TARGET_LIBS} ${LIBFREETYPE_LIBRARY})
endif (LIBFREETYPE_LIBRARY)
if (LIBICONV_LIBRARY)
  set(TARGET_LIBS ${TARGET_LIBS} ${LIBICONV_LIBRARY})
endif (LIBICONV_LIBRARY)
if (LIBEXPAT_LIBRARY)
  set(TARGET_LIBS ${TARGET_LIBS} ${LIBEXPAT_LIBRARY})
endif (LIBEXPAT_LIBRARY)
if (LIBENCA_LIBRARY)
  set(TARGET_LIBS ${TARGET_LIBS} ${LIBENCA_LIBRARY})
endif (LIBENCA_LIBRARY)
if (LIBICUUC_LIBRARY)
  set(TARGET_LIBS ${TARGET_LIBS} ${LIBICUUC_LIBRARY})
endif (LIBICUUC_LIBRARY)

if (PORTAUDIO_LIBRARY)
  set(TARGET_LIBS ${TARGET_LIBS} ${PORTAUDIO_LIBRARY})
  if (APPLE)
    find_library(CORE_AUDIO_FRAMEWORK CoreAudio)
    mark_as_advanced(CORE_AUDIO_FRAMEWORK)
    if (CORE_AUDIO_FRAMEWORK)
      set(TARGET_LIBS ${TARGET_LIBS} ${CORE_AUDIO_FRAMEWORK})
    endif (CORE_AUDIO_FRAMEWORK)

    find_library(AUDIO_TOOLBOX_FRAMEWORK AudioToolbox)
    mark_as_advanced(AUDIO_TOOLBOX_FRAMEWORK)
    if (AUDIO_TOOLBOX_FRAMEWORK)
      set(TARGET_LIBS ${TARGET_LIBS} ${AUDIO_TOOLBOX_FRAMEWORK})
    endif (AUDIO_TOOLBOX_FRAMEWORK)

    find_library(AUDIO_UNIT_FRAMEWORK AudioUnit)
    mark_as_advanced(AUDIO_UNIT_FRAMEWORK)
    if (AUDIO_UNIT_FRAMEWORK)
      set(TARGET_LIBS ${TARGET_LIBS} ${AUDIO_UNIT_FRAMEWORK})
    endif (AUDIO_UNIT_FRAMEWORK)
  endif (APPLE)
endif (PORTAUDIO_LIBRARY)

if (AVFORMAT_LIBRARY)
  set(TARGET_LIBS ${TARGET_LIBS} ${AVFORMAT_LIBRARY})
endif (AVFORMAT_LIBRARY)

if (AVCODEC_LIBRARY)
  set(TARGET_LIBS ${TARGET_LIBS} ${AVCODEC_LIBRARY})
endif (AVCODEC_LIBRARY)

if (AVFILTER_LIBRARY)
  set(TARGET_LIBS ${TARGET_LIBS} ${AVFILTER_LIBRARY})
endif (AVFILTER_LIBRARY)

if (AVUTIL_LIBRARY)
  set(TARGET_LIBS ${TARGET_LIBS} ${AVUTIL_LIBRARY})
endif (AVUTIL_LIBRARY)

if (SWSCALE_LIBRARY)
  set(TARGET_LIBS ${TARGET_LIBS} ${SWSCALE_LIBRARY})
endif (SWSCALE_LIBRARY)

if (SWRESAMPLE_LIBRARY)
  set(TARGET_LIBS ${TARGET_LIBS} ${SWRESAMPLE_LIBRARY})
endif (SWRESAMPLE_LIBRARY)

if (POSTPROC_LIBRARY)
  set(TARGET_LIBS ${TARGET_LIBS} ${POSTPROC_LIBRARY})
endif (POSTPROC_LIBRARY)

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

if (CELT_LIBRARY)
  set(TARGET_LIBS ${TARGET_LIBS} ${CELT_LIBRARY})
endif (CELT_LIBRARY)

if (MODPLUG_LIBRARY)
  set(TARGET_LIBS ${TARGET_LIBS} ${MODPLUG_LIBRARY})
endif (MODPLUG_LIBRARY)

#if (FREETYPE_LIBRARY)
#  set(TARGET_LIBS ${TARGET_LIBS} ${FREETYPE_LIBRARY})
#endif (FREETYPE_LIBRARY)

if (GNUTLS_LIBRARY)
  set(TARGET_LIBS ${TARGET_LIBS} ${GNUTLS_LIBRARY})
endif (GNUTLS_LIBRARY)

if (DIRACENC_LIBRARY)
  set(TARGET_LIBS ${TARGET_LIBS} ${DIRACENC_LIBRARY})
endif (DIRACENC_LIBRARY)

if (RTMP_LIBRARY)
  set(TARGET_LIBS ${TARGET_LIBS} ${RTMP_LIBRARY})
endif (RTMP_LIBRARY)

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

if (VA_LIBRARY)
  set(TARGET_LIBS ${TARGET_LIBS} ${VA_LIBRARY})
endif (VA_LIBRARY)

if (ALSA_LIBRARY)
  set(TARGET_LIBS ${TARGET_LIBS} ${ALSA_LIBRARY})
endif (ALSA_LIBRARY)

if (JACK_LIBRARY)
  set(TARGET_LIBS ${TARGET_LIBS} ${JACK_LIBRARY})
endif (JACK_LIBRARY)

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

  if (ICUUC_LIBRARY)
    set(TARGET_LIBS ${TARGET_LIBS} ${ICUUC_LIBRARY})
  endif (ICUUC_LIBRARY)

  if (ICUDT_LIBRARY)
    set(TARGET_LIBS ${TARGET_LIBS} ${ICUDT_LIBRARY})
  endif (ICUDT_LIBRARY)

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
  endif (MSYS OR MINGW)
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

  find_library(CORE_VIDEO_FRAMEWORK CoreVideo)
  mark_as_advanced(CORE_VIDEO_FRAMEWORK)
  if (CORE_VIDEO_FRAMEWORK)
    set(TARGET_LIBS ${TARGET_LIBS} ${CORE_VIDEO_FRAMEWORK})
  endif (CORE_VIDEO_FRAMEWORK)

  find_library(CORE_VDA_FRAMEWORK VideoDecodeAcceleration)
  mark_as_advanced(CORE_VDA_FRAMEWORK)
  if (CORE_VDA_FRAMEWORK)
    set(TARGET_LIBS ${TARGET_LIBS} ${CORE_VDA_FRAMEWORK})
  endif (CORE_VDA_FRAMEWORK)

  find_library(COCOA_FRAMEWORK Cocoa)
  mark_as_advanced(COCOA_FRAMEWORK)
  if (COCOA_FRAMEWORK)
    set(TARGET_LIBS ${TARGET_LIBS} ${COCOA_FRAMEWORK})
  endif (COCOA_FRAMEWORK)

  find_library(IOKIT_FRAMEWORK IOKit)
  mark_as_advanced(IOKIT_FRAMEWORK)
  if (IOKIT_FRAMEWORK)
    set(TARGET_LIBS ${TARGET_LIBS} ${IOKIT_FRAMEWORK})
  endif (IOKIT_FRAMEWORK)
endif (APPLE)

target_link_libraries(${PROGNAME}
  ${TARGET_LIBS}
  )

if (APPLE)
  set_target_properties(${PROGNAME}
    PROPERTIES
    MACOSX_BUNDLE_INFO_PLIST ${PROJECT_BINARY_DIR}/${PROGNAME}.plist)

  set_source_files_properties(${PROJECT_SOURCE_DIR}/${PROGNAME}.icns
    PROPERTIES
    MACOSX_PACKAGE_LOCATION Resources)

  set_source_files_properties(${PROJECT_SOURCE_DIR}/InfoPlist.strings
    PROPERTIES
    MACOSX_PACKAGE_LOCATION Resources/en.lproj)

  add_custom_command(TARGET ${PROGNAME} POST_BUILD
    COMMAND ${PROJECT_SOURCE_DIR}/DeployOSX.sh
    ARGS ${PROJECT_BINARY_DIR}/${CMAKE_CFG_INTDIR}/${PROGNAME}.app
    COMMENT "pull in external dependencies"
    )
endif (APPLE)

if (WIN32)
  add_executable(wixWrapper
    wixWrapper.cpp
    "${PROJECT_BINARY_DIR}/yaeVersion.h"
    )

  add_dependencies(wixWrapper "update revision")

  find_program(DEPENDENCY_WALKER NAMES depends.exe DOC
    "Dependency Walker executable depends.exe, http://www.dependencywalker.com/")

  find_program(WIX_CANDLE NAMES candle.exe DOC
    "WiX compiler executable candle.exe, http://wix.sourceforge.net/")

  find_program(WIX_LIGHT NAMES light.exe DOC
    "WiX installer package linker light.exe, http://wix.sourceforge.net/")

  set(ALLOWED_PATHS ${FFMPEG_LIBS_PATH} CACHE STRING
    "semicolon separated search path where dlls for Qt, portaudio, ffmpeg, etc... can be found")

  set(VC_CRT_MSM
    "C:/Program Files/Common Files/Merge Modules/Microsoft_VC*_CRT_*.msm"
    CACHE FILEPATH "VCRedist CRT .msm merge module")

  if (MINGW OR MSYS)
    string(REPLACE "/" "\\\\" NATIVE_PROGRAM_EXE "${CMAKE_CURRENT_BINARY_DIR}/${CMAKE_CFG_INTDIR}/${PROGNAME}.exe")
    string(REPLACE "/" "\\\\" NATIVE_PROGRAM_ICON "${PROJECT_SOURCE_DIR}/${PROGNAME}.ico")
    string(REPLACE "/" "\\\\" NATIVE_DEPENDS_EXE "${DEPENDENCY_WALKER}")
    string(REPLACE "/" "\\\\" NATIVE_ALLOWED_PATHS "${ALLOWED_PATHS}")
    string(REPLACE "/" "\\\\" NATIVE_CANDLE_EXE "${WIX_CANDLE}")
    string(REPLACE "/" "\\\\" NATIVE_LIGHT_EXE "${WIX_LIGHT}")
    string(REPLACE "/" "\\\\" NATIVE_VC_CRT_MSM "${VC_CRT_MSM}")
  else (MINGW OR MSYS)
    file(TO_NATIVE_PATH "${CMAKE_CURRENT_BINARY_DIR}/${CMAKE_CFG_INTDIR}/${PROGNAME}.exe" NATIVE_PROGRAM_EXE)
    file(TO_NATIVE_PATH "${PROJECT_SOURCE_DIR}/${PROGNAME}.ico" NATIVE_PROGRAM_ICON)
    file(TO_NATIVE_PATH "${DEPENDENCY_WALKER}" NATIVE_DEPENDS_EXE)
    file(TO_NATIVE_PATH "${ALLOWED_PATHS}" NATIVE_ALLOWED_PATHS)
    file(TO_NATIVE_PATH "${WIX_CANDLE}" NATIVE_CANDLE_EXE)
    file(TO_NATIVE_PATH "${WIX_LIGHT}" NATIVE_LIGHT_EXE)
    file(TO_NATIVE_PATH "${VC_CRT_MSM}" NATIVE_VC_CRT_MSM)
  endif (MINGW OR MSYS)

  add_custom_target("create installer"
    ALL DEPENDS
    wixWrapper
    "${PROJECT_BINARY_DIR}/depends-exe-log.txt"
    )

  add_custom_command(OUTPUT "${PROJECT_BINARY_DIR}/depends-exe-log.txt"
    COMMAND wixWrapper
    ARGS
    "${NATIVE_PROGRAM_EXE}"
    "${NATIVE_PROGRAM_ICON}"
    "http://sourceforge.net/projects/apprenticevideo/"
    "${NATIVE_DEPENDS_EXE}"
    "${NATIVE_ALLOWED_PATHS}"
    "${NATIVE_CANDLE_EXE}"
    "${NATIVE_LIGHT_EXE}"
    "${NATIVE_VC_CRT_MSM}"
    DEPENDS ${PROGNAME} wixWrapper
    COMMENT "create installer package"
    )

endif (WIN32)

install(TARGETS ${PROGNAME} DESTINATION bin)
