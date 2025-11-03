#!/bin/bash -xe

docker build \
       -t ubuntu-latest-qt5 \
       -f Dockerfile.linux_build_with_qt5 .

docker run --rm \
       --env SIGN_APP_IMAGE="${SIGN_APP_IMAGE}" \
       --volume=$HOME/.gnupg:/root/.gnupg \
       --volume=$PWD:/artifacts \
       --entrypoint /opt/local/src/aeyae/make-linux-app-images-in-docker.sh \
       ubuntu-latest-qt5
