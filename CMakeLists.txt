cmake_minimum_required(VERSION 3.1.0)

set(CMAKE_POSITION_INDEPENDENT_CODE ON)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

project(aeyae)

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

if (WIN32 AND NOT (MINGW OR MSYS))
  include_directories(AFTER apprenticevideo/msIntTypes)
endif (WIN32 AND NOT (MINGW OR MSYS))


set(FFMPEG_HEADERS_PATH "$ENV{FFMPEG_HEADERS_PATH}"
  CACHE PATH "search path for ffmpeg headers")

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

set(FFMPEG_LIBS_PATH "$ENV{FFMPEG_LIBS_PATH}"
  CACHE PATH "search path for ffmpeg libraries")

set(TARGET_LIBS
  ${Boost_LIBRARIES}
  ${CMAKE_THREAD_LIBS_INIT}
  )


find_library(AVFORMAT_LIBRARY avformat
  PATHS ${FFMPEG_LIBS_PATH}
  DOC "ffmpeg avformat library")
find_library(AVCODEC_LIBRARY avcodec
  PATHS ${FFMPEG_LIBS_PATH}
  DOC "ffmpeg avutil library")
find_library(AVUTIL_LIBRARY avutil
  PATHS ${FFMPEG_LIBS_PATH}
  DOC "ffmpeg avutil library")
find_library(AVFILTER_LIBRARY avfilter
  PATHS ${FFMPEG_LIBS_PATH}
  DOC "ffmpeg avfilter library")
find_library(SWSCALE_LIBRARY swscale
  PATHS ${FFMPEG_LIBS_PATH}
  DOC "ffmpeg swscale library")
find_library(SWRESAMPLE_LIBRARY swresample
  PATHS ${FFMPEG_LIBS_PATH}
  DOC "ffmpeg swresample library")
find_library(POSTPROC_LIBRARY postproc
  PATHS ${FFMPEG_LIBS_PATH}
  DOC "ffmpeg postproc library")

find_library(MODPLUG_LIBRARY modplug
  PATHS ${FFMPEG_LIBS_PATH}
  DOC "modplug library")
find_library(GNUTLS_LIBRARY gnutls
  PATHS ${FFMPEG_LIBS_PATH}
  DOC "gnutls library")
find_library(ZLIB_LIBRARY z
  PATHS ${FFMPEG_LIBS_PATH}
  DOC "zlib library")
find_library(BZIP2_LIBRARY bz2
  PATHS ${FFMPEG_LIBS_PATH}
  DOC "bzip2 library")
find_library(VA_LIBRARY va
  PATHS ${FFMPEG_LIBS_PATH}
  DOC "va library")
find_library(ALSA_LIBRARY asound
  PATHS ${FFMPEG_LIBS_PATH}
  DOC "alsa library")
find_library(JACK_LIBRARY jack
  PATHS ${FFMPEG_LIBS_PATH}
  DOC "jack library")
find_library(LZMA_LIBRARY lzma
  PATHS ${FFMPEG_LIBS_PATH}
  DOC "lzma library")
find_library(HOGWEED_LIBRARY hogweed
  PATHS ${FFMPEG_LIBS_PATH}
  DOC "hogweed library")
find_library(GMP_LIBRARY gmp
  PATHS ${FFMPEG_LIBS_PATH}
  DOC "gmp library")
find_library(DL_LIBRARY dl
  PATHS ${FFMPEG_LIBS_PATH}
  DOC "dl library")


