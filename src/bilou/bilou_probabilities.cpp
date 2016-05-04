// This file is part of NameTag <http://github.com/ufal/nametag/>.
//
// Copyright 2016 Institute of Formal and Applied Linguistics, Faculty of
// Mathematics and Physics, Charles University in Prague, Czech Republic.
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

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
