# - Try to find FFTW
# Once done this will define
#  
#  FFTW_FOUND        - system has FFTW
#  FFTW_INCLUDE_DIR  - the FFTW include directory
#  FFTW_LIBRARIES    - Link these to use FFTW

FIND_LIBRARY(FFTW_LIBRARY fftw3
  $ENV{FFTW_DIR}/lib 
  DOC "FFTW lib"
)

FIND_PATH(FFTW_INCLUDE_DIR fftw3.h 
  $ENV{FFTW_DIR}/include 
  DOC "Include for FFTW"
)

SET( FFTW_FOUND "NO" )
IF(FFTW_LIBRARY)
  
  SET( FFTW_LIBRARIES  ${FFTW_LIBRARY} ${FFTW_LIBRARIES})
  
  IF(FFTW_INCLUDE_DIR)
    SET( FFTW_FOUND "YES" )
  ELSE(FFTW_INCLUDE_DIR)
    SET( FFTW_FOUND "NO" )
  ENDIF(FFTW_INCLUDE_DIR)
  
ENDIF(FFTW_LIBRARY)
