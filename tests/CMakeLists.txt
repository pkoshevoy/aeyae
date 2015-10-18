find_package(Threads REQUIRED)
find_package(Boost
  COMPONENTS
  atomic
  chrono
  date_time
  system
  thread
  unit_test_framework
  REQUIRED)

include_directories(AFTER
  .
  ..
  ${PROJECT_BINARY_DIR}
  ${Boost_INCLUDE_DIR}
  )

add_executable(aeyae-tests
  yae_tests.cpp
  yae_settings_tests.cpp
  yae_shared_ptr_tests.cpp
  yae_benchmark_tests.cpp
  # yae_frame_observer_tests.cpp
  # yae_log_tests.cpp
  )

set_property(TARGET aeyae-tests PROPERTY CXX_STANDARD 98)
# set_property(TARGET aeyae-tests PROPERTY CXX_STANDARD 11)

target_compile_definitions(aeyae-tests PRIVATE -DBOOST_TEST_DYN_LINK)

target_link_libraries(aeyae-tests
  ${Boost_LIBRARIES}
  ${CMAKE_THREAD_LIBS_INIT}
  aeyae
  )
