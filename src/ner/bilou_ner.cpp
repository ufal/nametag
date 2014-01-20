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

#include "common.h"
#include "bilou_ner.h"
#include "bilou/bilou_entity.h"
#include "bilou/bilou_type.h"

namespace ufal {
namespace nametag {

bool bilou_ner::load(FILE* f) {
  if (tagger.reset(tagger::load_instance(f)), !tagger) return false;
  if (!named_entities.load(f)) return false;
  if (!templates.load(f)) return false;

  int stages = fgetc(f);
  if (stages == EOF) return false;
  networks.resize(stages);
  for (auto& network : networks)
    if (!network.load(f)) return false;

  return true;
}

void bilou_ner::recognize(const vector<string_piece>& forms, vector<named_entity>& entities) const {
  entities.clear();
  if (forms.empty() || !tagger || !named_entities.size() || !networks.size()) return;

  // Acquire cache
  cache* c = caches.pop();
  if (!c) c = new cache();
  auto& sentence = c->sentence;

  // Tag
  tagger->tag(forms, sentence);

  if (sentence.size >= sentence.size) {
    sentence.clear_previous_stage();

    // Perform required NER stages
    for (auto& network : networks) {
      sentence.clear_features();
      sentence.clear_probabilities_local_filled();

      // Compute per-sentence feature templates
      templates.process_sentence(sentence, c->string_buffer);

      // Sequentially classify sentence words
      for (unsigned i = 0; i < sentence.size; i++) {
        if (!sentence.probabilities[i].local_filled) {
          network.classify(sentence.features[i], c->outcomes, c->network_buffer);
          fill_bilou_probabilities(c->outcomes, sentence.probabilities[i].local);
          sentence.probabilities[i].local_filled = true;
        }

        if (i == 0) {
          sentence.probabilities[i].global.init(sentence.probabilities[i].local);
        } else {
          sentence.probabilities[i].global.update(sentence.probabilities[i].local, sentence.probabilities[i - 1].global);
        }
      }

      sentence.compute_best_decoding();
      sentence.fill_previous_stage();
    }

    // Store entities in the output array
    for (unsigned i = 0; i < sentence.size; i++)
      if (sentence.probabilities[i].global.best == bilou_type_U) {
        entities.emplace_back(i, 1, named_entities.name(sentence.probabilities[i].global.bilou[bilou_type_U].entity));
      } else if (sentence.probabilities[i].global.best == bilou_type_B) {
        unsigned start = i++;
        while (i < sentence.size && sentence.probabilities[i].global.best != bilou_type_L) i++;
        entities.emplace_back(start, i - start + (i < sentence.size), named_entities.name(sentence.probabilities[start].global.bilou[bilou_type_B].entity));
      }

    // Process the entities
    templates.process_entities(sentence, entities, c->entities_buffer);
  }

  caches.push(c);
}

void bilou_ner::fill_bilou_probabilities(const vector<double>& outcomes, bilou_probabilities& prob) {
  for (auto& prob_bilou : prob.bilou)
    prob_bilou.probability = -1;

  for (bilou_entity::value i = 0; i < outcomes.size(); i++) {
    auto bilou = bilou_entity::get_bilou(i);
    if (outcomes[i] > prob.bilou[bilou].probability) {
      prob.bilou[bilou].probability = outcomes[i];
      prob.bilou[bilou].entity = bilou_entity::get_entity(i);
    }
  }
}

} // namespace nametag
} // namespace ufal
