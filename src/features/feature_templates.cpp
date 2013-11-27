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
    sentence_processors.resize(data.next_4B());
    for (auto& processor : sentence_processors) {
      processor.processor.reset(sentence_processor::load_instance(data));
      processor.offset = data.next_4B();
    }

    form_processors.resize(data.next_4B());
    for (auto& processor : form_processors) {
      processor.processor.reset(form_processor::load_instance(data));
      processor.offset = data.next_4B();
    }

    entity_processors.resize(data.next_4B());
    for (auto& processor : entity_processors)
      processor.reset(entity_processor::load_instance(data));
  } catch (binary_decoder_error&) {
    return false;
  }

  return data.is_end();
}

void feature_templates::process_sentence(ner_sentence& sentence, string& buffer) const {
  // Start with omnipresent feature
  for (unsigned i = 0; i < sentence.size; i++) {
    sentence.features[i].clear();
    sentence.features[i].emplace_back(0);
  }

  // Add features from given sentence processor templates
  for (auto& processor : sentence_processors)
    processor.processor->process_sentence(sentence, processor.offset, buffer);
}

void feature_templates::process_form(int form, ner_sentence& sentence, string& buffer) const {
  for (auto& processor : form_processors)
    processor.processor->process_form(form, sentence, processor.offset, buffer);
}

void feature_templates::process_entities(ner_sentence& sentence, vector<named_entity>& entities, vector<named_entity>& buffer) const {
  for (auto& processor : entity_processors)
    processor->process_entities(sentence, entities, buffer);
}

} // namespace nametag
} // namespace ufal
