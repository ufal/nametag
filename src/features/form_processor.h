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

#pragma once

#include "common.h"
#include "bilou/ner_sentence.h"
#include "ner/entity_map.h"
#include "utils/binary_decoder.h"
#include "utils/binary_encoder.h"

namespace ufal {
namespace nametag {

class form_processor {
 public:
  virtual ~form_processor();

  virtual bool parse(int window, const vector<string>& args, entity_map& entities, ner_feature* total_features);
  virtual void load(binary_decoder& data);
  virtual void save(binary_encoder& enc);

  virtual void process_form(int form, ner_sentence& sentence, ner_feature* total_features, string& buffer) const = 0;

 protected:
  int window;

  inline ner_feature lookup(const string& key, ner_feature* total_features) const {
    auto it = map.find(key);
    if (it == map.end() && total_features) {
      it = map.emplace(key, *total_features).first;
      *total_features += window;
    }
    return it != map.end() ? it->second : ner_feature_unknown;
  }

  mutable unordered_map<string, ner_feature> map;

  // Factory method
 public:
  static form_processor* create(const string& name);
};

} // namespace nametag
} // namespace ufal
