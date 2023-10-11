#!/bin/sh

basepath=$(cd `dirname $0`; pwd)
work_dir=$basepath/..
grpc_dir=$work_dir/3rdparty/grpc
mkdir -p $grpc_dir
if [ ! -d $grpc_dir/grpc ]; then
  tar xzf $work_dir/pkg/grpc-1.50.0.tar.gz -C $grpc_dir
fi
if [ ! -d $grpc_dir/release ]; then
  mkdir -p $grpc_dir/release
  cd $grpc_dir/grpc
  mkdir -p build
  cd build
  cmake -DgRPC_INSTALL=ON -DgRPC_BUILD_TESTS=OFF -DBUILD_SHARED_LIBS=ON -DCMAKE_INSTALL_PREFIX=$grpc_dir/release ..
  make -j4
  make install
fi

