# - Try to find THE libraries
# Once done this will define
#  
#  THE_LIBS_FOUND	- system has THE libraries
#  THE_INCLUDE_DIR	- THE include directory
#  THE_CORE_LIBRARY	- the core library
#  THE_ITK_LIBRARY	- the itk library
#  THE_UI_LIBRARY	- the abstract ui library
#  THE_UI_QT_LIBRARY	- the Qt ui library
#  THE_UI_FLTK_LIBRARY	- the FLTK ui library

FILE(TO_CMAKE_PATH "$ENV{THE_SRC_DIR}" THE_SRC_DIR)
FILE(TO_CMAKE_PATH "$ENV{THE_BIN_DIR}" THE_BIN_DIR)

find_library(THE_CORE_LIBRARY the_core
  ${THE_BIN_DIR} 
  /scratch/$ENV{CPU}/the
  /usr/sci/crcnsdata/$ENV{CPU}/the
  DOC "the_core library"
  )

find_library(THE_ITK_LIBRARY the_itk
  ${THE_BIN_DIR} 
  /scratch/$ENV{CPU}/the
  /usr/sci/crcnsdata/$ENV{CPU}/the
  DOC "the_itk library"
  )

find_library(THE_UI_LIBRARY the_ui
  ${THE_BIN_DIR} 
  /scratch/$ENV{CPU}/the
  /usr/sci/crcnsdata/$ENV{CPU}/the
  DOC "the_ui library"
  )

find_library(THE_UI_QT_LIBRARY the_ui_qt
  ${THE_BIN_DIR} 
  /scratch/$ENV{CPU}/the
  /usr/sci/crcnsdata/$ENV{CPU}/the
  DOC "the_ui_qt library"
  )

find_library(THE_UI_FLTK_LIBRARY the_ui_fltk
  ${THE_BIN_DIR} 
  /scratch/$ENV{CPU}/the
  /usr/sci/crcnsdata/$ENV{CPU}/the
  DOC "the_ui_fltk library"
  )

find_path(THE_INCLUDE_DIR utils/the_utils.hxx 
  ${THE_SRC_DIR} 
  DOC "include directory for the_* libraries"
  )
#MESSAGE("${THE_SRC_DIR} ${THE_CORE_LIBRARY} ${THE_INCLUDE_DIR}")

set(THE_FOUND "NO")
if (THE_CORE_LIBRARY AND THE_INCLUDE_DIR)
  set(THE_FOUND "YES")
endif (THE_CORE_LIBRARY AND THE_INCLUDE_DIR)
