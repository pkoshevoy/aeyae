#find_package(Subversion)
#if(Subversion_FOUND)
#  Subversion_WC_INFO("${PROJECT_SOURCE_DIR}" PROJ)
#else(Subversion_FOUND)
#  set(PROJ_WC_REVISION "0")
#endif(Subversion_FOUND)

# FIXME: must figure out how to map git short hash or git tag
# to a revision number
find_package(Git)
if(GIT_FOUND)
  message("git found: ${GIT_EXECUTABLE}")
  execute_process(COMMAND ${GIT_EXECUTABLE} log -1
    --pretty=format:git-%cd-%h
    --date=short
    WORKING_DIRECTORY "${PROJECT_SOURCE_DIR}"
    OUTPUT_VARIABLE PROJ_WC_REVISION)
  message("PROJ_WC_REVISION: ${PROJ_WC_REVISION}")
else()
  set(PROJ_WC_REVISION "0")
endif()


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
