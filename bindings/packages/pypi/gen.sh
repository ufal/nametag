#!/bin/sh

set -e

[ -n "$1" ] || { echo Usage: $0 version >&2; exit 1; }

dir=ufal.nametag
rm -rf $dir
mkdir -p $dir

# Local files
cp -a Changes ../../../LICENSE MANIFEST.in ../../../bindings/python/examples tests $dir
mkdir $dir/tests/data
cp ../test_data/test.ner $dir/tests/data

# NameTag sources and SWIG files
make -C ../../../src_lib_only nametag.cpp
make -C ../../../bindings/python clean
make -C ../../../bindings/python nametag_python.cpp
cp -a ../../../bindings/python/ufal $dir
cp ../../../src_lib_only/nametag.[ch]* $dir/ufal/nametag
cp ../../../bindings/python/nametag_python.cpp $dir/ufal/nametag

# Fill in version
sed "s/^\\( *version *= *'\\)[^']*'/\\1$1'/" setup.py >$dir/setup.py
sed "s/^# *__version__ *=.*$/__version__ = \"$1\"/" $dir/ufal/nametag/__init__.py -i

# README file
./README.sh >$dir/README

# Generate sdist
(cd ufal.nametag && python3 setup.py sdist && cd dist && tar xf ufal.nametag-$1.tar.gz)
