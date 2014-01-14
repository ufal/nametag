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

#include "common.h"
#include "bilou_probabilities.h"
#include "features/ner_feature.h"
#include "ner_word.h"

namespace ufal {
namespace nametag {

struct ner_sentence {
  unsigned size = 0;
  vector<ner_word> words;
  vector<ner_features> features;

  struct probability_info {
    bilou_probabilities local;
    bool local_filled;
    bilou_probabilities_global global;
  };
  vector<probability_info> probabilities;

  struct previous_stage_info {
    bilou_type bilou;
    entity_type entity;
  };
  vector<previous_stage_info> previous_stage;

  void resize(unsigned size);
  void clear_features();
  void clear_probabilities_local_filled();
  void clear_previous_stage();

  void compute_best_decoding();
  void fill_previous_stage();
};

} // namespace nametag
} // namespace ufal

