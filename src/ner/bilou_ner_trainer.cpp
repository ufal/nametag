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

#include <algorithm>
#include <memory>
#include <unordered_map>

#include "bilou_entity.h"
#include "bilou_ner_trainer.h"
#include "classifier/network_classifier.h"
#include "entity_map.h"
#include "features/feature_templates.h"
#include "utils/input.h"

namespace ufal {
namespace nametag {

struct labelled_sentence {
  ner_sentence sentence;
  vector<bilou_entity::value> outcomes;
};

static void load_data(FILE* f, const tagger& tagger, vector<labelled_sentence>& data, entity_map& entity_map, bool add_entities) {
  vector<string> forms, entities;
  vector<raw_form> raw_forms;

  data.clear();

  string line;
  vector<string> tokens;
  for (bool eof; true; ) {
    eof = !getline(f, line);
    if (eof || line.empty()) {
      if (!forms.empty()) {
        // Tag the sentence
        for (auto& form : forms)
          raw_forms.emplace_back(form.c_str(), form.size());

        data.emplace_back();
        auto& sentence = data.back();
        tagger.tag(raw_forms, sentence.sentence);

        // Decode the entities names and ranges
        for (unsigned i = 0; i < entities.size(); i++)
          if (entities[i] == "_" || entities[i] == "O")
            sentence.outcomes.emplace_back(bilou_entity::O);
          else if (entities[i].size() >= 3 && (entities[i].compare(0, 2, "I-") == 0 || entities[i].compare(0, 2, "B-") == 0)) {
            bool has_prev = i > 0 && entities[i][0] == 'I' && entities[i-1].compare(1, string::npos, entities[i], 1, string::npos) == 0;
            bool has_next = i+1 < entities.size() && entities[i+1][0] != 'B' && entities[i+1].compare(1, string::npos, entities[i], 1, string::npos) == 0;
            entity_type entity = entity_map.parse(entities[i].c_str() + 2, add_entities);
            sentence.outcomes.emplace_back(!has_prev && !has_next ? bilou_entity::U(entity) : !has_prev && has_next ? bilou_entity::B(entity) : has_prev && has_next ? bilou_entity::I : bilou_entity::L);
          }
          else
            runtime_errorf("Cannot parse entity type %s!", entities[i].c_str());

        // Start a new sentence
        forms.clear();
        entities.clear();
        raw_forms.clear();
      }
      if (eof) break;
    } else {
      split(line, '\t', tokens);
      if (tokens.size() != 2) runtime_errorf("The NER data line '%s' does not contain two columns!", line.c_str());
      forms.emplace_back(tokens[0]);
      entities.emplace_back(tokens[1]);
    }
  }
}

static void generate_instances(vector<labelled_sentence>& data, const feature_templates& templates, vector<classifier_instance>& instances) {
  string buffer;

  for (auto& sentence : data) {
    // Sentence processors
    templates.process_sentence(sentence.sentence, buffer);

    // Form processors
    for (unsigned i = 0; i < sentence.sentence.size; i++) {
      // Fill local probs
      auto& probs = sentence.sentence.probabilities[i].local;
      probs.bilou.fill({0, entity_type_unknown});
      probs.bilou[bilou_entity::get_bilou(sentence.outcomes[i])] = {1, bilou_entity::get_entity(sentence.outcomes[i])};
      sentence.sentence.probabilities[i].local_filled = true;

      // Update global probs
      if (i == 0) {
        sentence.sentence.probabilities[i].global.init(probs);
      } else {
        sentence.sentence.probabilities[i].global.update(probs, sentence.sentence.probabilities[i - 1].global);
      }

      // Run form processors
      templates.process_form(i, sentence.sentence, buffer);
    }

    // Create classifier instances
    for (unsigned i = 0; i < sentence.sentence.size; i++)
     instances.emplace_back(sentence.sentence.features[i], sentence.outcomes[i]);
  }
}

void bilou_ner_trainer::train(const network_parameters& parameters, const tagger& tagger, FILE* in_features, FILE* in_train, FILE* in_heldout, FILE* out_ner) {
  // Parse feature templates
  feature_templates templates;
  eprintf("Parsing feature templates: ");
  templates.parse(in_features);
  eprintf("done\n");

  // Load training and possibly also heldout data
  entity_map entities;
  vector<labelled_sentence> train_data;
  eprintf("Loading train data: ");
  load_data(in_train, tagger, train_data, entities, true);
  eprintf("done, %d sentences\n", train_data.size());
  if (!entities.size()) runtime_errorf("No named entities present in the training data!");

  vector<labelled_sentence> heldout_data;
  if (in_heldout) {
    eprintf("Loading heldout data: ");
    load_data(in_heldout, tagger, heldout_data, entities, false);
    eprintf("done, %d sentences\n", heldout_data.size());
  }

  // Generate features
  eprintf("Generating features: ");
  vector<classifier_instance> train_instances, heldout_instances;

  // Create all features before freezing
  generate_instances(train_data, templates, train_instances);

  // Freeze the features and regenerate them
  ner_feature features = templates.freeze(entities);
  train_instances.clear();
  generate_instances(train_data, templates, train_instances);
  generate_instances(heldout_data, templates, heldout_instances);
  eprintf("done\n");

  // Train and encode the recognizer
  eprintf("Training network classifier.\n");
  network_classifier network;
  if (!network.train(features, bilou_entity::total(entities.size()), train_instances, heldout_instances, parameters, true))
      runtime_errorf("Cannot train the network classifier!");

  eprintf("Encoding the recognizer: \n");
  if (!entities.save(out_ner))
    runtime_error("Cannot save entity map!");
  if (!templates.save(out_ner))
    runtime_error("Cannot save feature templates!");
  if (!network.save(out_ner))
    runtime_error("Cannot save classifier network!");
}

} // namespace nametag
} // namespace ufal
