// This file is part of NameTag <http://github.com/ufal/nametag/>.
//
// Copyright 2016 Institute of Formal and Applied Linguistics, Faculty of
// Mathematics and Physics, Charles University in Prague, Czech Republic.
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#include "common.h"
#include "bilou_ner.h"
#include "bilou/bilou_entity.h"
#include "bilou/bilou_type.h"
#include "tokenizer/morphodita_tokenizer_wrapper.h"

namespace ufal {
namespace nametag {

bilou_ner::bilou_ner(ner_id id) : id(id) {}

bool bilou_ner::load(istream& is) {
  if (tagger.reset(tagger::load_instance(is)), !tagger) return false;
  if (!named_entities.load(is)) return false;

  unique_ptr<tokenizer> tokenizer(new_tokenizer());
  if (!templates.load(is, nlp_pipeline(tokenizer.get(), tagger.get()))) return false;

  int stages = is.get();
  if (stages == EOF) return false;
  networks.resize(stages);
  for (auto&& network : networks)
    if (!network.load(is)) return false;

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

  if (sentence.size) {
    sentence.clear_previous_stage();

    // Perform required NER stages
    for (auto&& network : networks) {
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

tokenizer* bilou_ner::new_tokenizer() const {
  return new_tokenizer(id);
}

void bilou_ner::entity_types(vector<string>& types) const {
  types.resize(named_entities.size());
  for (unsigned i = 0; i < types.size(); i++)
    types[i] = named_entities.name(i);
}

void bilou_ner::gazetteers(vector<string>& gazetteers, vector<int>* gazetteer_types) const {
  gazetteers.clear();
  if (gazetteer_types) gazetteer_types->clear();

  templates.gazetteers(gazetteers, gazetteer_types);
}

void bilou_ner::fill_bilou_probabilities(const vector<double>& outcomes, bilou_probabilities& prob) {
  for (auto&& prob_bilou : prob.bilou)
    prob_bilou.probability = -1;

  for (bilou_entity::value i = 0; i < outcomes.size(); i++) {
    auto bilou = bilou_entity::get_bilou(i);
    if (outcomes[i] > prob.bilou[bilou].probability) {
      prob.bilou[bilou].probability = outcomes[i];
      prob.bilou[bilou].entity = bilou_entity::get_entity(i);
    }
  }
}

tokenizer* bilou_ner::new_tokenizer(ner_id id) {
  switch (id) {
    case ner_id::CZECH_NER:
      return new morphodita_tokenizer_wrapper(morphodita::tokenizer::new_czech_tokenizer());
    case ner_id::ENGLISH_NER:
      return new morphodita_tokenizer_wrapper(morphodita::tokenizer::new_english_tokenizer());
    case ner_id::GENERIC_NER:
      return new morphodita_tokenizer_wrapper(morphodita::tokenizer::new_generic_tokenizer());
  }

  return nullptr;
}

} // namespace nametag
} // namespace ufal
