cmake_minimum_required(VERSION 3.1.0)

set(LIBBFD_INSTALL_PREFIX "$ENV{LIBBFD_INSTALL_PREFIX}"
  CACHE PATH "search path for libbfd install path")

find_path(LIBBFD_INSTALL_DIR
  include/bfd.h
  include/demangle.h
  PATHS
  ${LIBBFD_INSTALL_PREFIX}
  ${AEYAE_INSTALL_DIR_INCLUDE}
  /usr/include
  /usr/local/include
  /opt/local/include)

find_library(LIBBFD_LIBRARY bfd
  PATHS /usr/lib /usr/lib64 ${LIBBFD_LIBS_DIR} ${AEYAE_INSTALL_DIR_LIB}
  DOC "libbfd")

find_library(LIBSFRAME_LIBRARY sframe
  PATHS /usr/lib /usr/lib64 ${LIBBFD_LIBS_DIR} ${AEYAE_INSTALL_DIR_LIB}
  DOC "libsframe")

find_library(LIBIBERTY_LIBRARY iberty
  PATHS /usr/lib /usr/lib64 ${LIBBFD_LIBS_DIR} ${AEYAE_INSTALL_DIR_LIB}
  DOC "libiberty")

if (LIBBFD_INSTALL_DIR AND
    LIBBFD_LIBRARY AND
    LIBSFRAME_LIBRARY AND
    LIBIBERTY_LIBRARY)
  add_definitions(-DYAE_HAS_LIBBFD)
  file(TO_NATIVE_PATH "${LIBBFD_INSTALL_DIR}/include"
    LIBBFD_INCLUDE_DIR)
  file(TO_NATIVE_PATH "${LIBBFD_INSTALL_DIR}/lib"
    LIBBFD_LIBS_DIR)
  include_directories(AFTER ${LIBBFD_INCLUDE_DIR})
  set(TARGET_LIBS ${TARGET_LIBS} ${LIBBFD_LIBRARY})
  set(TARGET_LIBS ${TARGET_LIBS} ${LIBSFRAME_LIBRARY})
  set(TARGET_LIBS ${TARGET_LIBS} ${LIBIBERTY_LIBRARY})
endif ()

find_package(Backtrace)
if (Backtrace_FOUND)
  add_definitions(-DYAE_BACKTRACE_HEADER="${Backtrace_HEADER}")
endif ()
