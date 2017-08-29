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
#include "bilou/ner_sentence.h"
#include "ner/entity_map.h"
#include "ner/ner.h"
#include "nlp_pipeline.h"
#include "utils/binary_decoder.h"
#include "utils/binary_encoder.h"

namespace ufal {
namespace nametag {

class feature_processor {
 public:
  virtual ~feature_processor();

  virtual bool parse(int window, const vector<string>& args, entity_map& entities,
                     ner_feature* total_features, const nlp_pipeline& pipeline);
  virtual void load(binary_decoder& data, const nlp_pipeline& pipeline);
  virtual void save(binary_encoder& enc);

  virtual void process_sentence(ner_sentence& sentence, ner_feature* total_features, string& buffer) const;
  virtual void process_entities(ner_sentence& sentence, vector<named_entity>& entities, vector<named_entity>& buffer) const;

  virtual void gazetteers(vector<string>& gazetteers, vector<int>* gazetteer_types) const;

 protected:
  int window;

  inline ner_feature lookup(const string& key, ner_feature* total_features) const {
    auto it = map.find(key);
    if (it == map.end() && total_features) {
      it = map.emplace(key, window + *total_features).first;
      *total_features += 2*window + 1;
    }
    return it != map.end() ? it->second : ner_feature_unknown;
  }

  mutable unordered_map<string, ner_feature> map;

  // Factory method
 public:
  static feature_processor* create(const string& name);
};

} // namespace nametag
} // namespace ufal
