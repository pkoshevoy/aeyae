cmake_minimum_required(VERSION 3.1.0)

project(epg)
set(PROGNAME "yaepg")

set(Boost_USE_MULTITHREAD ON)
find_package(Boost
  COMPONENTS
  thread
  date_time
  system
  filesystem
  locale
  REQUIRED)
add_definitions(-DBOOST_ALL_DYN_LINK)
add_definitions(-DBOOST_FILESYSTEM_NO_DEPRECATED)

add_definitions(-D__STDC_CONSTANT_MACROS)
add_definitions(-D__STDC_FORMAT_MACROS)
add_definitions(-D__STDC_LIMIT_MACROS)

if (WIN32)
  add_definitions(-D_USE_MATH_DEFINES)
  add_definitions(-DNOMINMAX)

  # for windows xp:
  add_definitions(-D_WIN32_WINNT=0x0501)
  add_definitions(-DWINVER=0x0501)
endif (WIN32)

if (APPLE)
  add_definitions(-D__ASSERT_MACROS_DEFINE_VERSIONS_WITHOUT_UNDERSCORES=0)
endif (APPLE)

include_directories(AFTER
  .
  ..
  ../apprenticevideo
  ../jsoncpp/include
  ${PROJECT_BINARY_DIR}
  ${PROJECT_BINARY_DIR}/../apprenticevideo
  ${Boost_INCLUDE_DIR}
  )

if (WIN32 AND NOT (MINGW OR MSYS))
  include_directories(AFTER ../apprenticevideo/msIntTypes)
endif ()

set(FFMPEG_SOURCE_PATH "$ENV{FFMPEG_SOURCE_DIR}"
  CACHE PATH "search path for ffmpeg source tree")

find_path(FFMPEG_SOURCE_DIR
  libavcodec/get_bits.h
  PATHS
  ${FFMPEG_SOURCE_PATH}
  "$ENV{HOME}/src/ffmpeg-git-src"
  "$ENV{HOME}/src/ffmpeg"
  /Developer/ffmpeg-git-src
  /Developer/ffmpeg)

if (FFMPEG_SOURCE_DIR)
  include_directories(AFTER
    ${FFMPEG_SOURCE_DIR}
    )
endif (FFMPEG_SOURCE_DIR)


set(FFMPEG_BUILD_PATH "$ENV{FFMPEG_BUILD_DIR}"
  CACHE PATH "search path for ffmpeg build tree")

string(TOLOWER "${CMAKE_BUILD_TYPE}" lowercase_build_type)
find_path(FFMPEG_BUILD_DIR
  config.h
  PATHS
  ${FFMPEG_BUILD_PATH}
  "$ENV{HOME}/x86_64/build/ffmpeg-git-build-${lowercase_build_type}"
  "$ENV{HOME}/x86_64/build/ffmpeg-git-${lowercase_build_type}"
  "$ENV{HOME}/x86_64/build/ffmpeg-${lowercase_build_type}"
  "$ENV{HOME}/x86_64/build/ffmpeg"
  /Developer/x86_64/build/ffmpeg-git-build-${lowercase_build_type}
  /Developer/x86_64/build/ffmpeg-git-${lowercase_build_type}
  /Developer/x86_64/build/ffmpeg-${lowercase_build_type}
  /Developer/x86_64/build/ffmpeg)

if (FFMPEG_BUILD_DIR)
  include_directories(AFTER
    ${FFMPEG_BUILD_DIR}
    )
endif (FFMPEG_BUILD_DIR)


set(FFMPEG_LIBS_PATH "$ENV{FFMPEG_LIBS_PATH}"
  CACHE PATH "search path for ffmpeg libraries")

set(TARGET_LIBS
  ${Boost_LIBRARIES}
  ${CMAKE_THREAD_LIBS_INIT}
  )


find_library(AVFORMAT_LIBRARY avformat
  PATHS ${FFMPEG_LIBS_PATH}
  DOC "ffmpeg avformat library")

if (AVFORMAT_LIBRARY)
  set(TARGET_LIBS ${TARGET_LIBS} ${AVFORMAT_LIBRARY})
endif (AVFORMAT_LIBRARY)


find_library(AVCODEC_LIBRARY avcodec
  PATHS ${FFMPEG_LIBS_PATH}
  DOC "ffmpeg avutil library")

if (AVCODEC_LIBRARY)
  set(TARGET_LIBS ${TARGET_LIBS} ${AVCODEC_LIBRARY})
endif (AVCODEC_LIBRARY)


find_library(AVUTIL_LIBRARY avutil
  PATHS ${FFMPEG_LIBS_PATH}
  DOC "ffmpeg avutil library")

if (AVUTIL_LIBRARY)
  set(TARGET_LIBS ${TARGET_LIBS} ${AVUTIL_LIBRARY})
endif (AVUTIL_LIBRARY)


set(HDHOMERUN_INCLUDE_PATH "$ENV{HDHOMERUN_INCLUDE_DIR}"
  CACHE PATH "search path for hdhomerun headers")

find_path(HDHOMERUN_INCLUDE_DIR
  hdhomerun_device.h
  PATHS
  ${HDHOMERUN_INCLUDE_PATH}
  ${CMAKE_INSTALL_PREFIX}/include
  "$ENV{HOME}/x86_64/include"
  /Developer/x86_64/include)

if (HDHOMERUN_INCLUDE_DIR)
  include_directories(AFTER
    ${HDHOMERUN_INCLUDE_DIR}
    )
endif (HDHOMERUN_INCLUDE_DIR)


set(HDHOMERUN_LIB_PATH "$ENV{HDHOMERUN_LIBS_PATH}"
  CACHE PATH "search path for hdhomerun library")

find_library(HDHOMERUN_LIBRARY hdhomerun
  PATHS ${HDHOMERUN_LIB_PATH}
  DOC "hdhomerun library")

if (HDHOMERUN_LIBRARY)
  set(TARGET_LIBS ${TARGET_LIBS} ${HDHOMERUN_LIBRARY})
endif (HDHOMERUN_LIBRARY)


set(TARGET_LIBS
  aeyae
  ${TARGET_LIBS}
  ${Boost_LIBRARIES}
  ${CMAKE_THREAD_LIBS_INIT}
  )

set(project_sources
  epg.cpp
  )

add_executable(${PROGNAME}
  ${project_sources}
  )

target_link_libraries(${PROGNAME}
  ${TARGET_LIBS}
  )

set_property(TARGET ${PROGNAME} PROPERTY CXX_STANDARD 98)
add_dependencies(${PROGNAME} aeyae)
