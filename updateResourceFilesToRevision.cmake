if (EXISTS "${PROJECT_SOURCE_DIR}/${PROGNAME}.rc.in")
  configure_file(
    "${PROJECT_SOURCE_DIR}/${PROGNAME}.rc.in"
    "${PROJECT_BINARY_DIR}/${PROGNAME}.rc.tmp")
  execute_process(COMMAND "${CMAKE_COMMAND}" -E copy_if_different
    "${PROJECT_BINARY_DIR}/${PROGNAME}.rc.tmp"
    "${PROJECT_BINARY_DIR}/${PROGNAME}.rc")
  file(REMOVE "${PROJECT_BINARY_DIR}/${PROGNAME}.rc.tmp")
endif ()

if (EXISTS "${PROJECT_SOURCE_DIR}/${PROGNAME}.plist.in")
  configure_file(
    "${PROJECT_SOURCE_DIR}/${PROGNAME}.plist.in"
    "${PROJECT_BINARY_DIR}/${PROGNAME}.plist.tmp")
  execute_process(COMMAND "${CMAKE_COMMAND}" -E copy_if_different
    "${PROJECT_BINARY_DIR}/${PROGNAME}.plist.tmp"
    "${PROJECT_BINARY_DIR}/${PROGNAME}.plist")
  file(REMOVE "${PROJECT_BINARY_DIR}/${PROGNAME}.plist.tmp")
endif ()
