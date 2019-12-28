include("${PROJECT_SOURCE_DIR}/../aeyae_get_git_revision.cmake")


configure_file(
  "${PROJECT_SOURCE_DIR}/yamkaVersion.h.in"
  "${PROJECT_BINARY_DIR}/yamkaVersion.h.tmp")
execute_process(COMMAND "${CMAKE_COMMAND}" -E copy_if_different
  "${PROJECT_BINARY_DIR}/yamkaVersion.h.tmp"
  "${PROJECT_BINARY_DIR}/yamkaVersion.h")
file(REMOVE "${PROJECT_BINARY_DIR}/yamkaVersion.h.tmp")
