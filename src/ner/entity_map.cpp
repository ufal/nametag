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

#include "entity_map.h"
#include "utils/binary_decoder.h"
#include "utils/compressor.h"

namespace ufal {
namespace nametag {

entity_type entity_map::parse(const char* str, bool add_entity) const {
  auto it = str2id.find(str);
  if (it == str2id.end() && add_entity) {
    it = str2id.emplace(str, id2str.size()).first;
    id2str.emplace_back(str);
  }
  return it == str2id.end() ? entity_type_unknown : it->second;
}

const string& entity_map::name(entity_type entity) const {
  return entity < id2str.size() ? id2str[entity] : empty;
}

bool entity_map::load(istream& is) {
  binary_decoder data;
  if (!compressor::load(is, data)) return false;

  try {
    str2id.clear();
    id2str.resize(data.next_4B());
    for (unsigned i = 0; i < id2str.size(); i++) {
      data.next_str(id2str[i]);
      str2id.emplace(id2str[i], i);
    }
  } catch (binary_decoder_error&) {
    return false;
  }

  return data.is_end();
}

entity_type entity_map::size() const {
  return id2str.size();
}

} // namespace nametag
} // namespace ufal
