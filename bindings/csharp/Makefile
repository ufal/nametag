# This file is part of NameTag <http://github.com/ufal/nametag/>.
#
# Copyright 2015 Institute of Formal and Applied Linguistics, Faculty of
# Mathematics and Physics, Charles University in Prague, Czech Republic.
#
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http://mozilla.org/MPL/2.0/.

include ../common/Makefile.common

BINDING_MODULE=$(call dynlib,$(if $(filter win-%,$(PLATFORM)),nametag_csharp,libnametag_csharp))
ifeq ($(PLATFORM),win-gcc)
  BINDING_LD_FLAGS+=-Wl,--kill-at
endif

all: $(BINDING_MODULE)

$(call dynobj,nametag_csharp):C_FLAGS+=$(BINDING_C_FLAGS)
$(BINDING_MODULE): $(call dynobj,nametag_csharp) $(BINDING_NAMETAG_OBJECTS)
	$(call link_dynlib,$@,$^ $(BINDING_C_FLAGS), $(BINDING_LD_FLAGS) $(call version_script,nametag_csharp.map))

nametag_csharp.cpp: nametag_csharp.i ../common/nametag.i
	@$(call mkdir,Ufal/NameTag)
	swig $(SWIG_FLAGS) -csharp -namespace Ufal.NameTag -outdir $(call platform_name,Ufal/NameTag) -o $@ $<

.PHONY: clean
clean:
	@$(call rm,.build Ufal $(call all_dynlib,nametag_csharp libnametag_csharp) nametag_csharp.cpp $(wildcard examples/*.class))
