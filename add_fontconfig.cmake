cmake_minimum_required(VERSION 3.1.0)

include("${aeyae_SOURCE_DIR}/get_pkgconfig.cmake")

if (PKG_CONFIG_FOUND)
  pkg_check_modules(FONTCONFIG fontconfig)
  set(TARGET_LIBS ${TARGET_LIBS} ${FONTCONFIG_LIBRARIES})
  include_directories(${FONTCONFIG_INCLUDE_DIRS})
  link_directories(${FONTCONFIG_LIBRARY_DIRS})
else (PKG_CONFIG_FOUND)
  set(FONTCONFIG_INSTALL_PREFIX "$ENV{FONTCONFIG_INSTALL_PREFIX}"
    CACHE PATH "search path for fontconfig install path")

  find_path(FONTCONFIG_INSTALL_DIR
    include/fontconfig.h
    PATHS
    ${FONTCONFIG_INSTALL_PREFIX}
    ${AEYAE_INSTALL_DIR_INCLUDE}
    /usr/include
    /usr/local/include
    /opt/local/include)

  if (FONTCONFIG_INSTALL_DIR)
    file(TO_NATIVE_PATH "${FONTCONFIG_INSTALL_DIR}/include"
      FONTCONFIG_INCLUDE_DIR)
    file(TO_NATIVE_PATH "${FONTCONFIG_INSTALL_DIR}/lib"
      FONTCONFIG_LIBS_DIR)
    include_directories(AFTER ${FONTCONFIG_INCLUDE_DIR})
  endif (FONTCONFIG_INSTALL_DIR)
endif (PKG_CONFIG_FOUND)

if (NOT FONTCONFIG_FOUND)
  find_library(FONTCONFIG_LIBRARY fontconfig
    PATHS ${FONTCONFIG_LIBS_DIR} ${AEYAE_INSTALL_DIR_LIB}
    DOC "fontconfig library")
  if (FONTCONFIG_LIBRARY)
    set(TARGET_LIBS ${TARGET_LIBS} ${FONTCONFIG_LIBRARY})
  endif (FONTCONFIG_LIBRARY)
endif ()
