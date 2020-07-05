cmake_minimum_required(VERSION 3.1.0)

include("${aeyae_SOURCE_DIR}/get_pkgconfig.cmake")

if (PKG_CONFIG_FOUND)
  pkg_check_modules(FRIBIDI fribidi)
  set(TARGET_LIBS ${TARGET_LIBS} ${FRIBIDI_LIBRARIES})
  include_directories(${FRIBIDI_INCLUDE_DIRS})
  link_directories(${FRIBIDI_LIBRARY_DIRS})
else (PKG_CONFIG_FOUND)
  set(FRIBIDI_INSTALL_PREFIX "$ENV{FRIBIDI_INSTALL_PREFIX}"
    CACHE PATH "search path for fribidi install path")

  find_path(FRIBIDI_INSTALL_DIR
    include/fribidi.h
    PATHS
    ${FRIBIDI_INSTALL_PREFIX}
    ${AEYAE_INSTALL_DIR_INCLUDE}
    /usr/include
    /usr/local/include
    /opt/local/include)

  if (FRIBIDI_INSTALL_DIR)
    file(TO_NATIVE_PATH "${FRIBIDI_INSTALL_DIR}/include"
      FRIBIDI_INCLUDE_DIR)
    file(TO_NATIVE_PATH "${FRIBIDI_INSTALL_DIR}/lib"
      FRIBIDI_LIBS_DIR)
    include_directories(AFTER ${FRIBIDI_INCLUDE_DIR})
  endif (FRIBIDI_INSTALL_DIR)
endif (PKG_CONFIG_FOUND)

if (NOT FRIBIDI_FOUND)
  find_library(FRIBIDI_LIBRARY fribidi
    PATHS ${FRIBIDI_LIBS_DIR} ${AEYAE_INSTALL_DIR_LIB}
    DOC "fribidi library")
  if (FRIBIDI_LIBRARY)
    set(TARGET_LIBS ${TARGET_LIBS} ${FRIBIDI_LIBRARY})
  endif (FRIBIDI_LIBRARY)
endif ()
