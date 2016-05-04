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

class tagger_ids {
 public:
  enum tagger_id { TRIVIAL, EXTERNAL, MORPHODITA };

  static bool parse(const string& str, tagger_id& id) {
    if (str == "trivial") return id = TRIVIAL, true;
    if (str == "external") return id = EXTERNAL, true;
    if (str == "morphodita") return id = MORPHODITA, true;
    return false;
  }
};

typedef tagger_ids::tagger_id tagger_id;

} // namespace nametag
} // namespace ufal
