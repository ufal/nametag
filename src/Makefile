# This file is part of NameTag <http://github.com/ufal/nametag/>.
#
# Copyright 2016 Institute of Formal and Applied Linguistics, Faculty of
# Mathematics and Physics, Charles University in Prague, Czech Republic.
#
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http://mozilla.org/MPL/2.0/.

include Makefile.include
include Makefile.options

EXECUTABLES = $(call exe,run_ner run_tokenizer train_ner)
DYNAMIC_LIBRARIES = $(call dynlib,libnametag)
STATIC_LIBRARIES = $(call lib,libnametag libnametag.dyn)
all: exe
exe: $(EXECUTABLES)
lib: $(DYNAMIC_LIBRARIES)
a: $(STATIC_LIBRARIES)
full: exe lib a

# dependency and compilation rules
include Makefile.rules

# libraries
$(call lib,libnametag): $(call obj, $(NAMETAG_OBJECTS))
	ar -rcs $@ $^
$(call lib,libnametag.dyn): $(call dynobj, $(NAMETAG_OBJECTS))
	ar -rcs $@ $^
$(call dynlib,libnametag): $(call dynobj,$(NAMETAG_OBJECTS))
	$(CXX) -o $@ $(DYN_LD_FLAGS) $^ $(call version_script,libnametag.map) $(call create_implib,libnametag)

# executables
$(call exe,train_ner): $(call obj, classifier/network_classifier_encoder features/feature_templates_encoder ner/bilou_ner_trainer ner/entity_map_encoder utils/compressor_save)
$(EXECUTABLES): $(call exe,%): $(call obj,% utils/options $(NAMETAG_OBJECTS))
	$(CXX) $^ -o $@ $(LD_FLAGS)

# cleaning
.PHONY: clean veryclean

clean:
	@$(call rmdir,.objs) $(call rm,$(EXECUTABLES) $(DYNAMIC_LIBRARIES) $(STATIC_LIBRARIES) $(call implib,libnametag))

# dump library sources
lib_sources:
	@echo $(patsubst %,%.cpp,$(NAMETAG_OBJECTS))
