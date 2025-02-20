#!/usr/bin/env bash

BuildType=Release
#BuildType=Debug

export ANDROID_HOME="$(pwd)/../android-sdk"
export GRADLE_HOME="$(pwd)/../gradle"

cmake -B ../Jazz2-Android-${BuildType}_x86-64 -D CMAKE_BUILD_TYPE=${BuildType} -D CMAKE_PREFIX_PATH=$(pwd)/../nCine-external -D EXTERNAL_ANDROID_DIR=$(pwd)/../nCine-android-external -D NCINE_LINKTIME_OPTIMIZATION=ON -D NCINE_BUILD_ANDROID=ON -D NCINE_ASSEMBLE_APK=ON -D NCINE_NDK_ARCHITECTURES="x86_64" -D NDK_DIR=$(pwd)/../android-ndk
make -j2 -C ../Jazz2-Android-${BuildType}_x86-64