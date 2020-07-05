cmake_minimum_required(VERSION 3.1.0)

include("${aeyae_SOURCE_DIR}/get_pkgconfig.cmake")

if (PKG_CONFIG_FOUND)
  pkg_check_modules(LIBASS libass)
  set(TARGET_LIBS ${TARGET_LIBS} ${LIBASS_LIBRARIES})
  include_directories(${LIBASS_INCLUDE_DIRS})
  link_directories(${LIBASS_LIBRARY_DIRS})
else (PKG_CONFIG_FOUND)
  set(LIBASS_INSTALL_PREFIX "$ENV{LIBASS_INSTALL_PREFIX}"
    CACHE PATH "search path for libass install path")

  find_path(LIBASS_INSTALL_DIR
    include/ass/ass.h
    PATHS
    ${LIBASS_INSTALL_PREFIX}
    ${AEYAE_INSTALL_DIR_INCLUDE}
    /usr/include
    /usr/local/include
    /opt/local/include)

  if (LIBASS_INSTALL_DIR)
    file(TO_NATIVE_PATH "${LIBASS_INSTALL_DIR}/include"
      LIBASS_INCLUDE_DIR)
    file(TO_NATIVE_PATH "${LIBASS_INSTALL_DIR}/lib"
      LIBASS_LIBS_DIR)
    include_directories(AFTER ${LIBASS_INCLUDE_DIR})
  endif (LIBASS_INSTALL_DIR)
endif (PKG_CONFIG_FOUND)

if (NOT LIBASS_FOUND)
  find_library(LIBASS_LIBRARY ass libass
    PATHS ${LIBASS_LIBS_DIR} ${AEYAE_INSTALL_DIR_LIB}
    DOC "libass (for ssa/ass subtitle renderering)")
  if (LIBASS_LIBRARY)
    set(TARGET_LIBS ${TARGET_LIBS} ${LIBASS_LIBRARY})
  endif (LIBASS_LIBRARY)
endif ()
