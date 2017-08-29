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
#include "bilou/bilou_entity.h"
#include "classifier/network_classifier.h"
#include "entity_map.h"
#include "features/feature_templates.h"
#include "ner.h"
#include "ner_ids.h"
#include "tagger/tagger.h"
#include "tokenizer/tokenizer.h"
#include "utils/threadsafe_stack.h"

namespace ufal {
namespace nametag {

class bilou_ner : public ner {
 public:
  bilou_ner(ner_id id);

  bool load(istream& is);

  virtual void recognize(const vector<string_piece>& forms, vector<named_entity>& entities) const override;
  virtual tokenizer* new_tokenizer() const override;

  virtual void entity_types(vector<string>& types) const override;

  virtual void gazetteers(vector<string>& gazetteers, vector<int>* gazetteer_types) const override;
 private:
  friend class bilou_ner_trainer;

  // Methods used by bylou_ner_trainer
  static void fill_bilou_probabilities(const vector<double>& outcomes, bilou_probabilities& prob);
  static tokenizer* new_tokenizer(ner_id id);

  // Internal members of bilou_ner
  ner_id id;
  unique_ptr<ufal::nametag::tagger> tagger;
  entity_map named_entities;
  feature_templates templates;
  vector<network_classifier> networks;

  struct cache {
    ner_sentence sentence;
    vector<double> outcomes, network_buffer;
    string string_buffer;
    vector<named_entity> entities_buffer;
  };
  mutable threadsafe_stack<cache> caches;
};

} // namespace nametag
} // namespace ufal
