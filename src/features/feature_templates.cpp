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

bool feature_templates::load(istream& is, const nlp_pipeline& pipeline) {
  binary_decoder data;
  if (!compressor::load(is, data)) return false;

  try {
    total_features = data.next_4B();

    processors.clear();
    for (unsigned i = data.next_4B(); i; i--) {
      string name;
      data.next_str(name);

      // Try creating the processor
      auto* processor = feature_processor::create(name);
      if (processor) {
        processor->load(data, pipeline);
        processors.emplace_back(name, processor);
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

  // Add features from feature processors
  for (auto&& processor : processors)
    processor.processor->process_sentence(sentence, adding_features ? &total_features : nullptr, buffer);
}

void feature_templates::process_entities(ner_sentence& sentence, vector<named_entity>& entities, vector<named_entity>& buffer) const {
  for (auto&& processor : processors)
    processor.processor->process_entities(sentence, entities, buffer);
}

ner_feature feature_templates::get_total_features() const {
  return total_features;
}

void feature_templates::gazetteers(vector<string>& gazetteers, vector<int>* gazetteer_types) const {
  for (auto&& processor : processors)
    processor.processor->gazetteers(gazetteers, gazetteer_types);
}

} // namespace nametag
} // namespace ufal
