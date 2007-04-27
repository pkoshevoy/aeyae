#!/bin/bash

if [ -z "${CPU}" ]; then
    export CPU=`uname -p`;
fi

if [ ${CPU} = "unknown" ]; then
    OS=`uname -o`
    if [ ${OS} = "Cygwin" ]; then
       export CPU=w32
    fi
fi

# use the intel compiler, when available:
if [ ${CPU} = "ia64" -a -e /opt/intel/cc/9.0/bin/iccvars.sh ]; then
    export CC=icc
    export CXX=icpc
    . /opt/intel/cc/9.0/bin/iccvars.sh
fi

# add ITK to the search paths:
if [ -n "${ITK_SOURCE_DIR}" ]; then
    export CPATH=${ITK_SOURCE_DIR}:${CPATH}
fi

if [ -n "${ITK_BINARY_DIR}" ]; then
    export ITK_DIR=${ITK_BINARY_DIR}
    export LIBRARY_PATH=${ITK_BINARY_DIR}/bin:${LIBRARY_PATH}
    export LD_LIBRARY_PATH=${ITK_BINARY_DIR}/bin:${LD_LIBRARY_PATH}
fi

# add Qt to the search paths:
if [ -n "${QT4_DIR}" ]; then
    export QTDIR=${QT4_DIR}
    export PATH=${QT4_DIR}/bin:${PATH}
    export CPATH=${QT4_DIR}/include:${CPATH}
    export MANPATH=${QT4_DIR}/share/man:${MANPATH}
    export LIBRARY_PATH=${QT4_DIR}/lib:${LIBRARY_PATH}
    export LD_LIBRARY_PATH=${QT4_DIR}/lib:${LD_LIBRARY_PATH}
fi

# add GLEW to the search paths:
if [ -n "${GLEW_DIR}" ]; then
    export PATH=${GLEW_DIR}/bin:${PATH}
    export CPATH=${GLEW_DIR}/include:${CPATH}
    export MANPATH=${GLEW_DIR}/share/man:${MANPATH}
    export LIBRARY_PATH=${GLEW_DIR}/lib:${LIBRARY_PATH}
    export LD_LIBRARY_PATH=${GLEW_DIR}/lib:${LD_LIBRARY_PATH}
fi

# add Cg to the search paths:
if [ -n "${CG_DIR}" ]; then
    export PATH=${CG_DIR}/bin:${PATH}
    export CPATH=${CG_DIR}/include:${CPATH}
    export MANPATH=${CG_DIR}/share/man:${MANPATH}
    if [ -e ${CG_DIR}/lib64 ]; then
	export CG_LIB_DIR=${CG_DIR}/lib64
    else
	export CG_LIB_DIR=${CG_DIR}/lib
    fi
    export LIBRARY_PATH=${CG_LIB_DIR}:${LIBRARY_PATH}
    export LD_LIBRARY_PATH=${CG_LIB_DIR}:${LD_LIBRARY_PATH}
fi

# add FFTW to the search paths:
if [ -n "${FFTW_DIR}" ]; then
    export PATH=${FFTW_DIR}/bin:${PATH}
    export CPATH=${FFTW_DIR}/include:${CPATH}
    export MANPATH=${FFTW_DIR}/share/man:${MANPATH}
    export LIBRARY_PATH=${FFTW_DIR}/lib:${LIBRARY_PATH}
    export LD_LIBRARY_PATH=${FFTW_DIR}/lib:${LD_LIBRARY_PATH}
fi

# add BOOST to the search paths:
if [ -n "${BOOST_ROOT}" ]; then
    export CPATH=${BOOST_ROOT}/include/boost-1_33_1:${CPATH}
    export LIBRARY_PATH=${BOOST_ROOT}/lib:${LIBRARY_PATH}
    export LD_LIBRARY_PATH=${BOOST_ROOT}/lib:${LD_LIBRARY_PATH}
fi

# add THE libraries to the search paths:
if [ -n "${THE_SRC_DIR}" ]; then
    export CPATH=${THE_SRC_DIR}:${CPATH}
fi

if [ -n "${THE_BIN_DIR}" ]; then
    export LIBRARY_PATH=${THE_BIN_DIR}:${LIBRARY_PATH}
    export LD_LIBRARY_PATH=${THE_BIN_DIR}:${LD_LIBRARY_PATH}
fi
