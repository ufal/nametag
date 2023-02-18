#!/bin/sh

set -e

[ -n "$1" ] || { echo Usage: $0 version >&2; exit 1; }

dir=Ufal-NameTag
rm -rf $dir
mkdir -p $dir

# Local files
cp -a Build.PL Changes ../../../LICENSE README ../../../bindings/perl/examples t $dir
mkdir $dir/t/data
cp ../test_data/test.ner $dir/t/data

# NameTag sources
make -C ../../../src_lib_only nametag.cpp
mkdir $dir/nametag
cp ../../../src_lib_only/nametag.[ch]* $dir/nametag

# SWIG files
make -C ../../../bindings/perl nametag_perl.cpp
cp ../../../bindings/perl/nametag_perl.cpp $dir/nametag
mkdir -p $dir/lib/Ufal
cp ../../../bindings/perl/Ufal/NameTag.pm NameTag.xs $dir/lib/Ufal

# Fill in version
perl -e '%p=(); while(<>) {
  print "=head1 VERSION\n\n'"$1"'\n\n" if /^=head1 DESCRIPTION/;
  print;
  print "our \$VERSION = '"'$1'"';\n" if /^package ([^;]*);$/ and not $p{$1}++;
} ' -i $dir/lib/Ufal/NameTag.pm

# POD file
./NameTag.pod.sh >$dir/lib/Ufal/NameTag.pod

# Generate manifest
(cd Ufal-NameTag && perl Build.PL && perl Build manifest && perl Build dist && mv Ufal-NameTag-*$1.tar.gz .. && perl Build distclean && rm -f MANIFEST.SKIP.bak)
