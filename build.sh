#!/bin/bash

if [[ "$(python --version)" != "Python 3.6"* ]]; then
    echo "ERROR: Python 3.6 required."
    exit 1
fi

mkdir deps

pushd deps

OPENFST_DIR="$(pwd)/openfst_build"
RE2_DIR="$(pwd)/re2"

wget http://www.openfst.org/twiki/pub/FST/FstDownload/openfst-1.6.7.tar.gz
tar -xvf openfst-1.6.7.tar.gz
pushd openfst-1.6.7
./configure --prefix="${OPENFST_DIR}" --enable-grm CXXFLAGS="-O3 -march=native"
make -j
make install
popd

git clone http://github.com/google/re2
pushd re2
git checkout 2018-03-01
git pull
make -j
popd

popd

pip install -r requirements.txt
CPATH="${OPENFST_DIR}/include:${RE2_DIR}" \
LIBRARY_PATH="${OPENFST_DIR}/lib:${RE2_DIR}/obj/so" \
python setup.py install
