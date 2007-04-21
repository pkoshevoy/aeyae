#!/bin/bash

if [ -z "${CPU}" ]; then
    CPU=`uname -p`;
fi

# if possible, remove the old Qt out of the paths:
PATH_DEL=`which path_del`
if [ -e "${PATH_DEL}" ]; then
    if [ -n "${QTDIR}" ]; then
	PATH=`path_del ${PATH} ${QTDIR}/bin`
	LD_LIBRARY_PATH=`path_del ${LD_LIBRARY_PATH} ${QTDIR}/lib`
	CPLUS_INCLUDE_PATH=`path_del ${CPLUS_INCLUDE_PATH} ${QTDIR}/include`
	MANPATH=`path_del ${MANPATH} ${QTDIR}/doc/man`
    fi
else
    unset PATH_DEL
fi

# determine the new Qt:
if [ -e /scratch/"${CPU}"/Qt ]; then
    export QTDIR=/scratch/"${CPU}"/Qt
elif [ -e /usr/sci/crcnsdata/"${CPU}"/Qt ]; then
    export QTDIR=/usr/sci/crcnsdata/"${CPU}"/Qt
elif [ -e /usr/sci/crcnsdata/"${CPU}"/qt-4.2.0 ]; then
    export QTDIR=/usr/sci/crcnsdata/"${CPU}"/qt-4.2.0
elif [ -e /usr/sci/crcnsdata/"${CPU}"/qt-x11-opensource-4.1.4 ]; then
    export QTDIR=/usr/sci/crcnsdata/"${CPU}"/qt-x11-opensource-4.1.4
elif [ -e /usr/sci/crcnsdata/"${CPU}"/qt-x11-opensource-4.1.4-static-release ]; then
    export QTDIR=/usr/sci/crcnsdata/"${CPU}"/qt-x11-opensource-4.1.4-static-release
elif [ -e /VCR/uofu/"${CPU}"/Qt ]; then
    export QTDIR=/VCR/uofu/"${CPU}"/Qt
elif [ -e /VCR/uofu/qt-x11-opensource-4.1.4-static-release ]; then
    export QTDIR=/VCR/uofu/qt-x11-opensource-4.1.4-static-release
elif [ -e /VCR/uofu/qt-x11-opensource-4.1.4 ]; then
    export QTDIR=/VCR/uofu/qt-x11-opensource-4.1.4
elif [ -e /usr/local/unsafe/paul/qt-x11-opensource-4.1.2 ]; then 
    export QTDIR=/usr/local/unsafe/paul/qt-x11-opensource-4.1.2
fi

# add Qt to the paths:
PATH=${QTDIR}/bin:${PATH}
LD_LIBRARY_PATH=${QTDIR}/lib:${LD_LIBRARY_PATH}
CPLUS_INCLUDE_PATH=${QTDIR}/include:${CPLUS_INCLUDE_PATH}:/usr/sci/crcnsdata/boost_1_33_1
LIBRARY_PATH=${QTDIR}/lib:${LIBRARY_PATH}
MANPATH=${QTDIR}/doc/man:${MANPATH}
export PATH LD_LIBRARY_PATH CPLUS_INCLUDE_PATH LIBRARY_PATH MANPATH

# determine where GLEW lives:
if [ -e /scratch/"${CPU}"/GLEW ]; then
    export GLEW_DIR=/scratch/"${CPU}"/GLEW
elif [ -e /usr/sci/crcnsdata/"${CPU}"/GLEW ]; then
    export GLEW_DIR=/usr/sci/crcnsdata/"${CPU}"/GLEW
fi

# determine where Cg lives:
if [ -e /scratch/"${CPU}"/Cg ]; then
    export CG_DIR=/scratch/"${CPU}"/Cg
elif [ -e /usr/sci/crcnsdata/"${CPU}"/Cg ]; then
    export CG_DIR=/usr/sci/crcnsdata/"${CPU}"/Cg
fi

