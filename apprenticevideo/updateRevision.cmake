include("${PROJECT_SOURCE_DIR}/../getGitRevision.cmake")


configure_file(
  "${PROJECT_SOURCE_DIR}/yaeVersion.h.in"
  "${PROJECT_BINARY_DIR}/yaeVersion.h.tmp")
execute_process(COMMAND "${CMAKE_COMMAND}" -E copy_if_different
  "${PROJECT_BINARY_DIR}/yaeVersion.h.tmp"
  "${PROJECT_BINARY_DIR}/yaeVersion.h")
file(REMOVE "${PROJECT_BINARY_DIR}/yaeVersion.h.tmp")

if (WIN32)
  configure_file(
    "${PROJECT_SOURCE_DIR}/${PROGNAME}.rc.in"
    "${PROJECT_BINARY_DIR}/${PROGNAME}.rc.tmp")
  execute_process(COMMAND "${CMAKE_COMMAND}" -E copy_if_different
    "${PROJECT_BINARY_DIR}/${PROGNAME}.rc.tmp"
    "${PROJECT_BINARY_DIR}/${PROGNAME}.rc")
  file(REMOVE "${PROJECT_BINARY_DIR}/${PROGNAME}.rc.tmp")
endif (WIN32)

if (APPLE)
  configure_file(
    "${PROJECT_SOURCE_DIR}/${PROGNAME}.plist.in"
    "${PROJECT_BINARY_DIR}/${PROGNAME}.plist.tmp")
  execute_process(COMMAND "${CMAKE_COMMAND}" -E copy_if_different
    "${PROJECT_BINARY_DIR}/${PROGNAME}.plist.tmp"
    "${PROJECT_BINARY_DIR}/${PROGNAME}.plist")
  file(REMOVE "${PROJECT_BINARY_DIR}/${PROGNAME}.plist.tmp")
endif (APPLE)
