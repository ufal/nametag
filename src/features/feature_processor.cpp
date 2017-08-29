// This file is part of NameTag <http://github.com/ufal/nametag/>.
//
// Copyright 2016 Institute of Formal and Applied Linguistics, Faculty of
// Mathematics and Physics, Charles University in Prague, Czech Republic.
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#include <algorithm>

#include "feature_processor.h"

namespace ufal {
namespace nametag {

// Feature processor -- methods and virtual methods
feature_processor::~feature_processor() {}

bool feature_processor::parse(int window, const vector<string>& /*args*/, entity_map& /*entities*/,
                              ner_feature* total_features, const nlp_pipeline& /*pipeline*/) {
  if (window < 0) return false;
  if (!total_features) return false;

  this->window = window;

  map.clear();
  lookup(string(), total_features); // Always add an empty string to the map

  return true;
}

void feature_processor::load(binary_decoder& data, const nlp_pipeline& /*pipeline*/) {
  window = data.next_4B();

  map.clear();
  map.rehash(data.next_4B());
  for (unsigned i = data.next_4B(); i > 0; i--) {
    string key;
    data.next_str(key);
    map.emplace(key, data.next_4B());
  }
}

void feature_processor::save(binary_encoder& enc) {
  enc.add_4B(window);

  enc.add_4B(map.bucket_count());
  enc.add_4B(map.size());

  vector<pair<string, ner_feature>> map_elements(map.begin(), map.end());
  sort(map_elements.begin(), map_elements.end());
  for (auto&& element : map_elements) {
    enc.add_str(element.first);
    enc.add_4B(element.second);
  }
}

void feature_processor::process_sentence(ner_sentence& /*sentence*/, ner_feature* /*total_features*/, string& /*buffer*/) const {}

void feature_processor::process_entities(ner_sentence& /*sentence*/, vector<named_entity>& /*entities*/, vector<named_entity>& /*buffer*/) const {}

void feature_processor::gazetteers(vector<string>& /*gazetteers*/, vector<int>* /*gazetteer_types*/) const {}

} // namespace nametag
} // namespace ufal
