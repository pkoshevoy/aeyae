find_package(Subversion)
if(Subversion_FOUND)
  Subversion_WC_INFO("${PROJECT_SOURCE_DIR}" PROJ)
else(Subversion_FOUND)
  set(PROJ_WC_REVISION "0")
endif(Subversion_FOUND)

configure_file(
  "${PROJECT_SOURCE_DIR}/yamkaVersion.h.in"
  "${PROJECT_BINARY_DIR}/yamkaVersion.h.tmp")
execute_process(COMMAND "${CMAKE_COMMAND}" -E copy_if_different
  "${PROJECT_BINARY_DIR}/yamkaVersion.h.tmp"
  "${PROJECT_BINARY_DIR}/yamkaVersion.h")
file(REMOVE "${PROJECT_BINARY_DIR}/yamkaVersion.h.tmp")
