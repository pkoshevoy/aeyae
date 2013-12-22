All Platforms:

	OpenGL 1.2 or later
	OpenGL is used for frame rendering, so make sure you have
	OpenGL drivers for you graphics card.

	CMake 2.8.6 or later	http://www.cmake.org/cmake/resources/software.html
	CMake is used to configure the build (to generate projects, or makefiles).

	Qt 4.7.4 or later	http://download.qt-project.org/archive/qt/4.7/
	Qt is the cross platform C++ UI toolkit.

	boost 1.47.0 or later	http://www.boost.org/
	Boost is used for its shared pointer and threading facilities.

	GLEW 1.5 or later	http://glew.sourceforge.net/
	GLEW is an OpenGL extension wrangler library.

	portaudio v19		http://www.portaudio.com/download.html
	Portaudio is used for audio playback.

	FFmpeg fresh from git	http://ffmpeg.org/download.html
	FFmpeg is used for video/audio demuxing and decoding.


Windows:
	Dependency Walker 2.2	http://www.dependencywalker.com/
	depends.exe is used for automatic external dependency detection
	by wixWrapper.

	WiX 3.5	or later	http://wix.sourceforge.net/
	WiX is used to build the installer package.

	ffmpeg win32 latest	http://ffmpeg.zeranoe.com/builds/
	You can try and build ffmpeg yourself from source using MinGW,
	but it would be much easier to use the Zeranoe builds instead.

	Visual C++ 2008 SP1 or 2010 SP1 are recommended.
	The VS Express will work.
	MinGW/MSYS development has not been tested, but should work.


MacOS:
	http://www.macports.org/

	You can easily build prerequisite libraries from source
	yourself, but macports can do it for you.  If you run into
	problems where install_name_tool fails and suggests using
	-headerpad_max_install_names linker option -- you may try
	building from source yourself (make sure to use the
	-headerpad_max_install_names option),
	or you can do a dirty hack and make /usr/bin/ld
	always use the -headerpad_max_install_names switch:

	sudo mv /usr/bin/ld /usr/bin/ld.bin
	sudo printf '#!/bin/sh\nexec /usr/bin/ld.bin -headerpad_max_install_names $*\n' > /usr/bin/ld
	sudo chmod a+rx /usr/bin/ld
	sudo port uninstall ffmpeg
	sudo port install ffmpeg


Linux:
	Use the package management mechanism provided by your Linux
	distribution (apt, yast, yum, etc...) to install required
	tools and development libraries (git, cmake, Qt 4, boost,
	glew, portaudio, yasm).  Use of the latest FFmpeg pulled from
	git is recommended, because compilation against libav fork is not
	tested and may not work.