if (WIN32)
  find_library(LIBGCC_LIBRARY libgcc
  PATHS ${FFMPEG_LIBS_PATH}
    DOC "mingw libgcc library")
  find_library(LIBMINGW32_LIBRARY libmingw32
  PATHS ${FFMPEG_LIBS_PATH}
    DOC "mingw libmingw32 library")
  find_library(LIBMINGWEX_LIBRARY libmingwex
  PATHS ${FFMPEG_LIBS_PATH}
    DOC "mingw libmingwex library")
  find_library(LIBCOLDNAME_LIBRARY libcoldname
  PATHS ${FFMPEG_LIBS_PATH}
    DOC "mingw libcoldname library")
  find_library(LIBMOLDNAME_RELEASE_LIBRARY libmoldname
  PATHS ${FFMPEG_LIBS_PATH}
    DOC "mingw libmoldname library")
  find_library(LIBMOLDNAME_DEBUG_LIBRARY libmoldnamed
  PATHS ${FFMPEG_LIBS_PATH}
    DOC "mingw libmoldnamed library")
  find_library(AVICAP32_LIBRARY avicap32
  PATHS ${FFMPEG_LIBS_PATH}
    DOC "avicap32 library")
  find_library(AVIFIL32_LIBRARY avifil32
  PATHS ${FFMPEG_LIBS_PATH}
    DOC "avifil32 library")
  find_library(ICUUC_LIBRARY icuuc
  PATHS ${FFMPEG_LIBS_PATH}
    DOC "icuuc library")
  find_library(ICUDT_LIBRARY icudt
  PATHS ${FFMPEG_LIBS_PATH}
    DOC "icudt library")
endif (WIN32)

set(LIBASS_DIR "$ENV{LIBASS_DIR}"
  CACHE PATH "root path for libass lib/ and include/ folders")

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

if (AVFILTER_LIBRARY)
  set(TARGET_LIBS ${TARGET_LIBS} ${AVFILTER_LIBRARY})
endif (AVFILTER_LIBRARY)

if (SWSCALE_LIBRARY)
  set(TARGET_LIBS ${TARGET_LIBS} ${SWSCALE_LIBRARY})
endif (SWSCALE_LIBRARY)

if (POSTPROC_LIBRARY)
  set(TARGET_LIBS ${TARGET_LIBS} ${POSTPROC_LIBRARY})
endif (POSTPROC_LIBRARY)

if (AVFORMAT_LIBRARY)
  set(TARGET_LIBS ${TARGET_LIBS} ${AVFORMAT_LIBRARY})
endif (AVFORMAT_LIBRARY)

if (AVCODEC_LIBRARY)
  set(TARGET_LIBS ${TARGET_LIBS} ${AVCODEC_LIBRARY})
endif (AVCODEC_LIBRARY)

if (SWRESAMPLE_LIBRARY)
  set(TARGET_LIBS ${TARGET_LIBS} ${SWRESAMPLE_LIBRARY})
endif (SWRESAMPLE_LIBRARY)

if (AVUTIL_LIBRARY)
  set(TARGET_LIBS ${TARGET_LIBS} ${AVUTIL_LIBRARY})
endif (AVUTIL_LIBRARY)

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

if (OPUS_LIBRARY)
  set(TARGET_LIBS ${TARGET_LIBS} ${OPUS_LIBRARY})
endif (OPUS_LIBRARY)

if (MODPLUG_LIBRARY)
  set(TARGET_LIBS ${TARGET_LIBS} ${MODPLUG_LIBRARY})
endif (MODPLUG_LIBRARY)

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

if (VA_LIBRARY)
  set(TARGET_LIBS ${TARGET_LIBS} ${VA_LIBRARY})
endif (VA_LIBRARY)

if (ZLIB_LIBRARY)
  set(TARGET_LIBS ${TARGET_LIBS} ${ZLIB_LIBRARY})
endif (ZLIB_LIBRARY)

if (BZIP2_LIBRARY)
  set(TARGET_LIBS ${TARGET_LIBS} ${BZIP2_LIBRARY})
endif (BZIP2_LIBRARY)

if (LZMA_LIBRARY)
  set(TARGET_LIBS ${TARGET_LIBS} ${LZMA_LIBRARY})
endif (LZMA_LIBRARY)

if (HOGWEED_LIBRARY)
  set(TARGET_LIBS ${TARGET_LIBS} ${HOGWEED_LIBRARY})
endif (HOGWEED_LIBRARY)

