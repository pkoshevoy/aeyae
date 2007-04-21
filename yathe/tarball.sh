#!/bin/bash

# sanity check:
if [ ! -d .. ]; then
    echo cannot step into parent directory, aborting...
    exit 1
else
    cd ..
fi

# create an archive time stamp:
NAME=the/tarballs/`date +"%Y%m%d-%H%M"`.tar

# create the archive directory:
if [ ! -d the/tarballs ]; then
    mkdir the/tarballs
fi

# create an archive:
tar chf "${NAME}" \
the/Qt \
the/FLTK \
the/doc \
the/eh \
the/geom \
the/image \
the/io \
the/itk \
the/math \
the/opengl \
the/sel \
the/thread \
the/ui \
the/unused \
the/utils \
the/*.cmake \
the/CMakeLists.txt \
the/*.sh \
the/*.bat

# compress the archive:
bzip2 -9 "${NAME}"
