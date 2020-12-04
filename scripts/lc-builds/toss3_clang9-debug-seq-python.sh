#!/usr/bin/env bash

BUILD_SUFFIX=lc_toss3-clang-9.0.0-debug-seq-python

rm -rf ${BUILD_SUFFIX} 2>/dev/null
mkdir -p ${BUILD_SUFFIX}/install
mkdir -p ${BUILD_SUFFIX}/build && cd ${BUILD_SUFFIX}/build

module load cmake/3.14.5
module load clang/9.0.0

cmake \
  ../.. \
  -DCMAKE_BUILD_TYPE=Debug \
  -DCMAKE_CXX_COMPILER=/usr/tce/packages/clang/clang-9.0.0/bin/clang++ \
  -DCMAKE_C_COMPILER=/usr/tce/packages/clang/clang-9.0.0/bin/clang \
  -C ../../host-configs/lc-builds/toss3/clangX_tpl.cmake \
  -DENABLE_OPENMP=On \
  -DENABLE_MPI=Off \
  -DCMAKE_INSTALL_PREFIX=../install \
  "$@" \
