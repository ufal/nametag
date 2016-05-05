# This file is part of NameTag <http://github.com/ufal/nametag/>.
#
# Copyright 2016 Institute of Formal and Applied Linguistics, Faculty of
# Mathematics and Physics, Charles University in Prague, Czech Republic.
#
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http://mozilla.org/MPL/2.0/.

include Makefile.builtem
include Makefile.include
include rest_server/microrestd/Makefile.include

EXECUTABLES = $(call exe,run_ner run_tokenizer train_ner)
SERVER = $(call exe,rest_server/nametag_server)
LIBRARIES = $(call lib,libnametag)

.PHONY: all exe server lib full
all: exe
exe: $(EXECUTABLES)
server: $(SERVER)
lib: $(LIBRARIES)
full: exe server lib

# libraries
$(call lib,libnametag): $(call obj,$(NAMETAG_OBJECTS))
	$(call link_lib,$@,$^)

C_FLAGS += $(call include_dir,.)
# executables
$(call exe,rest_server/nametag_server): LD_FLAGS+=$(call use_library,$(if $(filter win-%,$(PLATFORM)),$(MICRORESTD_LIBRARIES_WIN),$(MICRORESTD_LIBRARIES_POSIX)))
$(call exe,rest_server/nametag_server): $(call obj,$(NAMETAG_OBJECTS) rest_server/nametag_service $(addprefix rest_server/microrestd/,$(MICRORESTD_OBJECTS)))
$(call exe,run_ner): $(call obj, $(NAMETAG_OBJECTS))
$(call exe,run_tokenizer): $(call obj, $(NAMETAG_OBJECTS))
$(call exe,train_ner): $(call obj, $(NAMETAG_OBJECTS) classifier/network_classifier_encoder features/feature_templates_encoder ner/bilou_ner_trainer ner/entity_map_encoder utils/compressor_save)
$(EXECUTABLES) $(SERVER):$(call exe,%): $$(call obj,% utils/options)
	$(call link_exe,$@,$^,$(call win_subsystem,console))

# cleaning
.PHONY: clean
clean:
	@$(call rm,.build $(call all_exe,$(EXECUTABLES) $(SERVER)) $(call all_lib,$(LIBRARIES)))

# dump library sources
.PHONY: lib_sources
lib_sources:
	@echo $(patsubst %,%.cpp,$(NAMETAG_OBJECTS))
