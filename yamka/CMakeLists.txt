project(yamka)

cmake_minimum_required(VERSION 2.8.0)
include_directories(AFTER .)

find_package(Boost REQUIRED)
if (Boost_INCLUDE_DIR)
  include_directories(AFTER ${Boost_INCLUDE_DIR})
endif (Boost_INCLUDE_DIR)

if (WIN32)
  add_definitions(-D_USE_MATH_DEFINES)
  add_definitions(-DNOMINMAX)
endif (WIN32)

add_library(yamka STATIC
  yamkaCrc32.h
  yamkaEBML.h
  yamkaElt.h
  yamkaFile.h
  yamkaFileStorage.h
  yamka.h
  yamkaIStorage.h
  yamkaMatroska.h
  yamkaPayload.h
  yamkaStdInt.h
  yamkaBytes.h
  yamkaBytes.cpp
  yamkaCrc32.cpp
  yamkaEBML.cpp
  yamkaElt.cpp
  yamkaFile.cpp
  yamkaFileStorage.cpp
  yamkaIStorage.cpp
  yamkaMatroska.cpp
  yamkaPayload.cpp
  yamkaStdInt.cpp
  )

add_executable(yamkaTest 
  yamkaTest.cpp
  )

target_link_libraries(yamkaTest
  yamka
  )

add_executable(yamkaAddAttachment 
  yamkaAddAttachment.cpp
  )

target_link_libraries(yamkaAddAttachment
  yamka
  )

add_executable(yamkaRemux
  yamkaRemux.cpp
  )

target_link_libraries(yamkaRemux
  yamka
  )

install(TARGETS 
  yamkaTest
  yamkaAddAttachment
  yamkaRemux
  DESTINATION bin
  )
