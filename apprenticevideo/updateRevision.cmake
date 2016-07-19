include("${PROJECT_SOURCE_DIR}/../getGitRevision.cmake")


configure_file(
  "${PROJECT_SOURCE_DIR}/yaeVersion.h.in"
  "${PROJECT_BINARY_DIR}/yaeVersion.h.tmp")
execute_process(COMMAND "${CMAKE_COMMAND}" -E copy_if_different
  "${PROJECT_BINARY_DIR}/yaeVersion.h.tmp"
  "${PROJECT_BINARY_DIR}/yaeVersion.h")
file(REMOVE "${PROJECT_BINARY_DIR}/yaeVersion.h.tmp")


include("${PROJECT_SOURCE_DIR}/../updateResourceFilesToRevision.cmake")
