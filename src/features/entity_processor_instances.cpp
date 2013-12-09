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

#include "entity_processor.h"

namespace ufal {
namespace nametag {

// Macro for defining the instances
#define BEGIN_ENTITY_PROCESSOR(Name)                                             \
class Name##_entity_processor : public entity_processor {                        \
  typedef entity_processor super;                                                \
 public:                                                                         \
  virtual const string& name() const { static string name(#Name); return name; }

#define END_ENTITY_PROCESSOR(Name)                                                     \
};                                                                                     \
static entity_processor::registrator<Name##_entity_processor> Name##_entity_processor;


////////////////////////
// CzechAddContainers //
////////////////////////

BEGIN_ENTITY_PROCESSOR(CzechAddContainers)
  virtual void process_entities(ner_sentence& /*sentence*/, vector<named_entity>& entities, vector<named_entity>& buffer) const override {
    buffer.clear();

    for (unsigned i = 0; i < entities.size(); i++) {
      // P if ps+ pf+
      if (entities[i].type.compare("pf") == 0 && (!i || entities[i-1].start + entities[i-1].length < entities[i].start || entities[i-1].type.compare("pf") != 0)) {
        unsigned j = i + 1;
        while (j < entities.size() && entities[j].start == entities[j-1].start + entities[j-1].length && entities[j].type.compare("pf") == 0) j++;
        if (j < entities.size() && entities[j].start == entities[j-1].start + entities[j-1].length && entities[j].type.compare("ps") == 0) {
          j++;
          while (j < entities.size() && entities[j].start == entities[j-1].start + entities[j-1].length && entities[j].type.compare("ps") == 0) j++;
          buffer.emplace_back(entities[i].start, entities[j - 1].start + entities[j - 1].length - entities[i].start, "P");
        }
      }

      // T if td tm ty | td tm
      if (entities[i].type.compare("td") == 0 && i+1 < entities.size() && entities[i+1].start == entities[i].start + entities[i].length && entities[i+1].type.compare("tm") == 0) {
        unsigned j = i + 2;
        if (j < entities.size() && entities[j].start == entities[j-1].start + entities[j-1].length && entities[j].type.compare("ty") == 0) j++;
        buffer.emplace_back(entities[i].start, entities[j - 1].start + entities[j - 1].length - entities[i].start, "T");
      }
      // T if !td tm ty
      if (entities[i].type.compare("tm") == 0 && (!i || entities[i-1].start + entities[i-1].length < entities[i].start || entities[i-1].type.compare("td") != 0))
        if (i+1 < entities.size() && entities[i+1].start == entities[i].start + entities[i].length && entities[i+1].type.compare("ty") == 0)
          buffer.emplace_back(entities[i].start, entities[i + 1].start + entities[i + 1].length - entities[i].start, "T");

      buffer.push_back(entities[i]);
    }

    if (buffer.size() > entities.size()) entities = buffer;
  }
END_ENTITY_PROCESSOR(CzechAddContainers)

} // namespace nametag
} // namespace ufal
