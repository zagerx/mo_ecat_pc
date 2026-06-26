#!/bin/bash
set -e

# 切换到脚本所在目录
cd "$(dirname "$0")"

BUILD_DIR="build"
BUILD_TYPE="${1:-Release}"

echo "==> Configure CMake (BUILD_TYPE=${BUILD_TYPE})"
cmake -S . -B "${BUILD_DIR}" -DCMAKE_BUILD_TYPE="${BUILD_TYPE}"

echo "==> Build"
cmake --build "${BUILD_DIR}" -j"$(nproc)"

echo "==> Build finished: ${BUILD_DIR}/app/mo_ecat_pc_app"
