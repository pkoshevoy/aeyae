# - Try to find GLEW
# Once done this will define
#  
#  GLEW_FOUND        - system has GLEW
#  GLEW_INCLUDE_DIR  - the GLEW include directory
#  GLEW_LIBRARIES    - Link these to use GLEW

FIND_LIBRARY(GLEW_LIBRARY GLEW 
  $ENV{GLEW_DIR}/lib 
  DOC "GLEW lib"
)

FIND_PATH(GLEW_INCLUDE_DIR GL/glew.h 
  $ENV{GLEW_DIR}/include 
  DOC "Include for GLEW"
)

SET( GLEW_FOUND "NO" )
IF(GLEW_LIBRARY)
  
  SET( GLEW_LIBRARIES  ${GLEW_LIBRARY} ${GLEW_LIBRARIES})
  
  IF(GLEW_INCLUDE_DIR)
    SET( GLEW_FOUND "YES" )
  ELSE(GLEW_INCLUDE_DIR)
    SET( GLEW_FOUND "NO" )
  ENDIF(GLEW_INCLUDE_DIR)
  
ENDIF(GLEW_LIBRARY)
