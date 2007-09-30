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

# determine where ITK lives:
if [ -e /scratch/ITK ]; then
    export ITK_SOURCE_DIR=/scratch/ITK
elif [ -e /usr/sci/crcnsdata/ITK ]; then
    export ITK_SOURCE_DIR=/usr/sci/crcnsdata/ITK
fi

if [ -e /scratch/"${CPU}"/ITK ]; then
    export ITK_BINARY_DIR=/scratch/"${CPU}"/ITK
elif [ -e /usr/sci/crcnsdata/"${CPU}"/ITK ]; then
    export ITK_BINARY_DIR=/usr/sci/crcnsdata/"${CPU}"/ITK
fi

# determine where Qt3 lives:
if [ -e /usr/lib/qt3 ]; then
    export QT3_DIR=/usr/lib/qt3
fi

# determine where Qt4 lives:
if [ -e /scratch/"${CPU}"/Qt ]; then
    export QT4_DIR=/scratch/"${CPU}"/Qt
elif [ -e /usr/sci/crcnsdata/"${CPU}"/Qt ]; then
    export QT4_DIR=/usr/sci/crcnsdata/"${CPU}"/Qt
elif [ -e /usr/lib/libQtCore.so ]; then
    export QT4_DIR=/usr
fi

# determine where GLEW lives:
if [ -e /scratch/"${CPU}"/GLEW ]; then
    export GLEW_DIR=/scratch/"${CPU}"/GLEW
elif [ -e /usr/sci/crcnsdata/"${CPU}"/GLEW ]; then
    export GLEW_DIR=/usr/sci/crcnsdata/"${CPU}"/GLEW
elif [ -e /usr/lib/libGLEW.so ]; then
    export GLEW_DIR=/usr
fi

# determine where Cg lives:
if [ -e /scratch/"${CPU}"/Cg ]; then
    export CG_DIR=/scratch/"${CPU}"/Cg
elif [ -e /usr/sci/crcnsdata/"${CPU}"/Cg ]; then
    export CG_DIR=/usr/sci/crcnsdata/"${CPU}"/Cg
elif [ -e /usr/lib/libCg.so ]; then
    export CG_DIR=/usr
fi

# determine where FFTW lives:
if [ -e /scratch/"${CPU}"/FFTW ]; then
    export FFTW_DIR=/scratch/"${CPU}"/FFTW
elif [ -e /usr/sci/crcnsdata/"${CPU}"/FFTW ]; then
    export FFTW_DIR=/usr/sci/crcnsdata/"${CPU}"/FFTW
elif [ -e /usr/lib/libfftw3.so ]; then
    export FFTW_DIR=/usr
fi

# determine where BOOST lives:
if [ -e /scratch/"${CPU}"/BOOST/include/boost-1_33_1 ]; then
    export BOOST_ROOT=/scratch/"${CPU}"/BOOST
elif [ -e /usr/sci/crcnsdata/"${CPU}"/BOOST/include/boost-1_33_1 ]; then
    export BOOST_ROOT=/usr/sci/crcnsdata/"${CPU}"/BOOST
elif [ -e /usr/include/boost ]; then
    export BOOST_ROOT=/usr/include
fi

# determine where FLTK lives:
if [ -e /usr/X11R6/include/FL ]; then
    export FLTK_DIR=/usr/X11R6
fi

# determine where THE libraries live:
if [ -e "${HOME}"/src/the ]; then
    export THE_SRC_DIR="${HOME}"/src/the
fi

if [ -e /scratch/"${CPU}"/the ]; then
    export THE_BIN_DIR=/scratch/"${CPU}"/the
elif [ -e /usr/sci/crcnsdata/"${CPU}"/the ]; then
    export THE_BIN_DIR=/usr/sci/crcnsdata/"${CPU}"/the
elif [ -e "${HOME}/${CPU}/build/the" ]; then
    export THE_BIN_DIR="${HOME}/${CPU}"/build/the
fi
