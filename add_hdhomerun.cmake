cmake_minimum_required(VERSION 3.1.0)

include("${aeyae_SOURCE_DIR}/get_pkgconfig.cmake")

if (PKG_CONFIG_FOUND)
  pkg_check_modules(HDHOMERUN hdhomerun)
  set(TARGET_LIBS ${TARGET_LIBS} ${HDHOMERUN_LIBRARIES})
  include_directories(${HDHOMERUN_INCLUDE_DIRS})
  link_directories(${HDHOMERUN_LIBRARY_DIRS})
endif ()

if (NOT HDHOMERUN_FOUND)
  set(HDHOMERUN_INSTALL_PREFIX "$ENV{HDHOMERUN_INSTALL_PREFIX}"
    CACHE PATH "search path for hdhomerun install path")

  find_path(HDHOMERUN_INSTALL_DIR
    include/hdhomerun_device.h
    PATHS
    ${HDHOMERUN_INSTALL_PREFIX}
    ${CMAKE_INSTALL_PREFIX}
    /usr
    /usr/local
    /opt/local)

  if (HDHOMERUN_INSTALL_DIR)
    file(TO_NATIVE_PATH "${HDHOMERUN_INSTALL_DIR}/include"
      HDHOMERUN_INCLUDE_DIR)
    file(TO_NATIVE_PATH "${HDHOMERUN_INSTALL_DIR}/lib"
      HDHOMERUN_LIBS_DIR)
    include_directories(AFTER ${HDHOMERUN_INCLUDE_DIR})
  endif (HDHOMERUN_INSTALL_DIR)

  find_library(HDHOMERUN_LIBRARY hdhomerun
    PATHS ${HDHOMERUN_LIBS_DIR} ${AEYAE_INSTALL_DIR_LIB}
    DOC "hdhomerun library")
  if (HDHOMERUN_LIBRARY)
    set(TARGET_LIBS ${TARGET_LIBS} ${HDHOMERUN_LIBRARY})
    if (WIN32)
      set(TARGET_LIBS ${TARGET_LIBS} iphlpapi ws2_32)
    endif (WIN32)
  endif (HDHOMERUN_LIBRARY)
endif ()
