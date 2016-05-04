// This file is part of NameTag <http://github.com/ufal/nametag/>.
//
// Copyright 2016 Institute of Formal and Applied Linguistics, Faculty of
// Mathematics and Physics, Charles University in Prague, Czech Republic.
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#include <algorithm>

#include "entity_processor.h"

namespace ufal {
namespace nametag {

// Feature template -- methods and virtual methods
entity_processor::~entity_processor() {}

bool entity_processor::parse(const vector<string>& /*args*/, entity_map& /*entities*/) {
  return true;
}

void entity_processor::load(binary_decoder& /*data*/) {
}

void entity_processor::save(binary_encoder& /*enc*/) {
}

} // namespace nametag
} // namespace ufal