# add Cg to the search paths:
if [ -n "${CG_DIR}" ]; then
    PATH=${CG_DIR}/bin:${PATH}
    CPATH=${CG_DIR}/include:${CPATH}
    MANPATH=${CG_DIR}/share/man:${MANPATH}
    LIBRARY_PATH=${CG_DIR}/lib:${LIBRARY_PATH}
    LD_LIBRARY_PATH=${CG_DIR}/lib:${LD_LIBRARY_PATH}
    export PATH CPATH MANPATH LIBRARY_PATH LD_LIBRARY_PATH
fi

# determine where FFTW lives:
if [ -e /scratch/"${CPU}"/FFTW ]; then
    export FFTW_DIR=/scratch/"${CPU}"/FFTW
elif [ -e /usr/sci/crcnsdata/"${CPU}"/FFTW ]; then
    export FFTW_DIR=/usr/sci/crcnsdata/"${CPU}"/FFTW
fi

# add FFTW to the search paths:
if [ -n "${FFTW_DIR}" ]; then
    PATH=${FFTW_DIR}/bin:${PATH}
    CPATH=${FFTW_DIR}/include:${CPATH}
    MANPATH=${FFTW_DIR}/share/man:${MANPATH}
    LIBRARY_PATH=${FFTW_DIR}/lib:${LIBRARY_PATH}
    LD_LIBRARY_PATH=${FFTW_DIR}/lib:${LD_LIBRARY_PATH}
    export PATH CPATH MANPATH LIBRARY_PATH LD_LIBRARY_PATH
fi

# determine where BOOST lives:
if [ -e /scratch/"${CPU}"/BOOST ]; then
    export BOOST_DIR=/scratch/"${CPU}"/BOOST
elif [ -e /usr/sci/crcnsdata/"${CPU}"/BOOST ]; then
    export BOOST_DIR=/usr/sci/crcnsdata/"${CPU}"/BOOST
fi

# add BOOST to the search paths:
if [ -n "${BOOST_DIR}" ]; then
    BOOST_ROOT=${BOOST_DIR}
    BOOST_INC=${BOOST_DIR}/include
    export BOOST_ROOT BOOST_INC
    
    CPATH=${BOOST_DIR}/include:${CPATH}
    LIBRARY_PATH=${BOOST_DIR}/lib:${LIBRARY_PATH}
    LD_LIBRARY_PATH=${BOOST_DIR}/lib:${LD_LIBRARY_PATH}
    export CPATH LIBRARY_PATH LD_LIBRARY_PATH
fi

# determine where THE lives:
if [ -e "${HOME}"/source_code/cxx/the ]; then
    export THE_DIR="${HOME}"/source_code/cxx/the
elif [ -e /home/sci/koshevoy/source_code/cxx/the ]; then
    export THE_DIR=/home/sci/koshevoy/source_code/cxx/the
fi

if [ -n "${THE_DIR}" ]; then
    CPATH=${THE_DIR}:${LIBRARY_PATH}
    export CPATH
fi

if [ -e /scratch/"${CPU}"/the ]; then
    export THE_DIR_CPU=/scratch/"${CPU}"/the
elif [ -e /usr/sci/crcnsdata/"${CPU}"/the ]; then
    export THE_DIR_CPU=/usr/sci/crcnsdata/"${CPU}"/the
elif [ -e /scratch/koshevoy/the ]; then
    export THE_DIR_CPU=/scratch/koshevoy/the
elif [ -e "${THE_DIR}/${CPU}" ]; then
    export THE_DIR_CPU="${THE_DIR}/${CPU}"
fi

if [ -n "${THE_DIR_CPU}" ]; then
    LIBRARY_PATH=${THE_DIR_CPU}:${LIBRARY_PATH}
    LD_LIBRARY_PATH=${THE_DIR_CPU}:${LD_LIBRARY_PATH}
    export LIBRARY_PATH LD_LIBRARY_PATH
fi
