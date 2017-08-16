// This file is part of NameTag <http://github.com/ufal/nametag/>.
//
// Copyright 2016 Institute of Formal and Applied Linguistics, Faculty of
// Mathematics and Physics, Charles University in Prague, Czech Republic.
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#include "feature_templates.h"
#include "utils/compressor.h"
#include "utils/binary_decoder.h"

namespace ufal {
namespace nametag {

bool feature_templates::load(istream& is) {
  binary_decoder data;
  if (!compressor::load(is, data)) return false;

  try {
    total_features = data.next_4B();

    sentence_processors.clear();
    entity_processors.clear();
    for (unsigned processors = data.next_4B(); processors; processors--) {
      string name;
      data.next_str(name);

      // Try sentence processor
      auto* maybe_sentence_processor = sentence_processor::create(name);
      if (maybe_sentence_processor) {
        maybe_sentence_processor->load(data);
        sentence_processors.emplace_back(name, maybe_sentence_processor);
        continue;
      }

      // Try entity processor
      auto* maybe_entity_processor = entity_processor::create(name);
      if (maybe_entity_processor) {
        maybe_entity_processor->load(data);
        entity_processors.emplace_back(name, maybe_entity_processor);
        continue;
      }

      // Could not find processor with specified name
      return false;
    }
  } catch (binary_decoder_error&) {
    return false;
  }

  return data.is_end();
}

void feature_templates::process_sentence(ner_sentence& sentence, string& buffer, bool adding_features) const {
  // Start with omnipresent feature
  for (unsigned i = 0; i < sentence.size; i++) {
    sentence.features[i].clear();
    sentence.features[i].emplace_back(0);
  }

  // Add features from given sentence processor templates
  for (auto&& processor : sentence_processors)
    processor.processor->process_sentence(sentence, adding_features ? &total_features : nullptr, buffer);
}

void feature_templates::process_entities(ner_sentence& sentence, vector<named_entity>& entities, vector<named_entity>& buffer) const {
  for (auto&& processor : sentence_processors)
    processor.processor->process_entities(sentence, entities, buffer);

  for (auto&& processor : entity_processors)
    processor.processor->process_entities(sentence, entities, buffer);
}

ner_feature feature_templates::get_total_features() const {
  return total_features;
}

} // namespace nametag
} // namespace ufal
