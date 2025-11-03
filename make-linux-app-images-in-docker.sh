#!/bin/bash

set -x

mkdir -p /opt/local/build/aeyae
cd /opt/local/build/aeyae

if [ -z "${AEYAE_GIT_BRANCH}" ]; then
    AEYAE_GIT_BRANCH=replay
fi

export PATH=/opt/local/bin:"${PATH}"
export LD_LIBRARY_PATH=/opt/local/lib:"${LD_LIBRARY_PATH}"
export PKG_CONFIG_PATH=/opt/local/lib/pkgconfig:"${PKG_CONFIG_PATH}"

(cd /opt/local/src/aeyae && \
     git pull && \
     git submodule update --init --recursive --depth 1 && \
     find . -type f -iname '*.in' -exec touch {} \;)

(cd /opt/local/build/aeyae && \
    cmake \
    -DAEYAE_CXX_STANDARD=17 \
    -DBUILD_STATIC_LIBAEYAE=YES \
    -DCMAKE_INSTALL_PREFIX=/usr \
    -DCMAKE_VERBOSE_MAKEFILE=NO \
    -DEXTRA_LIBS="-lstdc++ -lm" \
    -DHDHOMERUN_INSTALL_PREFIX=/opt/local \
    -DINSTALL_LIB_PKGCONFIG_DIR=/opt/local/lib/pkgconfig \
    -DYAE_USE_QOPENGL_WIDGET=NO \
    /opt/local/src/aeyae && \
    cmake --build . -j $(nproc))

/opt/local/src/aeyae/make-linux-app-images.sh

for i in /opt/local/build/aeyae/.dist/*.AppImage; do
    cp "${i}" /artifacts/
done
