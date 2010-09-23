TEMPLATE = app
TARGET = yaeViewer
QT += core gui
CONFIG += qt thread exceptions rtti stl uic resources

CONFIG -= release
CONFIG -= debug
CONFIG += debug
#CONFIG += release

MOC_DIR = moc_dir
UI_DIR = ui_dir

DEFINES += YAE_STATIC

debug {
	OBJECTS_DIR = debug
	DEFINES += _DEBUG
	DEFINES += DEBUG=1
}

release {
	OBJECTS_DIR = release
	DEFINES += NDEBUG
}

win32 {
	DEFINES += NOMINMAX
	DEFINES += __STDC_CONSTANT_MACROS
	DEFINES -= UNICODE
	QMAKE_CXXFLAGS -= -Zc:wchar_t-
	QMAKE_CXXFLAGS += -Zc:wchar_t
	QMAKE_CXXFLAGS_DEBUG += /wd4335 /wd4996 /wd4995 /wd4800
	QMAKE_CXXFLAGS_RELEASE += /O2 /wd4335 /wd4996 /wd4995
	INCLUDEPATH += c:\msys\1.0\local\include \
		       .\msIntTypes
	LIBS += -Lc:\msys\1.0\local\include \
	     	-lnut \
		-lws2_32 \
		-lavicap32 \
		-lavifil32 \
		-lpsapi
}

macx {
	QMAKE_CXXFLAGS_DEBUG += -fasm-blocks -fvisibility=default -fpascal-strings
	QMAKE_CXXFLAGS_RELEASE += -fasm-blocks -fvisibility=default -fpascal-strings
    
#	QMAKE_CXXFLAGS_DEBUG += -mmacosx-version-min=10.4
#	QMAKE_CXXFLAGS_RELEASE += -mmacosx-version-min=10.4
    
	QMAKE_CXXFLAGS_DEBUG += -fstack-check
#	QMAKE_CXXFLAGS_DEBUG += -fno-stack-check -Wno-system-headers -Wno-reorder -Wno-unused -Wno-non-virtual-dtor
}

linux-g++-64 | linux-g++-32 | linux-g++ | win32-g++ {
	message("Adding libs for g++")
	LIBS += -L/local/lib \
	     	-L/scratch/x86_64/ffmpeg/lib \
	     	-lavformat \
		-lavcodec \
		-lavutil \
		-lswscale \
		-lboost_thread \
		-lpthread \
		-lfaad \
		-lgsm \
		-lmp3lame \
		-lopencore-amrnb \
		-lopencore-amrwb \
		-lopenjpeg \
		-lschroedinger-1.0 \
		-lorc-0.4 \
		-lspeex \
		-ltheoraenc \
		-ltheoradec \
		-logg \
		-lvorbisenc \
		-lvorbis \
		-lvpx \
		-lx264 \
		-lxvidcore \
		-lz \
		-lbz2 \
		-lm \
}

win32-g++ {
	message("Adding libs for win32-g++")
	LIBS  += \
		-lnut \
		-lws2_32 \
		-lavicap32 \
		-lavifil32 \
		-lpsapi
}

SOURCES =	yaeMain.cpp \
		yaeAPI.cpp \
		yaeReaderFFMPEG.cpp \
		yaeViewer.cpp

HEADERS =	yaeAPI.h \
		yaeReaderFFMPEG.h \
		yaeReader.h \
		yaeViewer.h

INCLUDEPATH +=	$$(BOOST_INCLUDEDIR) \
		.
		
