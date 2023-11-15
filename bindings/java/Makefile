# This file is part of NameTag <http://github.com/ufal/nametag/>.
#
# Copyright 2015 Institute of Formal and Applied Linguistics, Faculty of
# Mathematics and Physics, Charles University in Prague, Czech Republic.
#
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http://mozilla.org/MPL/2.0/.

include ../common/Makefile.common

BINDING_MODULE=$(call dynlib,$(if $(filter win-%,$(PLATFORM)),nametag_java,libnametag_java))
BINDING_C_FLAGS+=$(if $(JAVA_HOME),,$(error JAVA_HOME must be set))
BINDING_C_FLAGS+=$(if $(findstring -gcc-,$(PLATFORM))$(findstring -clang-,$(PLATFORM)),-fno-strict-aliasing)
ifneq ($(filter linux-%,$(PLATFORM)),)
  BINDING_C_FLAGS+=$(call include_dir,$(JAVA_HOME)/include $(JAVA_HOME)/include/linux)
else ifneq ($(filter win-%,$(PLATFORM)),)
  BINDING_C_FLAGS+=$(call include_dir,"$(JAVA_HOME)"/include "$(JAVA_HOME)"/include/win32)
  ifneq ($(filter win-gcc-%,$(PLATFORM)),)
    BINDING_LD_FLAGS+=-Wl,--kill-at
  endif
else ifneq ($(filter macos-%,$(PLATFORM)),)
  BINDING_C_FLAGS+=$(call include_dir,$(JAVA_HOME)/include $(JAVA_HOME)/include/darwin $(patsubst %/Home,%/Headers,$(filter %/Home,$(JAVA_HOME))))
endif

all: $(BINDING_MODULE) nametag.jar

$(call dynobj,nametag_java):C_FLAGS+=$(BINDING_C_FLAGS)
$(BINDING_MODULE): $(call dynobj,nametag_java) $(BINDING_NAMETAG_OBJECTS)
	$(call link_dynlib,$@,$^ $(BINDING_C_FLAGS), $(BINDING_LD_FLAGS) $(call version_script,nametag_java.map))

nametag_java.cpp: nametag_java.i ../common/nametag.i
	@$(call mkdir,cz/cuni/mff/ufal/nametag)
	swig $(SWIG_FLAGS) -java -package cz.cuni.mff.ufal.nametag -outdir $(call platform_name,cz/cuni/mff/ufal/nametag) -o $@ $<

nametag.jar: nametag_java.cpp
	javac -source 7 -Xlint:-options $(wildcard cz/cuni/mff/ufal/nametag/*.java)
	jar cf $@ $(patsubst %.java,%.class,$(wildcard cz/cuni/mff/ufal/nametag/*.java))

.PHONY: clean
clean:
	@$(call rm,.build cz $(call all_dynlib,nametag_java libnametag_java) nametag.jar nametag_java.cpp $(wildcard examples/*.class))
