# - Try to find GLEW
# Once done this will define
#  
#  GLEW_FOUND        - system has GLEW
#  GLEW_INCLUDE_DIR  - the GLEW include directory
#  GLEW_LIBRARIES    - Link these to use GLEW

set(GLEW_DIR "$ENV{GLEW_DIR}" CACHE PATH 
  "root path for GLEW lib/ and include/ folders")

find_path(GLEW_INCLUDE_DIR GL/glew.h 
  PATHS ${GLEW_DIR}/include 
  DOC "headers directory for GLEW"
)

if (GLEW_INCLUDE_DIR)
  find_library(GLEW_LIBRARY 
    NAMES GLEW glew32s
    PATHS ${GLEW_INCLUDE_DIR}/../lib ${GLEW_DIR}/lib 
    DOC "GLEW lib"
    )
endif (GLEW_INCLUDE_DIR)

set(GLEW_FOUND "NO")
if(GLEW_LIBRARY AND GLEW_INCLUDE_DIR)
  set(GLEW_LIBRARIES ${GLEW_LIBRARY})
  set(GLEW_FOUND "YES")
endif(GLEW_LIBRARY AND GLEW_INCLUDE_DIR)
