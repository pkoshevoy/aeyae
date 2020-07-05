cmake_minimum_required(VERSION 3.1.0)

include("${aeyae_SOURCE_DIR}/get_pkgconfig.cmake")

if (PKG_CONFIG_FOUND)
  pkg_check_modules(GLEW glew)
  set(TARGET_LIBS ${TARGET_LIBS} ${GLEW_LIBRARIES})
  include_directories(${GLEW_INCLUDE_DIRS})
  link_directories(${GLEW_LIBRARY_DIRS})
else (PKG_CONFIG_FOUND)
  set(GLEW_INSTALL_PREFIX "$ENV{GLEW_INSTALL_PREFIX}"
    CACHE PATH "search path for glew install path")

  find_path(GLEW_INSTALL_DIR
    include/GL/glew.h
    PATHS
    ${GLEW_INSTALL_PREFIX}
    ${AEYAE_INSTALL_DIR_INCLUDE}
    /usr/include
    /usr/local/include
    /opt/local/include)

  if (GLEW_INSTALL_DIR)
    file(TO_NATIVE_PATH "${GLEW_INSTALL_DIR}/include"
      GLEW_INCLUDE_DIR)
    file(TO_NATIVE_PATH "${GLEW_INSTALL_DIR}/lib"
      GLEW_LIBS_DIR)
    include_directories(AFTER ${GLEW_INCLUDE_DIR})
  endif (GLEW_INSTALL_DIR)
endif (PKG_CONFIG_FOUND)

if (NOT GLEW_FOUND)
  find_library(GLEW_LIBRARY glew
    PATHS ${GLEW_LIBS_DIR} ${AEYAE_INSTALL_DIR_LIB}
    DOC "glew library")
  if (GLEW_LIBRARY)
    set(TARGET_LIBS ${TARGET_LIBS} ${GLEW_LIBRARY})
  endif (GLEW_LIBRARY)
endif ()
