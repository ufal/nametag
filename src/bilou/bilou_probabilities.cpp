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

#include "bilou_probabilities.h"

namespace ufal {
namespace nametag {

void bilou_probabilities_global::init(const bilou_probabilities& local) {
  bilou = local.bilou;
  previous.fill(bilou_type_unknown);

  best = bilou_type_B;
  bilou[bilou_type_I].probability = 0;
  bilou[bilou_type_L].probability = 0;
  if (bilou[bilou_type_O].probability > bilou[best].probability) best = bilou_type_O;
  if (bilou[bilou_type_U].probability > bilou[best].probability) best = bilou_type_U;
}

void bilou_probabilities_global::update(const bilou_probabilities& local, const bilou_probabilities_global& prev) {
  // Find the best of previous LOU
  bilou_type best_LOU = bilou_type_L;
  double best_LOU_prob = prev.bilou[bilou_type_L].probability;
  if (prev.bilou[bilou_type_O].probability > best_LOU_prob) {
    best_LOU = bilou_type_O;
    best_LOU_prob = prev.bilou[bilou_type_O].probability;
  }
  if (prev.bilou[bilou_type_U].probability > best_LOU_prob) {
    best_LOU = bilou_type_U;
    best_LOU_prob = prev.bilou[bilou_type_U].probability;
  }

  // Find the best of previous BI
  bilou_type best_BI = bilou_type_B;
  double best_BI_prob = prev.bilou[bilou_type_B].probability;
  if (prev.bilou[bilou_type_I].probability > best_BI_prob) {
    best_BI = bilou_type_I;
    best_BI_prob = prev.bilou[bilou_type_I].probability;
  }

  // Normalize the best_*_prob probabilities
  if (best_BI_prob > best_LOU_prob) {
    best_LOU_prob /= best_BI_prob;
    best_BI_prob = 1;
  } else {
    best_BI_prob /= best_LOU_prob;
    best_LOU_prob = 1;
  }

  // Store update probabilites
  bilou[bilou_type_B].probability = best_LOU_prob * local.bilou[bilou_type_B].probability;
  bilou[bilou_type_B].entity = local.bilou[bilou_type_B].entity;
  previous[bilou_type_B] = best_LOU;
  best = bilou_type_B;

  bilou[bilou_type_I].probability = best_BI_prob * local.bilou[bilou_type_I].probability;
  bilou[bilou_type_I].entity = prev.bilou[best_BI].entity;
  previous[bilou_type_I] = best_BI;
  if (bilou[bilou_type_I].probability > bilou[best].probability) best = bilou_type_I;

  bilou[bilou_type_L].probability = best_BI_prob * local.bilou[bilou_type_L].probability;
  bilou[bilou_type_L].entity = prev.bilou[best_BI].entity;
  previous[bilou_type_L] = best_BI;
  if (bilou[bilou_type_L].probability > bilou[best].probability) best = bilou_type_L;

  bilou[bilou_type_O].probability = best_LOU_prob * local.bilou[bilou_type_O].probability;
  bilou[bilou_type_O].entity = local.bilou[bilou_type_O].entity;
  previous[bilou_type_O] = best_LOU;
  if (bilou[bilou_type_O].probability > bilou[best].probability) best = bilou_type_O;

  bilou[bilou_type_U].probability = best_LOU_prob * local.bilou[bilou_type_U].probability;
  bilou[bilou_type_U].entity = local.bilou[bilou_type_U].entity;
  previous[bilou_type_U] = best_LOU;
  if (bilou[bilou_type_U].probability > bilou[best].probability) best = bilou_type_U;
}

} // namespace nametag
} // namespace ufal
