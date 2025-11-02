project(yamka)

cmake_minimum_required(VERSION 3.0.1)

include("${PROJECT_SOURCE_DIR}/updateRevision.cmake")

include_directories(AFTER
  .
  ${PROJECT_BINARY_DIR}
  )

find_package(Boost REQUIRED)
if (Boost_INCLUDE_DIR)
  include_directories(AFTER ${Boost_INCLUDE_DIR})
endif (Boost_INCLUDE_DIR)

if (WIN32)
  add_definitions(-D_USE_MATH_DEFINES)
  add_definitions(-DNOMINMAX)
endif (WIN32)

set (YAMKA_VERSIONED_FILES "${PROJECT_BINARY_DIR}/yamkaVersion.h")

add_custom_target("update_revision_yamka" ALL
  DEPENDS
  "${PROJECT_BINARY_DIR}/yamkaVersion.h.tmp"
)

add_custom_command(OUTPUT
  "${PROJECT_BINARY_DIR}/yamkaVersion.h.tmp"
  COMMAND "${CMAKE_COMMAND}"
  -DPROJECT_SOURCE_DIR="${PROJECT_SOURCE_DIR}"
  -DPROJECT_BINARY_DIR="${PROJECT_BINARY_DIR}"
  -DPROGNAME="${PROGNAME}"
  -P "${PROJECT_SOURCE_DIR}/updateRevision.cmake")

add_library(yamka STATIC
  yamka.h
  yamkaBytes.h
  yamkaCache.h
  yamkaCrc32.h
  yamkaEBML.h
  yamkaElt.h
  yamkaFile.h
  yamkaFileStorage.h
  yamkaHodgePodge.h
  yamkaIStorage.h
  yamkaMatroska.h
  yamkaMemoryStorage.h
  yamkaMixedElements.h
  yamkaPayload.h
  yamkaSharedPtr.h
  yamkaStdInt.h
  yamkaBytes.cpp
  yamkaCache.cpp
  yamkaCrc32.cpp
  yamkaEBML.cpp
  yamkaElt.cpp
  yamkaFile.cpp
  yamkaFileStorage.cpp
  yamkaHodgePodge.cpp
  yamkaIStorage.cpp
  yamkaMatroska.cpp
  yamkaMemoryStorage.cpp
  yamkaMixedElements.cpp
  yamkaPayload.cpp
  yamkaStdInt.cpp
  ${YAMKA_VERSIONED_FILES}
  )

add_dependencies(yamka "update_revision_yamka")

add_executable(yamkaTest
  examples/yamkaTest.cpp
  )

set(YAMKA_LIBS
  yamka
  ${EXTRA_LIBS}
  )

target_link_libraries(yamkaTest
  ${YAMKA_LIBS}
  )

add_executable(yamkaRemux
  examples/yamkaRemux.cpp
  )

target_link_libraries(yamkaRemux
  ${YAMKA_LIBS}
  )

add_executable(yamkaSimplify
  examples/yamkaSimplify.cpp
  )

target_link_libraries(yamkaSimplify
  ${YAMKA_LIBS}
  )

add_executable(yamkaFileInfo
  examples/yamkaFileInfo.cpp
  )

target_link_libraries(yamkaFileInfo
  ${YAMKA_LIBS}
  )

install(TARGETS
  yamkaRemux
  yamkaSimplify
  yamkaFileInfo
  DESTINATION bin
  )
