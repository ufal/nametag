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

#include "entity_processor.h"

namespace ufal {
namespace nametag {

// Feature template -- factory
entity_processor::factory_map& entity_processor::factory() {
  static factory_map map;
  return map;
}

entity_processor* entity_processor::create(const string& name) {
  auto it = factory().find(name);
  return it != factory().end() ? it->second() : nullptr;
}

entity_processor* entity_processor::load_instance(binary_decoder& data) {
  unsigned len = data.next_1B();
  unique_ptr<entity_processor> res(create(string(data.next<char>(len), len)));
  if (!res) return nullptr;

  res->load(data);
  return res.release();
}

void entity_processor::save_instance(binary_encoder& enc) {
  enc.add_1B(name().size());
  enc.add_str(name());
  save(enc);
}

// Feature template -- methods and virtual methods
entity_processor::~entity_processor() {}

bool entity_processor::init(const vector<string>& /*args*/) {
  return true;
}

void entity_processor::load(binary_decoder& /*data*/) {
}

void entity_processor::save(binary_encoder& /*enc*/) {
}

} // namespace nametag
} // namespace ufal
