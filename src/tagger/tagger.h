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
#include "tagger_ids.h"
#include "utils/string_piece.h"

namespace ufal {
namespace nametag {

class tagger {
 public:
  virtual ~tagger() {}

  virtual void tag(const vector<string_piece>& forms, ner_sentence& sentence) const = 0;

  // Factory methods
  static tagger* load_instance(istream& is);
  static tagger* create_and_encode_instance(const string& tagger_id_and_params, ostream& os);

 protected:
  virtual bool load(istream& is) = 0;
  virtual bool create_and_encode(const string& params, ostream& os) = 0;

 private:
  static tagger* create(tagger_id id);
};

} // namespace nametag
} // namespace ufal
