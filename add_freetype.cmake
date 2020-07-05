cmake_minimum_required(VERSION 3.1.0)

include("${aeyae_SOURCE_DIR}/get_pkgconfig.cmake")

if (PKG_CONFIG_FOUND)
  pkg_check_modules(FREETYPE freetype2)
  set(TARGET_LIBS ${TARGET_LIBS} ${FREETYPE_LIBRARIES})
  include_directories(${FREETYPE_INCLUDE_DIRS})
  link_directories(${FREETYPE_LIBRARY_DIRS})
else (PKG_CONFIG_FOUND)
  set(FREETYPE_INSTALL_PREFIX "$ENV{FREETYPE_INSTALL_PREFIX}"
    CACHE PATH "search path for freetype install path")

  find_path(FREETYPE_INSTALL_DIR
    include/freetype.h
    PATHS
    ${FREETYPE_INSTALL_PREFIX}
    ${AEYAE_INSTALL_DIR_INCLUDE}
    /usr/include
    /usr/local/include
    /opt/local/include)

  if (FREETYPE_INSTALL_DIR)
    file(TO_NATIVE_PATH "${FREETYPE_INSTALL_DIR}/include"
      FREETYPE_INCLUDE_DIR)
    file(TO_NATIVE_PATH "${FREETYPE_INSTALL_DIR}/lib"
      FREETYPE_LIBS_DIR)
    include_directories(AFTER ${FREETYPE_INCLUDE_DIR})
  endif (FREETYPE_INSTALL_DIR)
endif (PKG_CONFIG_FOUND)

if (NOT FREETYPE_FOUND)
  find_library(FREETYPE_LIBRARY freetype
    PATHS ${FREETYPE_LIBS_DIR} ${AEYAE_INSTALL_DIR_LIB}
    DOC "freetype library")
  if (FREETYPE_LIBRARY)
    set(TARGET_LIBS ${TARGET_LIBS} ${FREETYPE_LIBRARY})
  endif (FREETYPE_LIBRARY)
endif ()
