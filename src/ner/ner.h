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
#include "tokenizer/tokenizer.h"

namespace ufal {
namespace nametag {

struct named_entity {
  size_t start;
  size_t length;
  string type;

  named_entity() {}
  named_entity(size_t start, size_t length, const string& type) : start(start), length(length), type(type) {}
};

class ner {
 public:
  virtual ~ner() {}

  static ner* load(const char* fname);
  static ner* load(istream& is);

  // Perform named entity recognition on a tokenizes sentence and return found
  // named entities in the given vector.
  virtual void recognize(const vector<string_piece>& forms, vector<named_entity>& entities) const = 0;

  // Return the possible entity types
  virtual void entity_types(vector<string>& types) const = 0;

  // Return gazetteers used by the recognizer, if any, optionally with the index of entity type
  virtual void gazetteers(vector<string>& gazetteers, vector<int>* gazetteer_types) const = 0;

  // Construct a new tokenizer instance appropriate for this recognizer.
  // Can return NULL if no such tokenizer exists.
  virtual tokenizer* new_tokenizer() const = 0;
};

} // namespace nametag
} // namespace ufal
