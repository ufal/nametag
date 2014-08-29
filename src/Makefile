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

# object files groups
MORPHODITA_O = morphodita/utils/lzma/morphodita_LzmaDec morphodita/utils/morphodita_compressor_load morphodita/version/morphodita_version morphodita/tagset_converter/morphodita_identity_tagset_converter morphodita/tagset_converter/morphodita_strip_lemma_id_tagset_converter morphodita/tagset_converter/morphodita_tagset_converter morphodita/tagset_converter/morphodita_strip_lemma_comment_tagset_converter morphodita/tagset_converter/morphodita_pdt_to_conll2009_tagset_converter morphodita/tagger/morphodita_tagger morphodita/morpho/morphodita_czech_morpho morphodita/morpho/morphodita_generic_morpho morphodita/morpho/morphodita_tag_filter morphodita/morpho/morphodita_english_morpho morphodita/morpho/morphodita_morpho morphodita/morpho/morphodita_english_morpho_guesser morphodita/morpho/morphodita_external_morpho morphodita/morpho/morphodita_morpho_statistical_guesser morphodita/tokenizer/morphodita_utf8_tokenizer morphodita/tokenizer/morphodita_tokenizer morphodita/tokenizer/morphodita_vertical_tokenizer morphodita/tokenizer/morphodita_english_tokenizer morphodita/tokenizer/morphodita_generic_tokenizer morphodita/tokenizer/morphodita_czech_tokenizer morphodita/unilib/morphodita_unicode morphodita/unilib/morphodita_utf8
LZMA_DEC_O = utils/compressor_load utils/lzma/LzmaDec
LZMA_ENC_O = utils/compressor_save utils/lzma/LzmaEnc utils/lzma/LzFind
NAMETAG_O = $(MORPHODITA_O) $(LZMA_DEC_O) bilou/bilou_probabilities bilou/ner_sentence classifier/network_classifier features/entity_processor features/entity_processor_instances features/feature_templates features/sentence_processor features/sentence_processor_instances ner/bilou_ner ner/czech_ner ner/english_ner ner/entity_map ner/generic_ner ner/ner tagger/external_tagger tagger/morphodita_tagger tagger/tagger tagger/trivial_tagger tokenizer/tokenizer utils/distribution utils/input utils/url_detector version/version

# libraries
$(call lib,libnametag): $(call obj, $(NAMETAG_O))
	ar -rcs $@ $^
$(call lib,libnametag.dyn): $(call dynobj, $(NAMETAG_O))
	ar -rcs $@ $^
$(call dynlib,libnametag): $(call dynobj,$(NAMETAG_O))
	$(CXX) -o $@ $(DYN_LD_FLAGS) $^ $(call version_script,libnametag.map) $(call create_implib,libnametag)

# executables
$(call exe,run_ner): $(call obj, $(NAMETAG_O) utils/input utils/output utils/parse_options)
$(call exe,run_tokenizer): $(call obj, $(NAMETAG_O) utils/input utils/output utils/parse_options)
$(call exe,train_ner): $(call obj, $(LZMA_ENC_O) $(NAMETAG_O) classifier/network_classifier_encoder features/feature_templates_encoder ner/bilou_ner_trainer ner/entity_map_encoder utils/input utils/parse_options)
$(EXECUTABLES): $(call exe,%): $(call obj,%)
	$(CXX) $^ -o $@ $(LD_FLAGS)

# cleaning
.PHONY: clean veryclean

clean:
	@$(call rmdir,.objs) $(call rm,$(EXECUTABLES) $(DYNAMIC_LIBRARIES) $(STATIC_LIBRARIES) $(call implib,libnametag))

# dump library sources
lib_sources:
	@echo $(patsubst %,%.cpp,$(NAMETAG_O))
