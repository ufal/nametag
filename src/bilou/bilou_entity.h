// This file is part of NameTag <http://github.com/ufal/nametag/>.
//
// Copyright 2016 Institute of Formal and Applied Linguistics, Faculty of
// Mathematics and Physics, Charles University in Prague, Czech Republic.
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

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
