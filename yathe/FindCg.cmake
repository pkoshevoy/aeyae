# - Try to find Cg
# Once done this will define
#  
#  Cg_FOUND        - system has Cg
#  Cg_INCLUDE_DIR  - the Cg include directory
#  Cg_LIBRARIES    - Link these to use Cg

IF (APPLE)
  FIND_LIBRARY(Cg_LIBRARY Cg DOC "Cg lib for OSX")
  FIND_PATH(Cg_INCLUDE_DIR Cg/cgGL.h DOC "Include for Cg on OSX")

ELSE (APPLE)

  FIND_LIBRARY(Cg_LIBRARY Cg 
    $ENV{CG_LIB_DIR}
    DOC "Cg lib"
    )

  FIND_LIBRARY(CgGL_LIBRARY CgGL
    $ENV{CG_LIB_DIR} 
    DOC "CgGL lib"
    )

  FIND_PATH(Cg_INCLUDE_DIR Cg/cgGL.h 
    $ENV{CG_DIR}/include 
    DOC "Include for Cg"
    )
ENDIF (APPLE)


SET( Cg_FOUND "NO" )
IF(Cg_LIBRARY)

  IF (APPLE)
    SET (Cg_LIBRARIES ${Cg_LIBRARY})
  ELSE (APPLE)
    SET (Cg_LIBRARIES ${Cg_LIBRARY} ${CgGL_LIBRARY})
  ENDIF (APPLE)
  
  IF(Cg_INCLUDE_DIR)
    SET( Cg_FOUND "YES" )
  ELSE(Cg_INCLUDE_DIR)
    SET( Cg_FOUND "NO" )
  ENDIF(Cg_INCLUDE_DIR)
  
ENDIF(Cg_LIBRARY)
