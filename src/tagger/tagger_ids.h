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
