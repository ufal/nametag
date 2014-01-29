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
#include "ner/string_piece.h"
#include "tagger_ids.h"

namespace ufal {
namespace nametag {

class tagger {
 public:
  virtual ~tagger() {}

  virtual void tag(const vector<string_piece>& forms, ner_sentence& sentence) const = 0;

  // Factory methods
  static tagger* load_instance(FILE* f);
  static tagger* create_and_encode_instance(const string& tagger_id_and_params, FILE* f);

 protected:
  virtual bool load(FILE* f) = 0;
  virtual bool create_and_encode(const string& params, FILE* f) = 0;

 private:
  static tagger* create(tagger_id id);
};

} // namespace nametag
} // namespace ufal
