cmake_minimum_required(VERSION 3.1.0)

set(LIBBFD_INSTALL_PREFIX "$ENV{LIBBFD_INSTALL_PREFIX}"
  CACHE PATH "search path for libbfd install path")

find_path(LIBBFD_INSTALL_DIR
  include/bfd.h
  PATHS
  ${LIBBFD_INSTALL_PREFIX}
  ${AEYAE_INSTALL_DIR_INCLUDE}
  /usr/include
  /usr/local/include
  /opt/local/include)

if (LIBBFD_INSTALL_DIR)
  file(TO_NATIVE_PATH "${LIBBFD_INSTALL_DIR}/include"
    LIBBFD_INCLUDE_DIR)
  file(TO_NATIVE_PATH "${LIBBFD_INSTALL_DIR}/lib"
    LIBBFD_LIBS_DIR)
  include_directories(AFTER ${LIBBFD_INCLUDE_DIR})
endif (LIBBFD_INSTALL_DIR)

find_library(LIBBFD_LIBRARY bfd
  PATHS /usr/lib /usr/lib64 ${LIBBFD_LIBS_DIR} ${AEYAE_INSTALL_DIR_LIB}
  DOC "libbfd")
if (LIBBFD_LIBRARY)
  set(TARGET_LIBS ${TARGET_LIBS} ${LIBBFD_LIBRARY})
endif (LIBBFD_LIBRARY)

find_library(LIBIBERTY_LIBRARY iberty
  PATHS /usr/lib /usr/lib64 ${LIBBFD_LIBS_DIR} ${AEYAE_INSTALL_DIR_LIB}
  DOC "libiberty")
if (LIBIBERTY_LIBRARY)
  set(TARGET_LIBS ${TARGET_LIBS} ${LIBIBERTY_LIBRARY})
endif (LIBIBERTY_LIBRARY)
