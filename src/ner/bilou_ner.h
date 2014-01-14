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
#include "bilou/bilou_entity.h"
#include "classifier/network_classifier.h"
#include "entity_map.h"
#include "features/feature_templates.h"
#include "ner.h"
#include "tagger/tagger.h"
#include "utils/threadsafe_stack.h"

namespace ufal {
namespace nametag {

class bilou_ner : public ner {
 public:
  bool load(FILE* f);

  virtual void recognize(const vector<string_piece>& forms, vector<named_entity>& entities) const override;

 private:
  friend class bilou_ner_trainer;

  // Methods used by bylou_ner_trainer
  static void fill_bilou_probabilities(const vector<double>& outcomes, bilou_probabilities& prob);

  // Internal members of bilou_ner
  unique_ptr<ufal::nametag::tagger> tagger;
  entity_map named_entities;
  struct stage_info {
    feature_templates templates;
    network_classifier network;
  };
  vector<stage_info> stages;

  struct cache {
    ner_sentence sentence;
    vector<double> outcomes;
    string string_buffer;
    vector<named_entity> entities_buffer;

    cache(const bilou_ner& self) : outcomes(bilou_entity::total(self.named_entities.size())) {}
  };
  mutable threadsafe_stack<cache> caches;
};

} // namespace nametag
} // namespace ufal
