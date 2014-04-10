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
