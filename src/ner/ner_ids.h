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

namespace ufal {
namespace nametag {

class ner_ids {
 public:
  enum ner_id { CZECH_NER = 0, ENGLISH_NER = 1, GENERIC_NER = 2 };

  static bool parse(const string& str, ner_id& id) {
    if (str == "czech") return id = CZECH_NER, true;
    if (str == "english") return id = ENGLISH_NER, true;
    if (str == "generic") return id = GENERIC_NER, true;
    return false;
  }
};

typedef ner_ids::ner_id ner_id;

} // namespace nametag
} // namespace ufal
