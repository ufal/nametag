// This file is part of NameTag <http://github.com/ufal/nametag/>.
//
// Copyright 2016 Institute of Formal and Applied Linguistics, Faculty of
// Mathematics and Physics, Charles University in Prague, Czech Republic.
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

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
