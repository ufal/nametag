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
#include "bilou_type.h"
#include "entity_type.h"

namespace ufal {
namespace nametag {

class bilou_entity {
 public:
  typedef entity_type value;

  enum :value { I, L, O, B_first, U_first, unknown = ~0U };
  static constexpr inline value B(entity_type entity) { return entity == entity_type_unknown ? unknown : B_first + 2 * entity; }
  static constexpr inline value U(entity_type entity) { return entity == entity_type_unknown ? unknown : U_first + 2 * entity; }
  static constexpr inline value total(entity_type entities) { return B_first + 2 * entities; }

  static inline bilou_type get_bilou(value bilou_entity) {
    switch (bilou_entity) {
      case I: return bilou_type_I;
      case L: return bilou_type_L;
      case O: return bilou_type_O;
      default: return (bilou_entity - B_first) & 1 ? bilou_type_U : bilou_type_B;
    }
  }
  static inline entity_type get_entity(value bilou_entity) {
    switch (bilou_entity) {
      case I:
      case L:
      case O: return entity_type_unknown;
      default: return (bilou_entity - B_first) >> 1;
    }
  }
  static inline value from_bilou_entity(bilou_type bilou, entity_type entity) {
    switch (bilou) {
      case bilou_type_I: return I;
      case bilou_type_L: return L;
      case bilou_type_O: return O;
      case bilou_type_B: return B(entity);
      default: return U(entity);
    }
  }
};

} // namespace nametag
} // namespace ufal
