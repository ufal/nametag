// This file is part of NameTag <http://github.com/ufal/nametag/>.
//
// Copyright 2016 Institute of Formal and Applied Linguistics, Faculty of
// Mathematics and Physics, Charles University in Prague, Czech Republic.
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

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
