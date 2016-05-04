// This file is part of NameTag <http://github.com/ufal/nametag/>.
//
// Copyright 2016 Institute of Formal and Applied Linguistics, Faculty of
// Mathematics and Physics, Charles University in Prague, Czech Republic.
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

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
