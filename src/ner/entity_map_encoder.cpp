// This file is part of NameTag <http://github.com/ufal/nametag/>.
//
// Copyright 2016 Institute of Formal and Applied Linguistics, Faculty of
// Mathematics and Physics, Charles University in Prague, Czech Republic.
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#include "entity_map.h"
#include "utils/binary_encoder.h"
#include "utils/compressor.h"

namespace ufal {
namespace nametag {

bool entity_map::save(ostream& os) const {
  binary_encoder enc;

  enc.add_4B(id2str.size());
  for (auto&& entity : id2str)
    enc.add_str(entity);

  return compressor::save(os, enc);
}

} // namespace nametag
} // namespace ufal
