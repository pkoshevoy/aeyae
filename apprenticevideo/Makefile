# compiler, tools and options:

CC       = gcc
CXX      = g++
LINK     = g++
LFLAGS   = -L/scratch/x86_64/ffmpeg/lib -lavformat -lavcodec -lavutil -lswscale -lz -lbz2
DEL_FILE = rm -f
TARGET   = yae

CFLAGS   =	-Wno-long-long \
		-Wall

ifeq ($(CONFIG), DEBUG)
    CFLAGS +=	-DDEBUG \
		-fno-inline \
		-g \
		-O0
else
    CFLAGS +=	-DNDEBUG \
		-finline \
		-O2
endif

CXXFLAGS =	$(CFLAGS) \
		-Wnon-virtual-dtor \
		-Wctor-dtor-privacy

INCPATH  =	-I. \
		-I/scratch/x86_64/ffmpeg/include

HEADERS =	yaeAPI.h \
		yaeReader.h \
		yaeReaderFFMPEG.h

SOURCES =	yaeMain.cpp \
		yaeAPI.cpp \
		yaeReaderFFMPEG.cpp

OBJECTS =	$(filter %.o, $(SOURCES:%.cpp=%.o) $(SOURCES:%.cxx=%.o))

# implicit rules:

.SUFFIXES: .c .cxx .cpp .cc .C

.cpp.o:
	$(CXX) -c $(CXXFLAGS) $(INCPATH) -o $@ $<

.cc.o:
	$(CXX) -c $(CXXFLAGS) $(INCPATH) -o $@ $<

.cxx.o:
	$(CXX) -c $(CXXFLAGS) $(INCPATH) -o $@ $<

.C.o:
	$(CXX) -c $(CXXFLAGS) $(INCPATH) -o $@ $<

.c.o:
	$(CC) -c $(CFLAGS) $(INCPATH) -o $@ $<

# build rules:

first: all

all: $(TARGET)

$(TARGET): $(OBJECTS)
	$(LINK) -o $(TARGET) $(OBJECTS) $(LFLAGS)

clean:
	-$(DEL_FILE) $(OBJECTS)
	-$(DEL_FILE) *~ core *.core

distclean: clean
	-$(DEL_FILE) $(TARGET) $(TARGET) Makefile.deps

# generate dependency rules:
Makefile.deps: Makefile
	g++ -MM $(INCPATH) -c $(SOURCES) > Makefile.deps

# include dependency rules:
include Makefile.deps
