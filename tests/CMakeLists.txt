find_package(Boost REQUIRED)

include_directories(AFTER
  .
  ../api
  ${PROJECT_BINARY_DIR}
  ${Boost_INCLUDE_DIR}
  )

add_executable(aeyae-tests
  aeyae_settings_tests.cxx
  aeyae_shared_ptr_tests.cxx
  aeyae_frame_observer_tests.cxx
  aeyae_log_tests.cxx
  )

target_link_libraries(aeyae-tests
  ${Boost_LIBRARIES}
  )
