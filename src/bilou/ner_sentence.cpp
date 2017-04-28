// This file is part of NameTag <http://github.com/ufal/nametag/>.
//
// Copyright 2016 Institute of Formal and Applied Linguistics, Faculty of
// Mathematics and Physics, Charles University in Prague, Czech Republic.
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#include <algorithm>

#include "ner_sentence.h"

namespace ufal {
namespace nametag {

void ner_sentence::resize(unsigned size) {
  this->size = size;
  if (words.size() < size) words.resize(size);
  if (features.size() < size) features.resize(size);
  if (probabilities.size() < size) probabilities.resize(size);
  if (previous_stage.size() < size) previous_stage.resize(size);
}

void ner_sentence::clear_features() {
  for (unsigned i = 0; i < size; i++)
    features[i].clear();
}

void ner_sentence::clear_probabilities_local_filled() {
  for (unsigned i = 0; i < size; i++)
      probabilities[i].local_filled = false;
}

void ner_sentence::clear_previous_stage() {
  for (unsigned i = 0; i < size; i++) {
    previous_stage[i].bilou = bilou_type_unknown;
    previous_stage[i].entity = entity_type_unknown;
  }
}

void ner_sentence::compute_best_decoding() {
  // Find best L O U bilou_type for the last word
  bilou_type best = bilou_type_L;
  auto& last_bilou = probabilities[size - 1].global.bilou;
  if (last_bilou[bilou_type_O].probability > last_bilou[best].probability) best = bilou_type_O;
  if (last_bilou[bilou_type_U].probability > last_bilou[best].probability) best = bilou_type_U;
  probabilities[size - 1].global.best = best;

  // Store the best bilou_type for all preceeding words
  for (unsigned i = size - 1; i; i--) {
    best = probabilities[i].global.previous[best];
    probabilities[i - 1].global.best = best;
  }
}

void ner_sentence::fill_previous_stage() {
  for (unsigned i = 0; i < size; i++) {
    previous_stage[i].bilou = probabilities[i].global.best;
    previous_stage[i].entity = probabilities[i].global.bilou[probabilities[i].global.best].entity;
  }
}

} // namespace nametag
} // namespace ufal
