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
