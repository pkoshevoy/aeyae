cmake_minimum_required(VERSION 2.8.0)

project(aeyae)

include_directories(AFTER
  .
  apprenticevideo
  )

add_subdirectory(tests)

add_subdirectory(apprenticevideo)
