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
#include "entity_processor.h"
#include "sentence_processor.h"
#include "ner/entity_map.h"

namespace ufal {
namespace nametag {

class feature_templates {
 public:
  void parse(istream& is, entity_map& entities);

  bool load(istream& is);
  bool save(ostream& os);

  void process_sentence(ner_sentence& sentence, string& buffer, bool add_features = false) const;
  void process_entities(ner_sentence& sentence, vector<named_entity>& entities, vector<named_entity>& buffer) const;
  ner_feature get_total_features() const;

 private:
  mutable ner_feature total_features;

  struct sentence_processor_info {
    string name;
    unique_ptr<sentence_processor> processor;

    sentence_processor_info(const string& name, sentence_processor* processor) : name(name), processor(processor) {}
  };
  vector<sentence_processor_info> sentence_processors;

  struct entity_processor_info {
    string name;
    unique_ptr<entity_processor> processor;

    entity_processor_info(const string& name, entity_processor* processor) : name(name), processor(processor) {}
  };
  vector<entity_processor_info> entity_processors;
};

} // namespace nametag
} // namespace ufal
