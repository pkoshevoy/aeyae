# - Try to find YATHE libraries
# Once done this will define
#  
#  YATHE_LIBS_FOUND      - system has YATHE libraries
#  YATHE_INCLUDE_DIR     - YATHE headers directory
#  YATHE_CORE_LIBRARY    - the core library
#  YATHE_UI_LIBRARY      - the abstract ui library
#  YATHE_UI_QT3_LIBRARY  - the Qt3 ui library
#  YATHE_UI_QT4_LIBRARY  - the Qt4 ui library
#  YATHE_UI_FLTK_LIBRARY - the FLTK ui library

set(YATHE_DIR "$ENV{YATHE_DIR}" CACHE PATH 
  "root path for YATHE lib/ and include/ folders")

find_path(YATHE_INCLUDE_DIR utils/the_utils.hxx
  PATHS ${YATHE_DIR}/include
  DOC "headers directory for the_* libraries"
  )

if (YATHE_INCLUDE_DIR)
  find_library(YATHE_CORE_LIBRARY
    NAMES the_core 
    PATHS ${YATHE_INCLUDE_DIR}/../lib ${YATHE_DIR}/lib
    DOC "the_core library"
    )
  
  find_library(YATHE_UI_LIBRARY 
    NAMES the_ui
    PATHS ${YATHE_INCLUDE_DIR}/../lib ${YATHE_DIR}/lib
    DOC "the_ui library"
    )
  
  find_library(YATHE_UI_QT4_LIBRARY
    NAMES the_ui_qt4
    PATHS ${YATHE_INCLUDE_DIR}/../lib ${YATHE_DIR}/lib
    DOC "the_ui_qt4 library"
    )
  
  find_library(YATHE_UI_QT3_LIBRARY 
    NAMES the_ui_qt3
    PATHS ${YATHE_INCLUDE_DIR}/../lib ${YATHE_DIR}/lib
    DOC "the_ui_qt3 library"
    )
  
  find_library(YATHE_UI_FLTK_LIBRARY 
    NAMES the_ui_fltk
    PATHS ${YATHE_INCLUDE_DIR}/../lib ${YATHE_DIR}/lib
    DOC "the_ui_fltk library"
    )
endif (YATHE_INCLUDE_DIR)

set(YATHE_FOUND "NO")
if (YATHE_CORE_LIBRARY AND YATHE_INCLUDE_DIR)
  set(YATHE_FOUND "YES")
endif (YATHE_CORE_LIBRARY AND YATHE_INCLUDE_DIR)
