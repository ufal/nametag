// This file is part of NameTag <http://github.com/ufal/nametag/>.
//
// Copyright 2016 Institute of Formal and Applied Linguistics, Faculty of
// Mathematics and Physics, Charles University in Prague, Czech Republic.
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#pragma once

#include <unordered_map>

#include "common.h"
#include "bilou/entity_type.h"

namespace ufal {
namespace nametag {

class entity_map {
 public:
  entity_type parse(const char* str, bool add_entity = false) const;
  const string& name(entity_type entity) const;

  bool load(istream& is);
  bool save(ostream& os) const;

  entity_type size() const;
 private:
  mutable unordered_map<string, entity_type> str2id;
  mutable vector<string> id2str;
  string empty;
};

} // namespace nametag
} // namespace ufal
