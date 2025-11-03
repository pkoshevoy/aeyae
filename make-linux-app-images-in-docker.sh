#!/bin/bash

set -x

cd /opt/local/build/aeyae
export PATH=/opt/local/bin:"${PATH}"
export LD_LIBRARY_PATH=/opt/local/lib:"${LD_LIBRARY_PATH}"
/opt/local/src/aeyae/make-linux-app-images.sh

for i in /opt/local/build/aeyae/.dist/*.AppImage; do
    cp "${i}" /artifacts/
done
