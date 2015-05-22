find_package(Boost REQUIRED)

include_directories(AFTER
  .
  ../api
  ${PROJECT_BINARY_DIR}
  ${Boost_INCLUDE_DIR}
  )

add_executable(aeyae-tests
  main.cxx
  )

target_link_libraries(aeyae-tests
  ${Boost_LIBRARIES}
  )
