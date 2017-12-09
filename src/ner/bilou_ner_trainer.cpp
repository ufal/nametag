// This file is part of NameTag <http://github.com/ufal/nametag/>.
//
// Copyright 2016 Institute of Formal and Applied Linguistics, Faculty of
// Mathematics and Physics, Charles University in Prague, Czech Republic.
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#include <algorithm>
#include <unordered_map>

#include "bilou_ner.h"
#include "bilou_ner_trainer.h"
#include "utils/split.h"

namespace ufal {
namespace nametag {

void bilou_ner_trainer::train(ner_id id, int stages, const network_parameters& parameters, const tagger& tagger,
                              istream& features, istream& train, istream& heldout, ostream& os) {
  if (stages <= 0) runtime_failure("Cannot train NER with <= 0 stages!");
  if (stages >= 256) runtime_failure("Cannot train NER with >= 256 stages!");

  // Load training and possibly also heldout data
  entity_map entities;
  vector<labelled_sentence> train_data;
  cerr << "Loading train data: ";
  load_data(train, tagger, train_data, entities, true);
  cerr << "done, " << train_data.size() << " sentences" << endl;
  cerr << "Found " << entities.size() << " annotated entity types." << endl;

  vector<labelled_sentence> heldout_data;
  if (heldout) {
    cerr << "Loading heldout data: ";
    load_data(heldout, tagger, heldout_data, entities, false);
    cerr << "done, " << heldout_data.size() << " sentences" << endl;
  }

  // Parse feature templates
  feature_templates templates;
  unique_ptr<tokenizer> tokenizer(bilou_ner::new_tokenizer(id));
  cerr << "Parsing feature templates: ";
  templates.parse(features, entities, nlp_pipeline(tokenizer.get(), &tagger));
  cerr << "done" << endl;

  // Train required number of stages
  vector<network_classifier> networks(stages);

  for (auto&& network : networks) {
    // Generate features
    cerr << "Generating features: ";
    vector<classifier_instance> train_instances, heldout_instances;
    generate_instances(train_data, templates, train_instances, true);
    generate_instances(heldout_data, templates, heldout_instances, false);
    cerr << "done" << endl;

    // Train and encode the recognizer
    cerr << "Training network classifier." << endl;
    if (!network.train(templates.get_total_features(), bilou_entity::total(entities.size()), train_instances, heldout_instances, parameters, true))
      runtime_failure("Cannot train the network classifier!");

    // Use the trained classifier to compute previous_stage
    compute_previous_stage(train_data, templates, network);
    compute_previous_stage(heldout_data, templates, network);
  }

  // Encode the recognizer
  cerr << "Encoding the recognizer." << endl;
  if (!entities.save(os)) runtime_error("Cannot save entity map!");
  if (!templates.save(os)) runtime_error("Cannot save feature templates!");
  if (!os.put(stages)) runtime_error("Cannot save number of stages!");
  for (auto&& network : networks)
    if (!network.save(os)) runtime_error("Cannot save classifier network!");
}

void bilou_ner_trainer::load_data(istream& is, const tagger& tagger, vector<labelled_sentence>& data, entity_map& entity_map, bool add_entities) {
  vector<string> words, entities;
  vector<string_piece> forms;

  data.clear();

  string line;
  vector<string> tokens;
  for (bool eof; true; ) {
    eof = !getline(is, line);
    if (eof || line.empty()) {
      if (!words.empty()) {
        // Tag the sentence
        forms.clear();
        for (auto&& word : words)
          forms.emplace_back(word);
        data.emplace_back();
        auto& sentence = data.back();
        tagger.tag(forms, sentence.sentence);

        // Clear previous_stage
        sentence.sentence.clear_previous_stage();

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
            runtime_failure("Cannot parse entity type " << entities[i] << "!");

        // Start a new sentence
        words.clear();
        entities.clear();
      }
      if (eof) break;
    } else {
      split(line, '\t', tokens);
      if (tokens.size() != 2) runtime_failure("The NER data line '" << line << "' does not contain two columns!");
      words.emplace_back(tokens[0]);
      entities.emplace_back(tokens[1]);
    }
  }
}

void bilou_ner_trainer::generate_instances(vector<labelled_sentence>& data, const feature_templates& templates, vector<classifier_instance>& instances, bool add_features) {
  string buffer;

  for (auto&& sentence : data) {
    sentence.sentence.clear_features();
    sentence.sentence.clear_probabilities_local_filled();

    // Sentence processors
    templates.process_sentence(sentence.sentence, buffer, add_features);

    // Create classifier instances
    for (unsigned i = 0; i < sentence.sentence.size; i++)
     instances.emplace_back(sentence.sentence.features[i], sentence.outcomes[i]);
  }
}

void bilou_ner_trainer::compute_previous_stage(vector<labelled_sentence>& data, const feature_templates& templates, const network_classifier& network) {
  string buffer;
  vector<double> outcomes, network_buffer;

  for (auto&& labelled_sentence : data) {
    auto& sentence = labelled_sentence.sentence;

    sentence.clear_features();
    sentence.clear_probabilities_local_filled();

    // Sentence processors
    templates.process_sentence(sentence, buffer);

    // Sequentially classify sentence words
    for (unsigned i = 0; i < sentence.size; i++) {
      if (!sentence.probabilities[i].local_filled) {
        network.classify(sentence.features[i], outcomes, network_buffer);
        bilou_ner::fill_bilou_probabilities(outcomes, sentence.probabilities[i].local);
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
}

} // namespace nametag
} // namespace ufal
