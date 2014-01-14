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
#include "tagger/tagger.h"

namespace ufal {
namespace nametag {

class bilou_ner_trainer {
 public:
  static void train(int stages, const network_parameters& parameters, const tagger& tagger, FILE* in_features, FILE* in_train, FILE* in_heldout, FILE* out_ner);

 private:
  struct labelled_sentence {
    ner_sentence sentence;
    vector<bilou_entity::value> outcomes;
  };

  static void load_data(FILE* f, const tagger& tagger, vector<labelled_sentence>& data, entity_map& entity_map, bool add_entities);
  static void generate_instances(vector<labelled_sentence>& data, const feature_templates& templates, vector<classifier_instance>& instances, bool add_features);
  static void compute_previous_stage(vector<labelled_sentence>& data, const feature_templates& templates, const network_classifier& network, vector<double>& outcomes);
};

} // namespace nametag
} // namespace ufal
