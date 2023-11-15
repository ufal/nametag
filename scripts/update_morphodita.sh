#!/bin/bash

# This file is part of NameTag <http://github.com/ufal/nametag/>.
#
# Copyright 2016 Institute of Formal and Applied Linguistics, Faculty of
# Mathematics and Physics, Charles University in Prague, Czech Republic.
#
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http://mozilla.org/MPL/2.0/.

rm -rf ../src/morphodita/*
git -C ../src/morphodita clone --depth=1 --branch=stable https://github.com/ufal/morphodita
cp -a ../src/morphodita/morphodita/src/{derivator,morpho,tagger,tagset_converter,tokenizer,version,Makefile.include} ../src/morphodita/
cp -a ../src/morphodita/morphodita/{AUTHORS,CHANGES.md,LICENSE,README} ../src/morphodita/
rm -rf ../src/morphodita/morphodita/
sed '
  s/^namespace morphodita {/namespace nametag {\n&/
  s/^} \/\/ namespace morphodita/&\n} \/\/ namespace nametag/
  ' -i ../src/morphodita/*/*
perl -ple '
  BEGIN { use File::Basename }

  /^#include "([^"]*)"$/ and !-f dirname($ARGV)."/$1" and -f "../src/morphodita/$1" and $_ = "#include \"morphodita/$1\"";
  ' -i ../src/morphodita/*/*
