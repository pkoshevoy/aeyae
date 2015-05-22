cmake_minimum_required(VERSION 2.8.0)

project(aeyae)

include_directories(AFTER
  .
  ${PROJECT_SOURCE_DIR}/../sf-apprenticevideo
  )

add_subdirectory(tests)
