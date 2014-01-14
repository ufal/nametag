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
#include "ner/ner.h"
#include "utils/binary_decoder.h"
#include "utils/binary_encoder.h"

namespace ufal {
namespace nametag {

class entity_processor {
 public:
  virtual ~entity_processor();

  virtual bool parse(const vector<string>& args, entity_map& entities);
  virtual void load(binary_decoder& data);
  virtual void save(binary_encoder& enc);

  virtual void process_entities(ner_sentence& sentence, vector<named_entity>& entities, vector<named_entity>& buffer) const = 0;

  // Factory method
 public:
  static entity_processor* create(const string& name);
};

} // namespace nametag
} // namespace ufal
