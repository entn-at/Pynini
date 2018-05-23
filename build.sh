#!/bin/bash

if [[ "$(python --version)" != "Python 3.6"* ]]; then
    echo "ERROR: Python 3.6 required."
    exit 1
fi

# All dependent libs will reside here
SHARED_OBJECTS_DIR="$(pwd)/lib"
mkdir "${SHARED_OBJECTS_DIR}"

mkdir deps

pushd deps

OPENFST_DIR="$(pwd)/openfst_build"
RE2_DIR="$(pwd)/re2"

# Compile OpenFST
if [ ! -f openfst-1.6.7/.compiled ]; then
  wget http://www.openfst.org/twiki/pub/FST/FstDownload/openfst-1.6.7.tar.gz
  tar -xvf openfst-1.6.7.tar.gz
  pushd openfst-1.6.7
  ./configure --prefix="${OPENFST_DIR}" --enable-grm CXXFLAGS="-O3 -march=native"
  make -j
  make install
  touch .compiled
  popd
fi

# Compile re2
if [ ! -f re2/.compiled ]; then
  git clone http://github.com/google/re2
  pushd re2
  git checkout 2018-03-01
  git pull
  make -j
  touch .compiled
  popd
fi

popd

# Move all dependent *.so to one location
cp -P ${OPENFST_DIR}/lib/*.so* "${SHARED_OBJECTS_DIR}/"
cp -P ${RE2_DIR}/obj/so/*.so* "${SHARED_OBJECTS_DIR}/"

# Build and install pynini
pip install -r requirements.txt
CPATH="${OPENFST_DIR}/include:${RE2_DIR}" \
LIBRARY_PATH="${SHARED_OBJECTS_DIR}" \
python setup.py install

echo "Done!"
echo "To be able to import pynini, make *.so libraries in ${SHARED_OBJECTS_DIR} visible to python."

