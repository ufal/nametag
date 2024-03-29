# This file is part of NameTag <http://github.com/ufal/nametag/>.
#
# Copyright 2015 Institute of Formal and Applied Linguistics, Faculty of
# Mathematics and Physics, Charles University in Prague, Czech Republic.
#
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http://mozilla.org/MPL/2.0/.

include ../common/Makefile.common

BINDING_MODULE=$(call dynlib,auto/Ufal/NameTag/NameTag)
BINDING_C_FLAGS+=$(if $(PERL_INCLUDE),,$(error PERL_INCLUDE must be set to the directory containing perl.h))
BINDING_C_FLAGS+=$(call include_dir,$(PERL_INCLUDE))
BINDING_C_FLAGS+=$(if $(findstring -gcc-,$(PLATFORM))$(findstring -clang-,$(PLATFORM)),-w)
BINDING_C_FLAGS+=$(if $(filter macos-%,$(PLATFORM)),-Wno-reserved-user-defined-literal,)

all: $(BINDING_MODULE)

$(call dynobj,nametag_perl):C_FLAGS+=$(BINDING_C_FLAGS)
$(BINDING_MODULE): $(call dynobj,nametag_perl) $(BINDING_NAMETAG_OBJECTS)
	@$(call mkdir,$(dir $@))
	@$(call touch,$(dir $@)NameTag.bs)
	$(call link_dynlib,$@,$^ $(BINDING_C_FLAGS), $(BINDING_LD_FLAGS) $(call version_script,nametag_perl.map))

nametag_perl.cpp: nametag_perl.i ../common/nametag.i
	@$(call mkdir,Ufal)
	swig $(SWIG_FLAGS) -perl -outdir Ufal -o $@ $<

.PHONY: clean
clean:
	@$(call rm,.build Ufal auto nametag_perl.cpp)
