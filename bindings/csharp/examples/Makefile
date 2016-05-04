# This file is part of NameTag <http://github.com/ufal/nametag/>.
#
# Copyright 2015 Institute of Formal and Applied Linguistics, Faculty of
# Mathematics and Physics, Charles University in Prague, Czech Republic.
#
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http://mozilla.org/MPL/2.0/.

TARGETS = RunNer.exe

all: $(TARGETS)

%.exe: %.cs
	mcs -r:System.Xml $< $(wildcard ../Ufal/NameTag/*)

.PHONY: clean
clean:
	rm -rf $(TARGETS)
