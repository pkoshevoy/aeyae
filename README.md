This is a short summary of development environment requirements for building Apprentice Video player and Aeyae Remux for Mac/Windows/Linux.

Download Apprentice Video Mac/Windows binaries: [https://sourceforge.net/projects/apprenticevideo/](https://sourceforge.net/projects/apprenticevideo/).

Download Aeyae Remux Mac/Windows binaries: [https://sourceforge.net/projects/aeyae-remux/](https://sourceforge.net/projects/aeyae-remux/).

##### Checkout the code, configure, build, install:
    git clone https://github.com/pkoshevoy/aeyae.git
    cd aeyae
    git submodule update --init --recursive
    git clone https://github.com/pkoshevoy/libhdhomerun.git
    cd libhdhomerun
    git checkout add-cmake
    mkdir -p ../libhdhomerun-build
    cd ../libhdhomerun-build
    cmake-gui ../libhdhomerun # Configure (adjust paths as desired), Generate
    nice make -j8 && make install
    mkdir -p ../aeyae-build
    cd ../aeyae-build
    cmake-gui ../aeyae # Configure (adjust paths as desired), Generate
    nice make -j8 && make install

##### All Platforms:
###### OpenGL 1.1 or later
*OpenGL* is used for frame rendering, so make sure you have OpenGL drivers for your graphics card.

###### CMake 3.1.0 or later
[http://www.cmake.org/cmake/resources/software.html](http://www.cmake.org/cmake/resources/software.html)

*CMake* is used to configure the build (to generate projects, or makefiles).

###### Qt 4.7+, or Qt 5.6+
[https://download.qt.io/archive/qt/4.7/](https://download.qt.io/archive/qt/4.7/)

*Qt* is the cross platform C++ UI toolkit.  Use Qt 4.7+ for OS X 10.5, OS X 10.6, and 32-bit windows builds.  Use Qt 5.6+ for newer versions of macOS and 64-bit windows builds.

###### boost 1.47.0 or later
[http://www.boost.org/](http://www.boost.org/)

*Boost* is used for its shared pointer and threading libraries.

###### GLEW 1.5 or later
[http://glew.sourceforge.net/](http://glew.sourceforge.net/)

*GLEW* is an OpenGL extension wrangler library.

###### portaudio v19
[http://www.portaudio.com/download.html](http://www.portaudio.com/download.html)

*Portaudio* is used for audio playback on Windows and Linux.

###### FFmpeg fresh from git
[http://ffmpeg.org/download.html](http://ffmpeg.org/download.html)

*FFmpeg* is used for video/audio demuxing and decoding.

###### libass 0.13 or later
[http://code.google.com/p/libass/](http://code.google.com/p/libass/)

*libass* is used for high quality subtitle rendering.

###### FontConfig 2.10+
[http://www.freedesktop.org/wiki/Software/fontconfig/](http://www.freedesktop.org/wiki/Software/fontconfig/)

*FontConfig* is required by libass for accessing system fonts.

##### Windows:
###### Dependency Walker 2.2
[http://www.dependencywalker.com/](http://www.dependencywalker.com/)

*depends.exe* is used for automatic external dependency detection by wixWrapper (instead of wix harvester tool).  When building *x86* version of Apprentice Video you'll need the 32-bit version of *depends.exe*; for *x64* build you'll need the 64-bit version...

###### WiX 3.5
[http://wix.sourceforge.net/](http://wix.sourceforge.net/)

WiX is used to build the installer package.  I've only used/tested WiX 3.5 and do not know whether later versions work as well.

You can try and build ffmpeg yourself from source using MinGW.  You can compile using Visual Studio 2010 (or 2012) and c99-to-c89 converter (this is what I used to do -- see [http://www.ffmpeg.org/platform.html](http://www.ffmpeg.org/platform.html) for details).  However, you may find it much easier to use the [Zeranoe](http://ffmpeg.zeranoe.com/builds/) builds instead.

Visual C++ 2008 SP1 or 2010 SP1 are recommended. VS Express Edition will work. Version 2010 or 2012 is required if you decide to compile ffmpeg yourself using c99-to-c89 converter. MinGW/MSYS development has not been tested, but should work.

It is possible to compile *FontConfig* for Windows using MinGW
tool-chain.  However, I've also [patched fontconfig 2.10](http://sourceforge.net/projects/apprenticevideo/files/fontconfig-2.10.x-cmake-patches-for-win32/) to add
*CMake* config file to enable native win32 compilation in Visual Studio using CMake.  This is what I currently use.

##### MacOS:
You can easily build prerequisite libraries from source yourself, but [macports](http://www.macports.org/) can do it for you.  If you run into problems where `install_name_tool` fails and suggests using `-headerpad_max_install_names` linker option -- you may try building from source yourself (make sure to use the
`-headerpad_max_install_names` option), or you can do a dirty hack and make `/usr/bin/ld` always use the `-headerpad_max_install_names` switch:
```
sudo mv /usr/bin/ld /usr/bin/ld.bin
sudo printf '#!/bin/sh\nexec /usr/bin/ld.bin -headerpad_max_install_names $*\n' > /usr/bin/ld
sudo chmod a+rx /usr/bin/ld
sudo port uninstall ffmpeg
sudo port install ffmpeg
```

##### Linux:
Use the package management mechanism provided by your Linux distribution (apt, yast, yum, etc...) to install required tools and development libraries (git, cmake, Qt 4, boost, glew, portaudio, yasm).  Use of the latest FFmpeg pulled from git is recommended, because compilation against libav is not tested often and may not work.

##### Optional (any platform):
If you would like to enable MOD file playback (MOD, XM, S3M, IT, etc...), compile *libmodplug* and make sure to enable modplug support when configuring ffmpeg build.
