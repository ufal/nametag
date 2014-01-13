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
#include "bilou_entity.h"
#include "bilou_type.h"

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
  if (forms.empty() || !tagger || !named_entities.size()) return;

  // Acquire cache
  cache* c = caches.pop();
  if (!c) c = new cache(*this);
  auto& sentence = c->sentence;
  auto& outcomes = c->outcomes;

  // Tag
  tagger->tag(forms, sentence);

  if (sentence.size >= forms.size()) {
    // Initialize previous_stage
    for (unsigned i = 0; i < forms.size(); i++)
      sentence.previous_stage[i] = entity_type_unknown;

    // Perform required NER stages
    for (auto& network : networks) {
      // Clear local_filled
      for (unsigned i = 0; i < forms.size(); i++)
        sentence.probabilities[i].local_filled = false;

      // Compute per-sentence feature templates
      templates.process_sentence(sentence, c->string_buffer);

      // Perform classification of sentence words
      for (unsigned i = 0; i < forms.size(); i++) {
        templates.process_form(i, sentence, c->string_buffer);
        if (!sentence.probabilities[i].local_filled) {
          network.classify(sentence.features[i], outcomes);

          bilou_probabilities& p = sentence.probabilities[i].local;
          // Start with ILO
          p.bilou[bilou_type_I].probability = outcomes[bilou_entity::I];
          p.bilou[bilou_type_I].entity = entity_type_unknown;
          p.bilou[bilou_type_L].probability = outcomes[bilou_entity::L];
          p.bilou[bilou_type_L].entity = entity_type_unknown;
          p.bilou[bilou_type_O].probability = outcomes[bilou_entity::O];
          p.bilou[bilou_type_O].entity = entity_type_unknown;
          // Find best B and U
          entity_type entity = 0;
          p.bilou[bilou_type_B].probability = outcomes[bilou_entity::B(entity)];
          p.bilou[bilou_type_B].entity = entity;
          p.bilou[bilou_type_U].probability = outcomes[bilou_entity::U(entity)];
          p.bilou[bilou_type_U].entity = entity;
          while (++entity < named_entities.size()) {
            if (outcomes[bilou_entity::B(entity)] > p.bilou[bilou_type_B].probability) {
              p.bilou[bilou_type_B].probability = outcomes[bilou_entity::B(entity)];
              p.bilou[bilou_type_B].entity = entity;
            }
            if (outcomes[bilou_entity::U(entity)] > p.bilou[bilou_type_U].probability) {
              p.bilou[bilou_type_U].probability = outcomes[bilou_entity::U(entity)];
              p.bilou[bilou_type_U].entity = entity;
            }
          }
          // Done
          sentence.probabilities[i].local_filled = true;
        }

        if (i == 0) {
          sentence.probabilities[i].global.init(sentence.probabilities[i].local);
        } else {
          sentence.probabilities[i].global.update(sentence.probabilities[i].local, sentence.probabilities[i - 1].global);
        }
      }

      // Store the optimum decoding
      bilou_type best = bilou_type_L;
      auto& last_bilou = sentence.probabilities[forms.size() - 1].global.bilou;
      if (last_bilou[bilou_type_O].probability > last_bilou[best].probability) best = bilou_type_O;
      if (last_bilou[bilou_type_U].probability > last_bilou[best].probability) best = bilou_type_U;
      sentence.probabilities[forms.size() - 1].global.best = best;
      for (unsigned i = forms.size() - 1; i; i--) {
        best = sentence.probabilities[i].global.previous[best];
        sentence.probabilities[i - 1].global.best = best;
      }

  //    for (unsigned i = 0; i < forms.size(); i++) {
  //      eprintf("%s-%s-%s-%s-%s", sentence.words[i].form.c_str(), sentence.words[i].raw_lemma.c_str(), sentence.words[i].lemma.c_str(), sentence.words[i].lemma_info.c_str(), sentence.words[i].tag.c_str());
  //      for (auto& feature : sentence.features[i]) eprintf(" %u", feature);
  //      eprintf(": Local B%.2g(%u)", 100*sentence.probabilities[i].local.bilou[bilou_type_B].probability, sentence.probabilities[i].local.bilou[bilou_type_B].entity);
  //      eprintf(" I%.2g", 100*sentence.probabilities[i].local.bilou[bilou_type_I].probability);
  //      eprintf(" L%.2g", 100*sentence.probabilities[i].local.bilou[bilou_type_L].probability);
  //      eprintf(" O%.2g", 100*sentence.probabilities[i].local.bilou[bilou_type_O].probability);
  //      eprintf(" U%.2g(%u)", 100*sentence.probabilities[i].local.bilou[bilou_type_U].probability, sentence.probabilities[i].local.bilou[bilou_type_U].entity);
  //      eprintf(" Global B%.2g(%u)", 100*sentence.probabilities[i].global.bilou[bilou_type_B].probability, sentence.probabilities[i].global.bilou[bilou_type_B].entity);
  //      eprintf(" I%.2g", 100*sentence.probabilities[i].global.bilou[bilou_type_I].probability);
  //      eprintf(" L%.2g", 100*sentence.probabilities[i].global.bilou[bilou_type_L].probability);
  //      eprintf(" O%.2g", 100*sentence.probabilities[i].global.bilou[bilou_type_O].probability);
  //      eprintf(" U%.2g(%u)", 100*sentence.probabilities[i].global.bilou[bilou_type_U].probability, sentence.probabilities[i].global.bilou[bilou_type_U].entity);
  //      eprintf(" Best %.2g(%u)", 100*sentence.probabilities[i].global.bilou[sentence.probabilities[i].global.best].probability, sentence.probabilities[i].global.best);
  //      eprintf("\n");
  //    }
  //    eprintf("\n");
    }

    // Store entities in the output array
    for (unsigned i = 0; i < forms.size(); i++)
      if (sentence.probabilities[i].global.best == bilou_type_U) {
        entities.emplace_back(i, 1, named_entities.name(sentence.probabilities[i].global.bilou[bilou_type_U].entity));
      } else if (sentence.probabilities[i].global.best == bilou_type_B) {
        unsigned start = i++;
        while (i < forms.size() && sentence.probabilities[i].global.best != bilou_type_L) i++;
        entities.emplace_back(start, i - start + (i < forms.size()), named_entities.name(sentence.probabilities[start].global.bilou[bilou_type_B].entity));
      }

    // Process the entities
    templates.process_entities(sentence, entities, c->entities_buffer);
  }

  caches.push(c);
}

} // namespace nametag
} // namespace ufal
