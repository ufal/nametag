#!/bin/sh

../../../src/train_ner generic morphodita:test.tagger test.ner.ft 2 5 0 0.1 0.01 0.5 0 >test.ner <<EOF
Kočka	I-animal
vidí	_
.	_

Vidím	_
kočky	I-animal
.	_
EOF
