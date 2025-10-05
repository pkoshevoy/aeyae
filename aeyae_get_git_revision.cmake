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

  execute_process(COMMAND ${GIT_EXECUTABLE} symbolic-ref HEAD -q
    WORKING_DIRECTORY "${PROJECT_SOURCE_DIR}"
    OUTPUT_VARIABLE PROJ_GIT_SYMBOLIC_REF
    OUTPUT_STRIP_TRAILING_WHITESPACE)

  # get the branch name from symbolic-ref refs/heads/<branch>:
  string(SUBSTRING "${PROJ_GIT_SYMBOLIC_REF}" 11 -1 BRANCH)
  unset(PROJ_GIT_SYMBOLIC_REF)

  execute_process(COMMAND ${GIT_EXECUTABLE} log -1
    --pretty=format:%H
    WORKING_DIRECTORY "${PROJECT_SOURCE_DIR}"
    OUTPUT_VARIABLE PROJ_COMMIT_GIT_HASH
    OUTPUT_STRIP_TRAILING_WHITESPACE)

  execute_process(COMMAND ${GIT_EXECUTABLE} log -1
    --pretty=format:%ci
    WORKING_DIRECTORY "${PROJECT_SOURCE_DIR}"
    OUTPUT_VARIABLE PROJ_COMMIT_ISO_DATE
    OUTPUT_STRIP_TRAILING_WHITESPACE)

  # parse ISO date: 2025-10-05 13:22:23 -0300
  string(SUBSTRING "${PROJ_COMMIT_ISO_DATE}" 0 4 YYYY)
  string(SUBSTRING "${PROJ_COMMIT_ISO_DATE}" 5 2 MM)
  string(SUBSTRING "${PROJ_COMMIT_ISO_DATE}" 8 2 DD)
  string(SUBSTRING "${PROJ_COMMIT_ISO_DATE}" 11 2 HOUR)
  string(SUBSTRING "${PROJ_COMMIT_ISO_DATE}" 14 2 MIN)
  string(SUBSTRING "${PROJ_COMMIT_ISO_DATE}" 17 2 SEC)

  # get the short git hash:
  string(SUBSTRING "${PROJ_COMMIT_GIT_HASH}" 0 8 SHORT_HASH)

  set(PROJ_YYMMDD_HHMMSS "${YYYY}${MM}${DD}.${HOUR}${MIN}${SEC}")
  set(PROJ_YY_MM_DD_HHMM "${YYYY}.${MM}.${DD}.${HOUR}${MIN}")

  set(PROJ_WC_REVISION
    "git.${BRANCH}.${YYYY}${MM}${DD}.${HOUR}${MIN}${SEC}.${SHORT_HASH}")

  message("PROJ_WC_REVISION: ${PROJ_WC_REVISION}")

  unset(BRANCH)
  unset(YYYY)
  unset(MM)
  unset(DD)
  unset(HOUR)
  unset(MIN)
  unset(SEC)
  unset(SHORT_HASH)
else()
  set(PROJ_WC_REVISION "0")
endif()
