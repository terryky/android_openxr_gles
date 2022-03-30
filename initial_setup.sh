#!/bin/sh
set -e
set -x

git submodule update --init --recursive

cd third_party/Oculus_OpenXR_Mobile_SDK/
unzip ovr_openxr_mobile_sdk_38.0.zip

