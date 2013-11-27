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

#include "entity_map.h"
#include "utils/binary_encoder.h"
#include "utils/compressor.h"

namespace ufal {
namespace nametag {

bool entity_map::save(FILE *f) const {
  binary_encoder enc;

  enc.add_4B(id2str.size());
  for (auto& entity : id2str) {
    enc.add_1B(entity.size());
    enc.add_str(entity);
  }

  return compressor::save(f, enc);
}

} // namespace nametag
} // namespace ufal
