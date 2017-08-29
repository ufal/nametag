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
#include "ner/ner_ids.h"
#include "tagger/tagger.h"

namespace ufal {
namespace nametag {

class bilou_ner_trainer {
 public:
  static void train(ner_id id, int stages, const network_parameters& parameters, const tagger& tagger,
                    istream& features, istream& train, istream& heldout, ostream& os);

 private:
  struct labelled_sentence {
    ner_sentence sentence;
    vector<bilou_entity::value> outcomes;
  };

  static void load_data(istream& is, const tagger& tagger, vector<labelled_sentence>& data, entity_map& entity_map, bool add_entities);
  static void generate_instances(vector<labelled_sentence>& data, const feature_templates& templates, vector<classifier_instance>& instances, bool add_features);
  static void compute_previous_stage(vector<labelled_sentence>& data, const feature_templates& templates, const network_classifier& network);
};

} // namespace nametag
} // namespace ufal
