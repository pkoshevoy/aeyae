cmake_minimum_required(VERSION 3.1.0)

include("${aeyae_SOURCE_DIR}/get_pkgconfig.cmake")

if (PKG_CONFIG_FOUND)
  pkg_check_modules(PORTAUDIO portaudio-2.0)
  set(TARGET_LIBS ${TARGET_LIBS} ${PORTAUDIO_LIBRARIES})
  include_directories(${PORTAUDIO_INCLUDE_DIRS})
  link_directories(${PORTAUDIO_LIBRARY_DIRS})
else (PKG_CONFIG_FOUND)
  set(PORTAUDIO_INSTALL_PREFIX "$ENV{PORTAUDIO_INSTALL_PREFIX}"
    CACHE PATH "search path for portaudio install path")

  find_path(PORTAUDIO_INSTALL_DIR
    include/portaudio.h
    PATHS
    ${PORTAUDIO_INSTALL_PREFIX}
    ${AEYAE_INSTALL_DIR_INCLUDE}
    /usr/include
    /usr/local/include
    /opt/local/include)

  if (PORTAUDIO_INSTALL_DIR)
    file(TO_NATIVE_PATH "${PORTAUDIO_INSTALL_DIR}/include"
      PORTAUDIO_INCLUDE_DIR)
    file(TO_NATIVE_PATH "${PORTAUDIO_INSTALL_DIR}/lib"
      PORTAUDIO_LIBS_DIR)
    include_directories(AFTER ${PORTAUDIO_INCLUDE_DIR})
  endif (PORTAUDIO_INSTALL_DIR)
endif (PKG_CONFIG_FOUND)

if (NOT PORTAUDIO_FOUND)
  find_library(PORTAUDIO_LIBRARY portaudio
    PATHS ${PORTAUDIO_LIBS_DIR} ${AEYAE_INSTALL_DIR_LIB}
    DOC "portaudio library")
  if (PORTAUDIO_LIBRARY)
    set(TARGET_LIBS ${TARGET_LIBS} ${PORTAUDIO_LIBRARY})
  endif (PORTAUDIO_LIBRARY)
endif ()
