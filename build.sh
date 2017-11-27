#!/bin/sh

mkdir -p ./build
pkexec docker run -a STDOUT -a STDERR -v `pwd`:/libglpp:Z kazade/dreamcast-sdk /bin/sh -c "source /etc/bash.bashrc; cd /libglpp/build; cmake -DCMAKE_TOOLCHAIN_FILE=/libglpp/toolchain/Dreamcast.cmake -DCMAKE_BUILD_TYPE=Release ..; make" | sed 's?\/libglpp?'`pwd`'?'
