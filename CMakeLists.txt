cmake_minimum_required(VERSION 3.1.0)

set(CMAKE_POSITION_INDEPENDENT_CODE ON)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

set(PROGNAME "aeyae")
project(aeyae)

include("${PROJECT_SOURCE_DIR}/aeyae_get_git_revision.cmake")
include("${PROJECT_SOURCE_DIR}/add_ffmpeg.cmake")
include("${PROJECT_SOURCE_DIR}/add_libass.cmake")
include("${PROJECT_SOURCE_DIR}/add_freetype.cmake")
include("${PROJECT_SOURCE_DIR}/add_fontconfig.cmake")
include("${PROJECT_SOURCE_DIR}/add_fribidi.cmake")
include("${PROJECT_SOURCE_DIR}/add_harfbuzz.cmake")

find_package(Threads REQUIRED)

set(Boost_USE_MULTITHREAD ON)
find_package(Boost
  COMPONENTS
  thread
  date_time
  system
  filesystem
  locale
  chrono
  atomic
  regex
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
  add_definitions(-D__ASSERTMACROS__)
endif (APPLE)

if (WIN32 AND NOT (MINGW OR MSYS))
  include_directories(AFTER apprenticevideo/msIntTypes)
endif (WIN32 AND NOT (MINGW OR MSYS))

file(TO_NATIVE_PATH
  "${CMAKE_INSTALL_PREFIX}/lib"
  INSTALL_LIB_DIR)

if (QT4_FOUND)
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
endif (QT4_FOUND)

set(TARGET_LIBS
  ${TARGET_LIBS}
  ${Boost_LIBRARIES}
  ${CMAKE_THREAD_LIBS_INIT}
  )


if (WIN32)
  find_library(LIBGCC_LIBRARY libgcc
  PATHS ${INSTALL_LIB_DIR}
    DOC "mingw libgcc library")
  find_library(LIBMINGW32_LIBRARY libmingw32
  PATHS ${INSTALL_LIB_DIR}
    DOC "mingw libmingw32 library")
  find_library(LIBMINGWEX_LIBRARY libmingwex
  PATHS ${INSTALL_LIB_DIR}
    DOC "mingw libmingwex library")
  find_library(LIBCOLDNAME_LIBRARY libcoldname
  PATHS ${INSTALL_LIB_DIR}
    DOC "mingw libcoldname library")
  find_library(LIBMOLDNAME_RELEASE_LIBRARY libmoldname
  PATHS ${INSTALL_LIB_DIR}
    DOC "mingw libmoldname library")
  find_library(LIBMOLDNAME_DEBUG_LIBRARY libmoldnamed
  PATHS ${INSTALL_LIB_DIR}
    DOC "mingw libmoldnamed library")
  find_library(AVICAP32_LIBRARY avicap32
  PATHS ${INSTALL_LIB_DIR}
    DOC "avicap32 library")
  find_library(AVIFIL32_LIBRARY avifil32
  PATHS ${INSTALL_LIB_DIR}
    DOC "avifil32 library")
endif (WIN32)

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

  find_library(SECURITY_FRAMEWORK Security)
  mark_as_advanced(SECURITY_FRAMEWORK)
  if (SECURITY_FRAMEWORK)
    set(TARGET_LIBS ${TARGET_LIBS} ${SECURITY_FRAMEWORK})
  endif (SECURITY_FRAMEWORK)
endif (APPLE)

include_directories(AFTER
  .
  ${PROJECT_BINARY_DIR}
  ${Boost_INCLUDE_DIR}
  jsoncpp/include
  )

set(jsoncpp_sources
  jsoncpp/include/json/config.h
  jsoncpp/include/json/forwards.h
  jsoncpp/include/json/features.h
  jsoncpp/include/json/value.h
  jsoncpp/include/json/reader.h
  jsoncpp/include/json/writer.h
  jsoncpp/include/json/assertions.h
  jsoncpp/include/json/version.h

  jsoncpp/src/lib_json/json_tool.h
  jsoncpp/src/lib_json/json_reader.cpp
  jsoncpp/src/lib_json/json_valueiterator.inl
  jsoncpp/src/lib_json/json_value.cpp
  jsoncpp/src/lib_json/json_writer.cpp
  )

add_library(${PROGNAME} SHARED

  ${jsoncpp_sources}

  yae/api/yae_api.h
  yae/api/yae_assert.h
  yae/api/yae_frame_observer_interface.h
  yae/api/yae_log.cpp
  yae/api/yae_log.h
  yae/api/yae_message_carrier_interface.h
  yae/api/yae_plugin_interface.h
  yae/api/yae_settings.h
  yae/api/yae_settings_interface.h
  yae/api/yae_shared_ptr.h
  yae/api/yae_version.cpp

  yae/ffmpeg/yae_audio_fragment.h
  yae/ffmpeg/yae_audio_tempo_filter.h
  yae/ffmpeg/yae_audio_track.cpp
  yae/ffmpeg/yae_audio_track.h
  yae/ffmpeg/yae_closed_captions.cpp
  yae/ffmpeg/yae_closed_captions.h
  yae/ffmpeg/yae_demuxer.cpp
  yae/ffmpeg/yae_demuxer.h
  yae/ffmpeg/yae_demuxer_reader.cpp
  yae/ffmpeg/yae_demuxer_reader.h
  yae/ffmpeg/yae_pixel_format_ffmpeg.cpp
  yae/ffmpeg/yae_pixel_format_ffmpeg.h
  yae/ffmpeg/yae_ffmpeg_audio_filter_graph.cpp
  yae/ffmpeg/yae_ffmpeg_audio_filter_graph.h
  yae/ffmpeg/yae_ffmpeg_utils.cpp
  yae/ffmpeg/yae_ffmpeg_utils.h
  yae/ffmpeg/yae_ffmpeg_video_filter_graph.cpp
  yae/ffmpeg/yae_ffmpeg_video_filter_graph.h
  yae/ffmpeg/yae_live_reader.cpp
  yae/ffmpeg/yae_live_reader.h
  yae/ffmpeg/yae_movie.cpp
  yae/ffmpeg/yae_movie.h
  yae/ffmpeg/yae_reader_ffmpeg.cpp
  yae/ffmpeg/yae_reader_ffmpeg.h
  yae/ffmpeg/yae_remux.cpp
  yae/ffmpeg/yae_remux.h
  yae/ffmpeg/yae_subtitles_track.cpp
  yae/ffmpeg/yae_subtitles_track.h
  yae/ffmpeg/yae_track.cpp
  yae/ffmpeg/yae_track.h
  yae/ffmpeg/yae_video_track.cpp
  yae/ffmpeg/yae_video_track.h

  yae/thread/yae_queue.h
  yae/thread/yae_ring_buffer.cpp
  yae/thread/yae_ring_buffer.h
  yae/thread/yae_threading.h
  yae/thread/yae_task_runner.cpp
  yae/thread/yae_task_runner.h
  yae/thread/yae_worker.cpp
  yae/thread/yae_worker.h

  yae/utils/yae_benchmark.cpp
  yae/utils/yae_benchmark.h
  yae/utils/yae_data.cpp
  yae/utils/yae_data.h
  yae/utils/yae_json.cpp
  yae/utils/yae_json.h
  yae/utils/yae_linear_algebra.h
  yae/utils/yae_lru_cache.h
  yae/utils/yae_pgm_file.h
  yae/utils/yae_plugin_registry.cpp
  yae/utils/yae_plugin_registry.h
  yae/utils/yae_time.cpp
  yae/utils/yae_time.h
  yae/utils/yae_timesheet.cpp
  yae/utils/yae_timesheet.h
  yae/utils/yae_tree.h
  yae/utils/yae_type_name.h
  yae/utils/yae_utils.cpp
  yae/utils/yae_utils.h
  yae/utils/yae_wav_file.h

  yae/video/yae_audio_renderer.cpp
  yae/video/yae_audio_renderer.h
  yae/video/yae_audio_renderer_input.cpp
  yae/video/yae_audio_renderer_input.h
  yae/video/yae_auto_crop.cpp
  yae/video/yae_auto_crop.h
  yae/video/yae_colorspace.cpp
  yae/video/yae_colorspace.h
  yae/video/yae_istream.h
  yae/video/yae_mpeg_ts.cpp
  yae/video/yae_mpeg_ts.h
  yae/video/yae_pixel_format_traits.cpp
  yae/video/yae_pixel_format_traits.h
  yae/video/yae_pixel_formats.h
  yae/video/yae_reader_factory.cpp
  yae/video/yae_reader_factory.h
  yae/video/yae_reader.h
  yae/video/yae_recording.cpp
  yae/video/yae_recording.h
  yae/video/yae_synchronous.cpp
  yae/video/yae_synchronous.h
  yae/video/yae_video.cpp
  yae/video/yae_video.h
  yae/video/yae_video_canvas.h
  yae/video/yae_video_renderer.cpp
  yae/video/yae_video_renderer.h
  )

add_custom_command(
  OUTPUT "${PROJECT_BINARY_DIR}/yae/api/yae_version.h.tmp"
  BYPRODUCTS "${PROJECT_BINARY_DIR}/yae/api/yae_version.h"
  MAIN_DEPENDENCY "${PROJECT_SOURCE_DIR}/yae/api/yae_version.h.in"
  COMMAND "${CMAKE_COMMAND}"
  -DAEYAE_SOURCE_DIR="${aeyae_SOURCE_DIR}"
  -DAEYAE_BINARY_DIR="${aeyae_BINARY_DIR}"
  -DPROJECT_SOURCE_DIR="${PROJECT_SOURCE_DIR}"
  -DPROJECT_BINARY_DIR="${PROJECT_BINARY_DIR}"
  -DPROGNAME="${PROGNAME}"
  -DGIT_EXECUTABLE="${GIT_EXECUTABLE}"
  -DCMAKE_INSTALL_PREFIX="${CMAKE_INSTALL_PREFIX}"
  -P "${PROJECT_SOURCE_DIR}/aeyae_update_revision.cmake")

add_custom_target("aeyae_update_revision" ALL
  DEPENDS "${PROJECT_BINARY_DIR}/yae/api/yae_version.h.tmp"
  COMMAND "${CMAKE_COMMAND}" -E echo "updating ${PROJECT_BINARY_DIR}/yae/api/yae_version.h")

add_dependencies(aeyae "aeyae_update_revision")

set_property(TARGET aeyae PROPERTY CXX_STANDARD 98)
# set_property(TARGET aeyae PROPERTY CXX_STANDARD 11)

target_compile_definitions(aeyae PRIVATE
  -DYAE_DLL_EXPORTS
  -DJSON_DLL_BUILD
  )

target_link_libraries(aeyae PRIVATE ${TARGET_LIBS})

add_subdirectory(tests)
add_subdirectory(yaeui)
add_subdirectory(apprenticevideo)
add_subdirectory(apprenticevideo-classic)
add_subdirectory(aeyaeremux)
add_subdirectory(yaetv)
add_subdirectory(yamka)
#add_subdirectory(yathe)

if (NOT (WIN32 OR APPLE))
  install(TARGETS aeyae DESTINATION lib)
endif()
