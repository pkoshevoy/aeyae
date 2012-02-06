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
#if [ -e /scratch/ITK ]; then
#    export ITK_SOURCE_DIR=/scratch/ITK
#elif [ -e /usr/sci/crcnsdata/ITK ]; then
#    export ITK_SOURCE_DIR=/usr/sci/crcnsdata/ITK
#fi
#
#if [ -e /scratch/Developer/"${CPU}"/ITK ]; then
#    export ITK_BINARY_DIR=/scratch/Developer/"${CPU}"/ITK
#elif [ -e /usr/sci/crcnsdata/"${CPU}"/ITK ]; then
#    export ITK_BINARY_DIR=/usr/sci/crcnsdata/"${CPU}"/ITK
#fi

# determine where Qt3 lives:
if [ -e /usr/lib64/qt3 ]; then
    export QT3_DIR=/usr/lib64/qt3
elif [ -e /usr/lib/qt3 ]; then
    export QT3_DIR=/usr/lib/qt3
fi

# determine where Qt4 lives:
if [ -e /Developer/"${CPU}"/Qt ]; then
    export QT4_DIR=/Developer/"${CPU}"/Qt
elif [ -e /scratch/Developer/"${CPU}"/Qt ]; then
    export QT4_DIR=/scratch/Developer/"${CPU}"/Qt
elif [ -e /usr/sci/crcnsdata/"${CPU}"/Qt ]; then
    export QT4_DIR=/usr/sci/crcnsdata/"${CPU}"/Qt
elif [ -e /usr/include/QtCore ]; then
    export QT4_DIR=/usr
elif [ -e /usr/lib64/libQtCore.so ]; then
    export QT4_DIR=/usr
elif [ -e /usr/lib/libQtCore.so ]; then
    export QT4_DIR=/usr
fi

# determine where GLEW lives:
if [ -e /Developer/"${CPU}"/GLEW ]; then
    export GLEW_DIR=/Developer/"${CPU}"/GLEW
elif [ -e /scratch/Developer/"${CPU}"/GLEW ]; then
    export GLEW_DIR=/scratch/Developer/"${CPU}"/GLEW
elif [ -e /usr/sci/crcnsdata/"${CPU}"/GLEW ]; then
    export GLEW_DIR=/usr/sci/crcnsdata/"${CPU}"/GLEW
elif [ -e /usr/include/GL/glew.h ]; then
    export GLEW_DIR=/usr
elif [ -e /usr/lib64/libGLEW.so ]; then
    export GLEW_DIR=/usr
elif [ -e /usr/lib/libGLEW.so ]; then
    export GLEW_DIR=/usr
fi

# determine where Cg lives:
if [ -e /Developer/"${CPU}"/Cg ]; then
    export CG_DIR=/Developer/"${CPU}"/Cg
elif [ -e /scratch/Developer/"${CPU}"/Cg ]; then
    export CG_DIR=/scratch/Developer/"${CPU}"/Cg
elif [ -e /usr/sci/crcnsdata/"${CPU}"/Cg ]; then
    export CG_DIR=/usr/sci/crcnsdata/"${CPU}"/Cg
elif [ -e /usr/include/Cg/cgGL.h ]; then
    export CG_DIR=/usr
elif [ -e /usr/lib64/libCg.so ]; then
    export CG_DIR=/usr
elif [ -e /usr/lib/libCg.so ]; then
    export CG_DIR=/usr
fi

# determine where FFTW lives:
if [ -e /Developer/"${CPU}"/FFTW ]; then
    export FFTW_DIR=/Developer/"${CPU}"/FFTW
elif [ -e /scratch/Developer/"${CPU}"/FFTW ]; then
    export FFTW_DIR=/scratch/Developer/"${CPU}"/FFTW
elif [ -e /usr/sci/crcnsdata/"${CPU}"/FFTW ]; then
    export FFTW_DIR=/usr/sci/crcnsdata/"${CPU}"/FFTW
elif [ -e /usr/include/fftw3.h ]; then
    export FFTW_DIR=/usr
elif [ -e /usr/lib64/libfftw3.so ]; then
    export FFTW_DIR=/usr
elif [ -e /usr/lib/libfftw3.so ]; then
    export FFTW_DIR=/usr
fi

# determine where BOOST lives:
if [ -e /Developer/"${CPU}"/include/boost ]; then
    export BOOST_ROOT=/Developer/"${CPU}"
elif [ -e /scratch/Developer/"${CPU}"/include/boost ]; then
    export BOOST_ROOT=/scratch/Developer/"${CPU}"
elif [ -e /usr/include/boost ]; then
    export BOOST_ROOT=/usr
elif [ -e /usr/include/boost ]; then
    export BOOST_ROOT=/usr/include
fi

if [ -n "${BOOST_ROOT}" ]; then
    export BOOST_LIBRARYDIR=${BOOST_ROOT}/lib
    export BOOST_INCLUDEDIR=${BOOST_ROOT}/include
fi

# determine where FLTK lives:
if [ -e /usr/X11R6/include/FL ]; then
    export FLTK_DIR=/usr/X11R6
fi

# determine where THE libraries live:
if [ -e "${HOME}"/src/the ]; then
    export THE_SRC_DIR="${HOME}"/src/the
fi

if [ -e "${HOME}/${CPU}/build/the" ]; then
    export THE_BIN_DIR="${HOME}/${CPU}"/build/the
elif [ -e /Developer/"${CPU}"/build/the ]; then
    export THE_BIN_DIR=/Developer/"${CPU}"/build/the
elif [ -e /scratch/Developer/"${CPU}"/the ]; then
    export THE_BIN_DIR=/scratch/Developer/"${CPU}"/the
elif [ -e /usr/sci/crcnsdata/"${CPU}"/the ]; then
    export THE_BIN_DIR=/usr/sci/crcnsdata/"${CPU}"/the
fi

# use local build of ffmpeg if it is available:
if [ -e "/Developer/${CPU}/include/libavcodec/avcodec.h" ]; then
    export FFMPEG_HEADERS_PATH="/Developer/${CPU}/include"
    export FFMPEG_LIBS_PATH="/Developer/${CPU}/lib"
elif [ -e "/scratch/Developer/${CPU}/include/libavcodec/avcodec.h" ]; then
    export FFMPEG_HEADERS_PATH="/scratch/Developer/${CPU}/include"
    export FFMPEG_LIBS_PATH="/scratch/Developer/${CPU}/lib"
fi
