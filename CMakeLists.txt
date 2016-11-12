cmake_minimum_required(VERSION 3.1.0)

set(CMAKE_POSITION_INDEPENDENT_CODE ON)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

project(aeyae)

find_package(Threads REQUIRED)

set(Boost_USE_MULTITHREAD ON)
find_package(Boost
  COMPONENTS
  thread
  date_time
  system
  filesystem
  REQUIRED)
add_definitions(-DBOOST_ALL_DYN_LINK)
add_definitions(-DBOOST_FILESYSTEM_NO_DEPRECATED)

add_definitions(-D__STDC_CONSTANT_MACROS)
add_definitions(-D__STDC_LIMIT_MACROS)

if (WIN32)
  add_definitions(-D_USE_MATH_DEFINES)
  add_definitions(-DNOMINMAX)

  # for windows xp:
  add_definitions(-D_WIN32_WINNT=0x0501)
  add_definitions(-DWINVER=0x0501)
endif (WIN32)

if (APPLE)
  add_definitions(-D__ASSERT_MACROS_DEFINE_VERSIONS_WITHOUT_UNDERSCORES=0)
endif (APPLE)

if (WIN32 AND NOT (MINGW OR MSYS))
  include_directories(AFTER apprenticevideo/msIntTypes)
endif (WIN32 AND NOT (MINGW OR MSYS))

include_directories(AFTER
  .
  apprenticevideo
  ${PROJECT_BINARY_DIR}
  ${Boost_INCLUDE_DIR}
  )

add_library(aeyae SHARED

  yae/api/aeyae_api.hxx
  yae/api/aeyae_frame_observer_interface.hxx
  yae/api/aeyae_message_carrier_interface.hxx
  yae/api/aeyae_plugin_interface.hxx
  yae/api/aeyae_settings.hxx
  yae/api/aeyae_settings_interface.hxx

  yae/utils/aeyae_log.hxx
  yae/utils/aeyae_shared_ptr.hxx
  yae/utils/aeyae_type_name.hxx
  )

set(TARGET_LIBS
  ${Boost_LIBRARIES}
  ${CMAKE_THREAD_LIBS_INIT}
  )

set_target_properties(aeyae PROPERTIES
  LINKER_LANGUAGE CXX
  CXX_STANDARD 98)
target_compile_definitions(aeyae PRIVATE -DYAE_DLL_EXPORTS)
target_link_libraries(aeyae PRIVATE ${TARGET_LIBS})

add_subdirectory(tests)
add_subdirectory(apprenticevideo)
