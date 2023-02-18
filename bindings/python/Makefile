# This file is part of NameTag <http://github.com/ufal/nametag/>.
#
# Copyright 2015 Institute of Formal and Applied Linguistics, Faculty of
# Mathematics and Physics, Charles University in Prague, Czech Republic.
#
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http://mozilla.org/MPL/2.0/.

include ../common/Makefile.common

BINDING_MODULE=$(if $(filter macos-%,$(PLATFORM)),ufal_nametag.so,$(call dynlib,ufal_nametag))
BINDING_C_FLAGS+=$(if $(PYTHON_INCLUDE),,$(error PYTHON_INCLUDE must be set to the directory containing Python.h))
BINDING_C_FLAGS+=$(call include_dir,$(PYTHON_INCLUDE))
BINDING_C_FLAGS+=$(if $(findstring -gcc-,$(PLATFORM))$(findstring -clang-,$(PLATFORM)),-w)

all: $(BINDING_MODULE)

$(call dynobj,nametag_python):C_FLAGS+=$(BINDING_C_FLAGS)
$(BINDING_MODULE): $(call dynobj,nametag_python) $(BINDING_NAMETAG_OBJECTS)
	$(call link_dynlib,$@,$^ $(BINDING_C_FLAGS), $(BINDING_LD_FLAGS)

nametag_python.cpp: nametag_python.i ../common/nametag.i
	@$(call mkdir,ufal)
	@$(call echo,>ufal/__init__.py)
	swig $(SWIG_FLAGS) -python -outdir ufal -o $@ -interface ufal_nametag -builtin $<

.PHONY: clean
clean:
	@$(call rm,.build ufal $(call all_dynlib,ufal_nametag) nametag_python.cpp)
