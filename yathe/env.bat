@echo off
rem
rem This file is generated
rem

echo Setting up a MinGW, Qt, ITK, and Boost environment...

set QTDIR=C:\scratch\qt422
echo QTDIR set to %QTDIR%
set QMAKESPEC=win32-g++
echo QMAKESPEC set to %QMAKESPEC%
echo configure -qt-libjpeg -qt-libmng -qt-libpng -qt-gif -qt-zlib -no-fast -static -release

set GLEW_DIR=C:\scratch\glew-135
echo GLEW_DIR set to %GLEW_DIR%

set CG_DIR=C:\PROGRA~1\NVIDIA~1\Cg
echo CG_DIR set to %CG_DIR%

set FFTW_DIR=C:\scratch\fftw-312.bin
echo FFTW_DIR set to %FFTW_DIR%

set BOOST_DIR=C:\scratch\boost133.1
set BOOST_ROOT=%BOOST_DIR%
set BOOST_INC=%BOOST_DIR%\include
echo BOOST_DIR set to %BOOST_DIR%

set ITK_SOURCE_DIR=C:\scratch\itk301p
set ITK_BINARY_DIR=C:\scratch\itk301p.bin
set ITK_DIR=%ITK_BINARY_DIR%
echo ITK_DIR set to %ITK_DIR%

set THE_DIR=H:\source_code\cxx\the
set THE_DIR_CPU=C:\scratch\the

set PATH=%THE_DIR_CPU%;%PATH%
set PATH=%ITK_DIR%\bin;%PATH%
set PATH=%QTDIR%\bin;%PATH%
set PATH=C:\MinGW\bin;%PATH%
set PATH=%PATH%;%SystemRoot%\System32

echo PATH set to %PATH%

set CPATH=%THE_DIR%;%BOOST_INC%;%GLEW_DIR%\include;%FFTW_DIR%\include;%CPATH%
set LIBRARY_PATH=%THE_DIR_CPU%;%BOOST_DIR%\lib;%GLEW_DIR%\lib;%FFTW_DIR%\lib;%LIBRARY_PATH%
