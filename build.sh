#!/bin/bash

if [[ "$(python --version)" != "Python 3.6"* ]]; then
    echo "ERROR: Python 3.6 required."
    exit 1
fi

which cmake &>/dev/null
HAS_CMAKE=$?
if [ ${HAS_CMAKE} -ne 0 ]; then
    echo "ERROR: CMake required"
    exit 1
fi

# All dependent libs will reside here
SHARED_OBJECTS_DIR="$(pwd)/lib"
mkdir "${SHARED_OBJECTS_DIR}"

# Installations will reside here
INSTALL_DIR="$(pwd)/installs"
mkdir "${INSTALL_DIR}"
OPENFST_INSTALL="${INSTALL_DIR}/openfst_build"
RE2_INSTALL="${INSTALL_DIR}/re2"

mkdir deps

pushd deps

    # Compile OpenFST
    if [ ! -f openfst-1.6.7/.compiled ]; then
      wget http://www.openfst.org/twiki/pub/FST/FstDownload/openfst-1.6.7.tar.gz
      tar -xvf openfst-1.6.7.tar.gz
      pushd openfst-1.6.7
          ./configure --prefix="${OPENFST_INSTALL}" --enable-grm CXXFLAGS="-O3 -march=native"
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
          mkdir -p build
          pushd build
              cmake .. -DCMAKE_INSTALL_PREFIX:PATH="${RE2_INSTALL}" -DBUILD_SHARED_LIBS=TRUE -DCMAKE_BUILD_TYPE=Release
              make -j
              make install
          popd
          touch .compiled
      popd
    fi

popd

# Move all dependent *.so to one location
cp -P ${OPENFST_INSTALL}/lib/*.so* "${SHARED_OBJECTS_DIR}/"
cp -P ${RE2_INSTALL}/lib/*.so* "${SHARED_OBJECTS_DIR}/"

# Build and install pynini
pip install -r requirements.txt
CPATH="${OPENFST_INSTALL}/include:${RE2_INSTALL}/include" \
LIBRARY_PATH="${SHARED_OBJECTS_DIR}" \
python setup.py install

echo "Done!"
echo "To be able to import pynini, make *.so libraries in ${SHARED_OBJECTS_DIR} visible to python."

