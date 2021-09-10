find_package(Threads REQUIRED)

set(Boost_USE_MULTITHREAD ON)
find_package(Boost
  COMPONENTS
  atomic
  chrono
  date_time
  filesystem
  locale
  regex
  system
  thread
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
  yae_benchmark_tests.cpp
  yae_color_transform_tests.cpp
  yae_data_tests.cpp
  yae_lru_cache_tests.cpp
  yae_settings_tests.cpp
  yae_shared_ptr_tests.cpp
  yae_tests.cpp
  yae_timeline_tests.cpp
  yae_utils_tests.cpp
  # yae_frame_observer_tests.cpp
  # yae_log_tests.cpp
  )

set_property(TARGET aeyae-tests PROPERTY CXX_STANDARD 98)
# set_property(TARGET aeyae-tests PROPERTY CXX_STANDARD 11)

target_compile_definitions(aeyae-tests PRIVATE
  -DJSON_DLL
  -DBOOST_TEST_DYN_LINK)

target_link_libraries(aeyae-tests
  ${TARGET_LIBS}
  ${Boost_LIBRARIES}
  ${CMAKE_THREAD_LIBS_INIT}
  aeyae
  )
