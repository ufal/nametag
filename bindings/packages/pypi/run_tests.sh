#!/bin/sh

PYTHONPATH=`echo ufal.nametag/build/lib.*` python3 -m unittest discover -s ufal.nametag/tests
