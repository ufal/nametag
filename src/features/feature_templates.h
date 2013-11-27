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

#include <memory>

#include "common.h"
#include "entity_processor.h"
#include "form_processor.h"
#include "sentence_processor.h"
#include "ner/entity_map.h"

namespace ufal {
namespace nametag {

class feature_templates {
 public:
  void parse(FILE* f);
  ner_feature freeze(entity_map& entities);

  bool load(FILE* f);
  bool save(FILE* f);

  void process_sentence(ner_sentence& sentence, string& buffer) const;
  void process_form(int form, ner_sentence& sentence, string& buffer) const;
  void process_entities(ner_sentence& sentence, vector<named_entity>& entities, vector<named_entity>& buffer) const;

 private:
  struct sentence_processor_info {
    unique_ptr<sentence_processor> processor;
    ner_feature offset;

    sentence_processor_info(sentence_processor* processor = nullptr, ner_feature offset = 0) : processor(processor), offset(offset) {}
  };
  vector<sentence_processor_info> sentence_processors;

  struct form_processor_info {
    unique_ptr<form_processor> processor;
    ner_feature offset;

    form_processor_info(form_processor* processor = nullptr, ner_feature offset = 0) : processor(processor), offset(offset) {}
  };
  vector<form_processor_info> form_processors;

  vector<unique_ptr<entity_processor>> entity_processors;
};

} // namespace nametag
} // namespace ufal
