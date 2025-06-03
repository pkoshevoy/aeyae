This is a short summary of development environment requirements for building apprenticevideo player, aeyaeremux, yaetv, etc... for Mac/Windows/Linux.

Download ApprenticeVideo Mac/Windows/Linux binaries: [https://sourceforge.net/projects/apprenticevideo/](https://sourceforge.net/projects/apprenticevideo/).

Download AeyaeRemux Mac/Windows/Linux binaries: [https://sourceforge.net/projects/aeyae-remux/](https://sourceforge.net/projects/aeyae-remux/).

Download YaeTV Mac/Windows/Linux binaries: [https://sourceforge.net/projects/yaetv/](https://sourceforge.net/projects/yaetv/).

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
[https://cmake.org/download/](https://cmake.org/download/)

*CMake* is used to configure the build (to generate projects, or makefiles).

###### Qt 4.7+, Qt 5.6+, Qt 6.6+
[https://www.qt.io/download-qt-installer-oss](https://www.qt.io/download-qt-installer-oss)

*Qt* is the cross platform C++ UI toolkit.
Use Qt 4.7+ for OS X 10.4 (ppc), 10.5 (ppc), 10.6 (intel), and 32-bit windows builds.
Use Qt 5.6+ or newer for later versions of macOS and 64-bit windows builds.

###### boost 1.47.0 or later
[https://www.boost.org/releases/latest/](https://www.boost.org/releases/latest/)

*Boost* is used for its shared pointer and threading libraries.

###### GLEW 1.5 or later
[http://glew.sourceforge.net/](http://glew.sourceforge.net/)

*GLEW* is an OpenGL extension wrangler library.

###### portaudio v19
[https://files.portaudio.com/download.html](https://files.portaudio.com/download.html)

*Portaudio* is used for audio playback on Windows and Linux.

###### FFmpeg fresh from git
[https://ffmpeg.org/download.html](https://ffmpeg.org/download.html)

*FFmpeg* is used for video/audio demuxing and decoding.

###### libass 0.13 or later
[https://github.com/libass/libass/releases](https://github.com/libass/libass/releases)

*libass* is used for high quality subtitle rendering.

###### FontConfig 2.10+
[https://www.freedesktop.org/wiki/Software/fontconfig/](https://www.freedesktop.org/wiki/Software/fontconfig/)

*FontConfig* is required by libass for accessing system fonts.

##### Windows:
###### Dependency Walker 2.2
[http://www.dependencywalker.com/](http://www.dependencywalker.com/)

*depends.exe* is used for automatic external dependency detection by wixWrapper (instead of wix harvester tool).
When building *x86* version of Apprentice Video you'll need the 32-bit version of *depends.exe*; for *x64* build you'll need the 64-bit version...

###### WiX 3.5
[http://wix.sourceforge.net/](http://wix.sourceforge.net/)

WiX is used to build the installer package.  I've only used/tested WiX 3.5 and do not know whether later versions work as well.

You can try and cross-compile ffmpeg yourself from source on linux using Zeranoe build scripts: [https://github.com/Zeranoe/mingw-w64-build](https://github.com/Zeranoe/mingw-w64-build).
However, you may find it much easier to use the ffmpeg nightly builds instead: [https://github.com/BtbN/FFmpeg-Builds/releases](https://github.com/BtbN/FFmpeg-Builds/releases).

Windows 32-bit intel binaries are produced with Visual Studio 2015.
Windows 64-bit intel binaries are produced with Visual Studio Community Edition 2019, or 2022.
Compilation with MinGW/MSYS2 has not been tested, and probably doesn't work.

It is possible to compile *FontConfig* for Windows using MinGW tool-chain,
but recently I simply use the pre-compiled ming64 packages provided by msys2 or cygwin.

##### MacOS:
You can build prerequisite libraries from source yourself, but [https://www.macports.org/](https://www.macports.org/) can easily do it for you -- it's what I use.

The OS X 10.4, 10.5, 10.6 builds are produced with Qt 4.8 and release/6.1 branch of ffmpeg.
The macOS 10.13+ builds are produced with Qt 5 provided by macports, and either release/6.1 or master branch of ffmpeg.

##### Linux:
Use the package management mechanism provided by your Linux distribution (apt, yast, yum, etc...) to install required tools and development libraries (git, cmake, Qt 4/5/6, boost, glew, portaudio, yasm/nasm).
Use of the latest FFmpeg pulled from git is recommended.
I normally build and test on openSuSE, and occasioanlly on Kubuntu.

The provided .AppImage linux builds are produced on a Kubuntu 14.04 LTS VM with Qt 5.6.3 and ffmpeg release/6.1 branch, and should work on pretty much any current linux distribution.

##### Optional (any platform):
If you would like to enable MOD file playback (MOD, XM, S3M, IT, etc...), install libmodplug or libopenmpt and enable modplug/openmpt support when configuring ffmpeg.
