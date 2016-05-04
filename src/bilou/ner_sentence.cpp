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
    features.clear();
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
