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
#include "feature_processor.h"
#include "ner/entity_map.h"

namespace ufal {
namespace nametag {

class feature_templates {
 public:
  void parse(istream& is, entity_map& entities, const nlp_pipeline& pipeline);

  bool load(istream& is, const nlp_pipeline& pipeline);
  bool save(ostream& os);

  void process_sentence(ner_sentence& sentence, string& buffer, bool add_features = false) const;
  void process_entities(ner_sentence& sentence, vector<named_entity>& entities, vector<named_entity>& buffer) const;
  ner_feature get_total_features() const;

  void gazetteers(vector<string>& gazetteers, vector<int>* gazetteer_types) const;

 private:
  mutable ner_feature total_features;

  struct feature_processor_info {
    string name;
    unique_ptr<feature_processor> processor;

    feature_processor_info(const string& name, feature_processor* processor) : name(name), processor(processor) {}
  };
  vector<feature_processor_info> processors;
};

} // namespace nametag
} // namespace ufal
