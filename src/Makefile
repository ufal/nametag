MORPHODITA_DIR=../morphodita/src

include $(MORPHODITA_DIR)/Makefile.options

EXECUTABLES = $(call exe,run_ner train_ner)
LIBRARIES = $(call lib,libnametag libnametag.dyn) $(call dynlib,libnametag)
all: $(EXECUTABLES)
lib: $(LIBRARIES)

# options
C_FLAGS += -I$(MORPHODITA_DIR)/include
ifdef WIN
  C_FLAGS += -DMORPHODITA_STATIC
endif

# dependency and compilation rules
include $(MORPHODITA_DIR)/Makefile.rules

# object files groups
LZMA_DEC_O = utils/compressor_load utils/lzma/LzmaDec
LZMA_ENC_O = utils/compressor_save utils/lzma/LzmaEnc utils/lzma/LzFind
NAMETAG_O = $(LZMA_DEC_O) bilou/bilou_probabilities bilou/ner_sentence classifier/network_classifier features/entity_processor features/entity_processor_instances features/feature_templates features/sentence_processor features/sentence_processor_instances ner/bilou_ner ner/czech_ner ner/entity_map ner/ner tagger/external_tagger tagger/morphodita_tagger tagger/tagger tagger/trivial_tagger utils/distribution utils/input utils/url_detector utils/utf8

# libraries
$(MORPHODITA_DIR)/libmorphodita%:
	$(MAKE) -C $(@D) $(@F)
$(call lib,libnametag): $(call obj, $(NAMETAG_O))
	ar -rcs $@ $^
$(call lib,libnametag.dyn): $(call dynobj, $(NAMETAG_O))
	ar -rcs $@ $^
$(call dynlib,libnametag): $(call lib,libnametag.dyn) $(call lib,$(MORPHODITA_DIR)/libmorphodita$(if $(WIN),,.dyn))
	$(CXX) -o $@ $(DYN_LD_FLAGS) -Wl,--whole-archive $^ -Wl,-no-whole-archive -Wl,--version-script=libnametag.map $(call create_implib,libnametag)

# executables
$(call exe,run_ner): $(call obj, $(NAMETAG_O) utils/input utils/output) $(call lib,$(MORPHODITA_DIR)/libmorphodita)
$(call exe,train_ner): $(call obj, $(LZMA_ENC_O) $(NAMETAG_O) classifier/network_classifier_encoder features/feature_templates_encoder ner/bilou_ner_trainer ner/entity_map_encoder utils/input) $(call lib,$(MORPHODITA_DIR)/libmorphodita)
$(EXECUTABLES): $(call exe,%): $(call obj,%)
	$(CXX) $^ -o $@ $(LD_FLAGS)

# cleaning
.PHONY: clean veryclean

clean:
	@$(call rmdir,.objs) $(call rm,$(LIBRARIES) $(EXECUTABLES) $(call implib,libnametag))

veryclean: clean
	$(MAKE) -C $(MORPHODITA_DIR) clean
