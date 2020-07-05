cmake_minimum_required(VERSION 3.1.0)

include("${aeyae_SOURCE_DIR}/get_pkgconfig.cmake")

if (PKG_CONFIG_FOUND)
  pkg_check_modules(AVFORMAT libavformat)
  set(TARGET_LIBS ${TARGET_LIBS} ${AVFORMAT_LIBRARIES})
  include_directories(${AVFORMAT_INCLUDE_DIRS})
  link_directories(${AVFORMAT_LIBRARY_DIRS})

  pkg_check_modules(AVCODEC libavcodec)
  set(TARGET_LIBS ${TARGET_LIBS} ${AVCODEC_LIBRARIES})
  include_directories(${AVCODEC_INCLUDE_DIRS})
  link_directories(${AVCODEC_LIBRARY_DIRS})

  pkg_check_modules(AVUTIL libavutil)
  set(TARGET_LIBS ${TARGET_LIBS} ${AVUTIL_LIBRARIES})
  include_directories(${AVUTIL_INCLUDE_DIRS})
  link_directories(${AVUTIL_LIBRARY_DIRS})

  pkg_check_modules(AVFILTER libavfilter)
  set(TARGET_LIBS ${TARGET_LIBS} ${AVFILTER_LIBRARIES})
  include_directories(${AVFILTER_INCLUDE_DIRS})
  link_directories(${AVFILTER_LIBRARY_DIRS})

  #pkg_check_modules(AVDEVICE libavdevice)
  pkg_check_modules(SWSCALE libswscale)
  set(TARGET_LIBS ${TARGET_LIBS} ${SWSCALE_LIBRARIES})
  include_directories(${SWSCALE_INCLUDE_DIRS})
  link_directories(${SWSCALE_LIBRARY_DIRS})

  pkg_check_modules(SWRESAMPLE libswresample)
  set(TARGET_LIBS ${TARGET_LIBS} ${SWRESAMPLE_LIBRARIES})
  include_directories(${SWRESAMPLE_INCLUDE_DIRS})
  link_directories(${SWRESAMPLE_LIBRARY_DIRS})
else (PKG_CONFIG_FOUND)
  set(FFMPEG_INSTALL_PREFIX "$ENV{FFMPEG_INSTALL_PREFIX}"
    CACHE PATH "search path for ffmpeg install path")

  find_path(FFMPEG_INSTALL_DIR
    include/libavutil/avutil.h
    PATHS
    ${FFMPEG_INSTALL_PREFIX}
    ${AEYAE_INSTALL_DIR_INCLUDE}
    /usr/include
    /usr/local/include
    /opt/local/include)
  if (FFMPEG_INSTALL_DIR)
    file(TO_NATIVE_PATH "${FFMPEG_INSTALL_DIR}/include" FFMPEG_INCLUDE_DIR)
    file(TO_NATIVE_PATH "${FFMPEG_INSTALL_DIR}/lib" FFMPEG_LIBS_DIR)
    include_directories(AFTER ${FFMPEG_INCLUDE_DIR})
  endif (FFMPEG_INSTALL_DIR)
endif (PKG_CONFIG_FOUND)

if (NOT AVFILTER_FOUND)
  find_library(AVFILTER_LIBRARY avfilter
    PATHS ${FFMPEG_LIBS_DIR}
    DOC "ffmpeg avfilter library")
  if (AVFILTER_LIBRARY)
    set(TARGET_LIBS ${TARGET_LIBS} ${AVFILTER_LIBRARY})
  endif (AVFILTER_LIBRARY)
endif ()

if (NOT SWSCALE_FOUND)
  find_library(SWSCALE_LIBRARY swscale
    PATHS ${FFMPEG_LIBS_DIR}
    DOC "ffmpeg swscale library")
  if (SWSCALE_LIBRARY)
    set(TARGET_LIBS ${TARGET_LIBS} ${SWSCALE_LIBRARY})
  endif (SWSCALE_LIBRARY)
endif ()

if (NOT SWRESAMPLE_FOUND)
  find_library(SWRESAMPLE_LIBRARY swresample
    PATHS ${FFMPEG_LIBS_DIR}
    DOC "ffmpeg swresample library")
  if (SWRESAMPLE_LIBRARY)
    set(TARGET_LIBS ${TARGET_LIBS} ${SWRESAMPLE_LIBRARY})
  endif (SWRESAMPLE_LIBRARY)
endif ()

if (NOT AVFORMAT_FOUND)
  find_library(AVFORMAT_LIBRARY avformat
    PATHS ${FFMPEG_LIBS_DIR}
    DOC "ffmpeg avformat library")
  if (AVFORMAT_LIBRARY)
    set(TARGET_LIBS ${TARGET_LIBS} ${AVFORMAT_LIBRARY})
  endif (AVFORMAT_LIBRARY)
endif ()

if (NOT AVUTIL_FOUND)
  find_library(AVUTIL_LIBRARY avutil
    PATHS ${FFMPEG_LIBS_DIR}
    DOC "ffmpeg avutil library")
  if (AVUTIL_LIBRARY)
    set(TARGET_LIBS ${TARGET_LIBS} ${AVUTIL_LIBRARY})
  endif (AVUTIL_LIBRARY)
endif ()

if (NOT AVCODEC_FOUND)
  find_library(AVCODEC_LIBRARY avcodec
    PATHS ${FFMPEG_LIBS_DIR}
    DOC "ffmpeg avcodec library")
  if (AVCODEC_LIBRARY)
    set(TARGET_LIBS ${TARGET_LIBS} ${AVCODEC_LIBRARY})
  endif (AVCODEC_LIBRARY)
endif ()
