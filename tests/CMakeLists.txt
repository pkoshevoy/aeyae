find_package(Threads REQUIRED)

set(Boost_USE_STATIC_LIBS OFF)
set(Boost_USE_MULTITHREAD ON)
find_package(Boost
  COMPONENTS
  unit_test_framework
  REQUIRED)
add_definitions(-DBOOST_ALL_DYN_LINK)
add_definitions(-DBOOST_FILESYSTEM_NO_DEPRECATED)

include_directories(AFTER
  .
  ..
  ${PROJECT_BINARY_DIR}
  ${Boost_INCLUDE_DIR}
  )

add_executable(aeyae-tests
  test_yae_audio_utils.cpp
  test_yae_benchmark.cpp
  test_yae_color_transform.cpp
  test_yae_data.cpp
  test_yae_iso14496.cpp
  test_yae_lru_cache.cpp
  test_yae_settings.cpp
  test_yae_shared_ptr.cpp
  test_yae_timeline.cpp
  test_yae_utils.cpp
  test_yae_wav_file.cpp
  # test_yae_frame_observer.cpp
  # test_yae_log.cpp
  test_yae.cpp
  )

target_compile_definitions(aeyae-tests PRIVATE
  -DJSON_DLL
  -DBOOST_TEST_DYN_LINK)

target_link_libraries(aeyae-tests
  ${TARGET_LIBS}
  ${Boost_LIBRARIES}
  ${CMAKE_THREAD_LIBS_INIT}
  aeyae
  )
