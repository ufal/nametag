// This file is part of NameTag <http://github.com/ufal/nametag/>.
//
// Copyright 2016 Institute of Formal and Applied Linguistics, Faculty of
// Mathematics and Physics, Charles University in Prague, Czech Republic.
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

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
