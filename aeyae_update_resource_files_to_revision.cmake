set(CMAKE_INCLUDE_CURRENT_DIR ON)
include("${AEYAE_SOURCE_DIR}/aeyae_get_git_revision.cmake")

if (EXISTS "${PROJECT_SOURCE_DIR}/${PROGNAME}.rc.in")
  configure_file(
    "${PROJECT_SOURCE_DIR}/${PROGNAME}.rc.in"
    "${PROJECT_BINARY_DIR}/${PROGNAME}.rc.tmp")
  execute_process(COMMAND "${CMAKE_COMMAND}" -E copy_if_different
    "${PROJECT_BINARY_DIR}/${PROGNAME}.rc.tmp"
    "${PROJECT_BINARY_DIR}/${PROGNAME}.rc")
  file(REMOVE "${PROJECT_BINARY_DIR}/${PROGNAME}.rc.tmp")
endif ()
