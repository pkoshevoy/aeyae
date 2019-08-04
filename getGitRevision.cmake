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
    --pretty=format:%cd
    --date=format:%Y%m%d.%H%M%S
    WORKING_DIRECTORY "${PROJECT_SOURCE_DIR}"
    OUTPUT_VARIABLE PROJ_YYMMDD_HHMMSS
    OUTPUT_STRIP_TRAILING_WHITESPACE)
  execute_process(COMMAND ${GIT_EXECUTABLE} symbolic-ref --short HEAD -q
    WORKING_DIRECTORY "${PROJECT_SOURCE_DIR}"
    OUTPUT_VARIABLE PROJ_GIT_BRANCH
    OUTPUT_STRIP_TRAILING_WHITESPACE)
  execute_process(COMMAND ${GIT_EXECUTABLE} log -1
    --pretty=format:%cd.%h
    --date=format:%Y%m%d.%H%M%S
    WORKING_DIRECTORY "${PROJECT_SOURCE_DIR}"
    OUTPUT_VARIABLE PROJ_GIT_REVISION
    OUTPUT_STRIP_TRAILING_WHITESPACE)
  set(PROJ_WC_REVISION "git.${PROJ_GIT_BRANCH}.${PROJ_GIT_REVISION}")
  execute_process(COMMAND ${GIT_EXECUTABLE} log -1
    --pretty=format:%cd
    --date=format:%Y.%m.%d.%H%M
    WORKING_DIRECTORY "${PROJECT_SOURCE_DIR}"
    OUTPUT_VARIABLE PROJ_YY_MM_DD_HHMM
    OUTPUT_STRIP_TRAILING_WHITESPACE)
  message("PROJ_WC_REVISION: ${PROJ_WC_REVISION}")
  set(PROJ_WC_REVISION "git.${PROJ_GIT_BRANCH}.${PROJ_GIT_REVISION}")
else()
  set(PROJ_WC_REVISION "0")
endif()
