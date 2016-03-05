project(yae_reader_ffmpeg)

find_package(Boost REQUIRED)

# setup header search paths:
include_directories(AFTER
  .
  ../..
  ${PROJECT_BINARY_DIR}
  )

set(project_sources

  yae_reader_ffmpeg.h
  yae_reader_ffmpeg.cpp
  )

add_library(yae_reader_ffmpeg MODULE
  ${project_sources}
  )

target_link_libraries(yae_reader_ffmpeg PRIVATE
  aeyae
  ${TARGET_LIBS}
  )

set_target_properties(yae_reader_ffmpeg PROPERTIES
  PREFIX "aeyae-plugin-"
  OUTPUT_NAME "reader-ffmpeg"
  SUFFIX ".yae"
  )

set_property(TARGET yae_reader_ffmpeg PROPERTY CXX_STANDARD 98)
# set_property(TARGET yae_reader_ffmpeg PROPERTY CXX_STANDARD 11)

add_dependencies(yae_reader_ffmpeg aeyae)

install(TARGETS
  yae_reader_ffmpeg
  DESTINATION lib
  )
