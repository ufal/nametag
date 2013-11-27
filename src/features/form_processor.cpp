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

#include <algorithm>
#include <memory>

#include "form_processor.h"

namespace ufal {
namespace nametag {

// Form processor -- factory
form_processor::factory_map& form_processor::factory() {
  static factory_map map;
  return map;
}

form_processor* form_processor::create(const string& name) {
  auto it = factory().find(name);
  return it != factory().end() ? it->second() : nullptr;
}

form_processor* form_processor::load_instance(binary_decoder& data) {
  unsigned len = data.next_1B();
  unique_ptr<form_processor> res(create(string(data.next<char>(len), len)));
  if (!res) return nullptr;

  res->load(data);
  return res.release();
}

void form_processor::save_instance(binary_encoder& enc) {
  enc.add_1B(name().size());
  enc.add_str(name());
  save(enc);
}

// Form processor -- methods and virtual methods
form_processor::~form_processor() {}

bool form_processor::init(int window, const vector<string>& /*args*/) {
  if (window < 0) return false;

  this->window = window;

  map.clear();
  adding_features = true;
  lookup(string()); // Always add an empty string to the map

  return true;
}

void form_processor::load(binary_decoder& data) {
  window = data.next_4B();

  map.clear();
  map.rehash(data.next_4B());
  for (unsigned i = data.next_4B(); i > 0; i--) {
    unsigned len = data.next_1B();
    string key(data.next<char>(len), len);
    map.emplace(key, data.next_4B());
  }
}

void form_processor::save(binary_encoder& enc) {
  enc.add_4B(window);

  enc.add_4B(map.bucket_count());
  enc.add_4B(map.size());

  vector<pair<string, ner_feature>> map_elements(map.begin(), map.end());
  sort(map_elements.begin(), map_elements.end());
  for (auto& element : map_elements) {
    enc.add_1B(element.first.size());
    enc.add_str(element.first);
    enc.add_4B(element.second);
  }
}

ner_feature form_processor::freeze(entity_map& /*entities*/) {
  adding_features = false;
  return window * map.size();
}

} // namespace nametag
} // namespace ufal
