set(CMAKE_INCLUDE_CURRENT_DIR ON)
include("${AEYAE_SOURCE_DIR}/aeyae_get_git_revision.cmake")

configure_file(
  "${AEYAE_SOURCE_DIR}/yae/api/yae_version.h.in"
  "${AEYAE_BINARY_DIR}/yae/api/yae_version.h.tmp")

execute_process(COMMAND "${CMAKE_COMMAND}" -E copy_if_different
  "${AEYAE_BINARY_DIR}/yae/api/yae_version.h.tmp"
  "${AEYAE_BINARY_DIR}/yae/api/yae_version.h")
file(REMOVE "${AEYAE_BINARY_DIR}/yae/api/yae_version.h.tmp")


include("${AEYAE_SOURCE_DIR}/aeyae_update_resource_files_to_revision.cmake")
