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

#include <unordered_map>

#include "common.h"
#include "entity_type.h"

namespace ufal {
namespace nametag {

typedef unsigned bilou_type;
enum :bilou_type { bilou_type_B, bilou_type_I, bilou_type_L, bilou_type_O, bilou_type_U, bilou_type_total, bilou_type_unknown = ~0U };

} // namespace nametag
} // namespace ufal
