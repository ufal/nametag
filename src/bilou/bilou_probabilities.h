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

#include <array>

#include "common.h"
#include "bilou_type.h"
#include "entity_type.h"

namespace ufal {
namespace nametag {

class bilou_probabilities {
 public:
  struct probability_info {
    double probability;
    entity_type entity;
  };

  array<probability_info, bilou_type_total> bilou;
};

class bilou_probabilities_global : public bilou_probabilities {
 public:
  bilou_type best;
  array<bilou_type, bilou_type_total> previous;

  void init(const bilou_probabilities& local);
  void update(const bilou_probabilities& local, const bilou_probabilities_global& prev);
};

} // namespace nametag
} // namespace ufal
