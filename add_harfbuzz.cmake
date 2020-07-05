cmake_minimum_required(VERSION 3.1.0)

include("${aeyae_SOURCE_DIR}/get_pkgconfig.cmake")

if (PKG_CONFIG_FOUND)
  pkg_check_modules(HARFBUZZ harfbuzz)
  set(TARGET_LIBS ${TARGET_LIBS} ${HARFBUZZ_LIBRARIES})
  include_directories(${HARFBUZZ_INCLUDE_DIRS})
  link_directories(${HARFBUZZ_LIBRARY_DIRS})
else (PKG_CONFIG_FOUND)
  set(HARFBUZZ_INSTALL_PREFIX "$ENV{HARFBUZZ_INSTALL_PREFIX}"
    CACHE PATH "search path for harfbuzz install path")

  find_path(HARFBUZZ_INSTALL_DIR
    include/harfbuzz.h
    PATHS
    ${HARFBUZZ_INSTALL_PREFIX}
    ${AEYAE_INSTALL_DIR_INCLUDE}
    /usr/include
    /usr/local/include
    /opt/local/include)

  if (HARFBUZZ_INSTALL_DIR)
    file(TO_NATIVE_PATH "${HARFBUZZ_INSTALL_DIR}/include"
      HARFBUZZ_INCLUDE_DIR)
    file(TO_NATIVE_PATH "${HARFBUZZ_INSTALL_DIR}/lib"
      HARFBUZZ_LIBS_DIR)
    include_directories(AFTER ${HARFBUZZ_INCLUDE_DIR})
  endif (HARFBUZZ_INSTALL_DIR)
endif (PKG_CONFIG_FOUND)

if (NOT HARFBUZZ_FOUND)
  find_library(HARFBUZZ_LIBRARY harfbuzz
    PATHS ${HARFBUZZ_LIBS_DIR} ${AEYAE_INSTALL_DIR_LIB}
    DOC "harfbuzz library")
  if (HARFBUZZ_LIBRARY)
    set(TARGET_LIBS ${TARGET_LIBS} ${HARFBUZZ_LIBRARY})
  endif (HARFBUZZ_LIBRARY)
endif ()
