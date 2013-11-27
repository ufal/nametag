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
#include "ner_feature.h"
#include "ner_word.h"
#include "ner/bilou_probabilities.h"

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

  void resize(unsigned size) {
    this->size = size;
    if (words.size() < size) words.resize(size);
    if (features.size() < size) features.resize(size);
    if (probabilities.size() < size) probabilities.resize(size);
  }
};

} // namespace nametag
} // namespace ufal

