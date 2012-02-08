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
  yamka.h
  yamkaBytes.h
  yamkaCrc32.h
  yamkaEBML.h
  yamkaElt.h
  yamkaFile.h
  yamkaFileStorage.h
  yamkaHodgePodge.h
  yamkaIStorage.h
  yamkaMatroska.h
  yamkaMixedElements.h
  yamkaPayload.h
  yamkaSharedPtr.h
  yamkaStdInt.h
  yamkaBytes.cpp
  yamkaCrc32.cpp
  yamkaEBML.cpp
  yamkaElt.cpp
  yamkaFile.cpp
  yamkaFileStorage.cpp
  yamkaHodgePodge.cpp
  yamkaIStorage.cpp
  yamkaMatroska.cpp
  yamkaMixedElements.cpp
  yamkaPayload.cpp
  yamkaStdInt.cpp
  )

add_executable(yamkaTest 
  examples/yamkaTest.cpp
  )

target_link_libraries(yamkaTest
  yamka
  )

add_executable(yamkaAddAttachment 
  examples/yamkaAddAttachment.cpp
  )

target_link_libraries(yamkaAddAttachment
  yamka
  )

add_executable(yamkaRemux
  examples/yamkaRemux.cpp
  )

target_link_libraries(yamkaRemux
  yamka
  )

add_executable(yamkaSimplify
  examples/yamkaSimplify.cpp
  )

target_link_libraries(yamkaSimplify
  yamka
  )

add_executable(yamkaFileInfo
  examples/yamkaFileInfo.cpp
  )

target_link_libraries(yamkaFileInfo
  yamka
  )

install(TARGETS 
  yamkaAddAttachment
  yamkaRemux
  yamkaSimplify
  yamkaFileInfo
  DESTINATION bin
  )
