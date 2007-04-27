@echo off
echo Setting up a MinGW, Qt, ITK, Boost, Cg, GLEW, etc...

set QTDIR=C:\scratch\qt422
echo QTDIR set to %QTDIR%
set QMAKESPEC=win32-g++
echo QMAKESPEC set to %QMAKESPEC%
echo configure -qt-libjpeg -qt-libmng -qt-libpng -qt-gif -qt-zlib -no-fast -static -release

set GLEW_DIR=C:\scratch\glew-135
echo GLEW_DIR set to %GLEW_DIR%

set CG_DIR=C:\PROGRA~1\NVIDIA~1\Cg
echo CG_DIR set to %CG_DIR%
set CG_LIB_DIR=%CG_DIR%\lib
echo CG_LIB_DIR set to %CG_LIB_DIR%

set FFTW_DIR=C:\scratch\fftw-312.bin
echo FFTW_DIR set to %FFTW_DIR%

set BOOST_ROOT=C:\scratch\boost133.1
echo BOOST_ROOT set to %BOOST_ROOT%

set ITK_SOURCE_DIR=C:\scratch\itk301p
set ITK_BINARY_DIR=C:\scratch\itk301p.bin
set ITK_DIR=%ITK_BINARY_DIR%
echo ITK_DIR set to %ITK_DIR%

set THE_SRC_DIR=H:\src\CRCNS\the
set THE_BIN_DIR=C:\scratch\the

set PATH=%THE_BIN_DIR%;%PATH%
set PATH=%ITK_DIR%\bin;%PATH%
set PATH=%QTDIR%\bin;%PATH%
set PATH=C:\MinGW\bin;%PATH%
set PATH=%PATH%;%SystemRoot%\System32

echo PATH set to %PATH%

set CPATH=%THE_SRC_DIR%;%BOOST_ROOT%\include\BOOST-~1;%GLEW_DIR%\include;%CG_DIR%\include;%FFTW_DIR%\include;%CPATH%
set LIBRARY_PATH=%THE_BIN_DIR%;%BOOST_ROOT%\lib;%GLEW_DIR%\lib;%FFTW_DIR%\lib;%ITK_DIR%\bin;%LIBRARY_PATH%
