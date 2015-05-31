cmake_minimum_required(VERSION 2.8.0)

project(aeyae)

include_directories(AFTER
  .
  api
  apprenticevideo
  )

add_subdirectory(tests)
