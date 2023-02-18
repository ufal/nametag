#!/bin/sh

# Prepare API documentation and examples
make -C ../../../doc nametag_bindings_api.txt >/dev/null

cat <<"EOF"
ufal.nametag
============

The ``ufal.nametag`` is a Python binding to NameTag library <http://ufal.mff.cuni.cz/nametag>.

The bindings is a straightforward conversion of the ``C++`` bindings API.
Python >=3 is supported.


Wrapped C++ API
---------------

The C++ API being wrapped follows. For a API reference of the original
C++ API, see <http://ufal.mff.cuni.cz/nametag/api-reference>.

::

EOF
tail -n+4 ../../../doc/nametag_bindings_api.txt | sed 's/^/  /'
cat <<EOF


Examples
========

run_ner
-------

Simple example performing named entity recognition::

EOF
sed '1,/^$/d' ../../../bindings/python/examples/run_ner.py | sed 's/^/  /'
cat <<EOF


AUTHORS
=======

Milan Straka <straka@ufal.mff.cuni.cz>

Jana Straková <strakova@ufal.mff.cuni.cz>


COPYRIGHT AND LICENCE
=====================

Copyright 2016 Institute of Formal and Applied Linguistics, Faculty of
Mathematics and Physics, Charles University in Prague, Czech Republic.

This Source Code Form is subject to the terms of the Mozilla Public
License, v. 2.0. If a copy of the MPL was not distributed with this
file, You can obtain one at http://mozilla.org/MPL/2.0/.
