find_package(Subversion)
if(Subversion_FOUND)
  Subversion_WC_INFO("${PROJECT_SOURCE_DIR}" PROJ)
else(Subversion_FOUND)
  set(PROJ_WC_REVISION "0")
endif(Subversion_FOUND)