if (GMP_LIBRARY)
  set(TARGET_LIBS ${TARGET_LIBS} ${GMP_LIBRARY})
endif (GMP_LIBRARY)

if (LIBASS_LIBRARY)
  set(TARGET_LIBS ${TARGET_LIBS} ${LIBASS_LIBRARY})
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

if (DL_LIBRARY)
  set(TARGET_LIBS ${TARGET_LIBS} ${DL_LIBRARY})
endif (DL_LIBRARY)

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
  )

add_library(aeyae SHARED

  yae/api/yae_api.h
  yae/api/yae_frame_observer_interface.h
  yae/api/yae_message_carrier_interface.h
  yae/api/yae_plugin_interface.h
  yae/api/yae_settings.h
  yae/api/yae_settings_interface.h
  yae/api/yae_shared_ptr.h

  yae/ffmpeg/yae_audio_fragment.h
  yae/ffmpeg/yae_audio_tempo_filter.h
  yae/ffmpeg/yae_audio_track.cpp
  yae/ffmpeg/yae_audio_track.h
  yae/ffmpeg/yae_closed_captions.cpp
  yae/ffmpeg/yae_closed_captions.h
  yae/ffmpeg/yae_demuxer.cpp
  yae/ffmpeg/yae_demuxer.h
  yae/ffmpeg/yae_pixel_format_ffmpeg.cpp
  yae/ffmpeg/yae_pixel_format_ffmpeg.h
  yae/ffmpeg/yae_ffmpeg_audio_filter_graph.cpp
  yae/ffmpeg/yae_ffmpeg_audio_filter_graph.h
  yae/ffmpeg/yae_ffmpeg_utils.cpp
  yae/ffmpeg/yae_ffmpeg_utils.h
  yae/ffmpeg/yae_ffmpeg_video_filter_graph.cpp
  yae/ffmpeg/yae_ffmpeg_video_filter_graph.h
  yae/ffmpeg/yae_movie.cpp
  yae/ffmpeg/yae_movie.h
  yae/ffmpeg/yae_subtitles_track.cpp
  yae/ffmpeg/yae_subtitles_track.h
  yae/ffmpeg/yae_track.cpp
  yae/ffmpeg/yae_track.h
  yae/ffmpeg/yae_video_track.cpp
  yae/ffmpeg/yae_video_track.h

  yae/thread/yae_queue.h
  yae/thread/yae_threading.h
  yae/thread/yae_task_runner.cpp
  yae/thread/yae_task_runner.h

  yae/utils/yae_benchmark.cpp
  yae/utils/yae_benchmark.h
  yae/utils/yae_log.h
  yae/utils/yae_pgm_file.h
  yae/utils/yae_plugin_registry.cpp
  yae/utils/yae_plugin_registry.h
  yae/utils/yae_time.cpp
  yae/utils/yae_time.h
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
  yae/video/yae_pixel_format_traits.cpp
  yae/video/yae_pixel_format_traits.h
  yae/video/yae_pixel_formats.h
  yae/video/yae_reader.h
  yae/video/yae_synchronous.cpp
  yae/video/yae_synchronous.h
  yae/video/yae_video.cpp
  yae/video/yae_video.h
  yae/video/yae_video_canvas.h
  yae/video/yae_video_renderer.cpp
  yae/video/yae_video_renderer.h
  )

set_property(TARGET aeyae PROPERTY CXX_STANDARD 98)
# set_property(TARGET aeyae PROPERTY CXX_STANDARD 11)

target_compile_definitions(aeyae PRIVATE -DYAE_DLL_EXPORTS)

target_link_libraries(aeyae PRIVATE ${TARGET_LIBS})

add_subdirectory(plugins/yae_reader_ffmpeg)
add_subdirectory(tests)
add_subdirectory(apprenticevideo)
add_subdirectory(yamka)
add_subdirectory(replay)

if (NOT (WIN32 OR APPLE))
  install(TARGETS aeyae DESTINATION lib)
endif()
