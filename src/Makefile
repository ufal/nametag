MORPHODITA_DIR=../morphodita/src

include $(MORPHODITA_DIR)/Makefile.options

EXECUTABLES = $(call exe,run_ner train_ner)
LIBRARIES = $(call lib,libnametag libnametag.dyn) $(call dynlib,libnametag)
all: $(EXECUTABLES)
lib: $(LIBRARIES)

# options
C_FLAGS += -I$(MORPHODITA_DIR)/include -DMORPHODITA_STATIC

# dependency and compilation rules
include $(MORPHODITA_DIR)/Makefile.rules

# object files groups
LZMA_DEC_O = utils/compressor_load utils/lzma/LzmaDec
LZMA_ENC_O = utils/compressor_save utils/lzma/LzmaEnc utils/lzma/LzFind
NAMETAG_O = $(LZMA_DEC_O) classifier/network_classifier features/entity_processor features/entity_processor_instances features/feature_templates features/form_processor features/form_processor_instances features/sentence_processor features/sentence_processor_instances ner/bilou_ner ner/bilou_probabilities ner/entity_map ner/ner tagger/external_tagger tagger/morphodita_tagger tagger/tagger tagger/trivial_tagger utils/distribution utils/input utils/utf8

# libraries
$(MORPHODITA_DIR)/libmorphodita%:
	make -C $(@D) $(@F)
$(call lib,libnametag): $(call lib,$(MORPHODITA_DIR)/libmorphodita) $(call obj, $(NAMETAG_O))
	@$(foreach obj,$(filter-out $<,$^),cp $(obj) $(call libobj,nametag,$(obj));)
	@cp $< $@
	ar -rcs $@ $(call libobj,nametag,$(filter-out $<,$^))
	@rm -r $(call libobj,nametag,$(filter-out $<,$^))
$(call lib,libnametag.dyn): $(call lib,$(MORPHODITA_DIR)/libmorphodita.dyn) $(call dynobj, $(NAMETAG_O))
	@$(foreach obj,$(filter-out $<,$^),cp $(obj) $(call libobj,nametag,$(obj));)
	@cp $< $@
	ar -rcs $@ $(call libobj,nametag,$(filter-out $<,$^))
	@rm -r $(call libobj,nametag,$(filter-out $<,$^))
$(call dynlib,libnametag): $(call lib,libnametag.dyn)
	$(CXX) -o $@ $(DYN_LD_FLAGS) -Wl,--whole-archive $< -Wl,-no-whole-archive -Wl,--version-script=libnametag.map

# executables
$(call exe,run_ner): $(call obj, $(NAMETAG_O) utils/input) $(call lib,$(MORPHODITA_DIR)/libmorphodita)
$(call exe,train_ner): $(call obj, $(LZMA_ENC_O) $(NAMETAG_O) classifier/network_classifier_encoder features/feature_templates_encoder ner/bilou_ner_trainer ner/entity_map_encoder utils/input) $(call lib,$(MORPHODITA_DIR)/libmorphodita)
$(EXECUTABLES): $(call exe,%): $(call obj,%)
	$(CXX) $^ -o $@ $(LD_FLAGS)

# cleaning
.PHONY: clean veryclean

clean:
	rm -rf .objs/ $(LIBRARIES) $(EXECUTABLES) $(if $(WIN),$(call lib,$(call dynlib,libnametag)))

veryclean: clean
	make -C $(MORPHODITA_DIR) clean
