// This file is part of NameTag.
//
// Copyright 2013 by Institute of Formal and Applied Linguistics, Faculty of
// Mathematics and Physics, Charles University in Prague, Czech Republic.
//
// NameTag is free software: you can redistribute it and/or modify
// it under the terms of the GNU Lesser General Public License as
// published by the Free Software Foundation, either version 3 of
// the License, or (at your option) any later version.
//
// NameTag is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU Lesser General Public License for more details.
//
// You should have received a copy of the GNU Lesser General Public License
// along with NameTag.  If not, see <http://www.gnu.org/licenses/>.

#include "feature_templates.h"
#include "utils/compressor.h"
#include "utils/binary_decoder.h"

namespace ufal {
namespace nametag {

bool feature_templates::load(FILE* f) {
  binary_decoder data;
  if (!compressor::load(f, data)) return false;

  try {
    total_features = data.next_4B();

    sentence_processors.clear();
    entity_processors.clear();
    for (unsigned processors = data.next_4B(); processors; processors--) {
      unsigned name_len = data.next_1B();
      string name(data.next<char>(name_len), name_len);

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
  for (auto&& processor : entity_processors)
    processor.processor->process_entities(sentence, entities, buffer);
}

ner_feature feature_templates::get_total_features() const {
  return total_features;
}

} // namespace nametag
} // namespace ufal
