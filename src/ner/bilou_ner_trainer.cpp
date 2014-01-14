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

#include "bilou_ner.h"
#include "bilou_ner_trainer.h"
#include "utils/input.h"

namespace ufal {
namespace nametag {

void bilou_ner_trainer::train(int stages_len, const network_parameters& parameters, const tagger& tagger, FILE* in_features, FILE* in_train, FILE* in_heldout, FILE* out_ner) {
  if (stages_len <= 0) runtime_errorf("Cannot train NER with <= 0 stages!");
  if (stages_len >= 256) runtime_errorf("Cannot train NER with >= 256 stages!");

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

  // Train required number of stages
  struct stage_info {
    feature_templates templates;
    network_classifier network;
  };
  vector<stage_info> stages(stages_len);

  bool first_stage = true;
  for (auto& stage : stages) {
    // Parse feature templates
    long in_features_start = ftell(in_features);
    if (in_features_start < 0) runtime_errorf("Cannot seek in features file!");
    if (first_stage) eprintf("Parsing feature templates: ");
    stage.templates.parse(in_features, entities);
    if (first_stage) eprintf("done\n");
    if (fseek(in_features, 0, SEEK_SET) != 0) runtime_errorf("Cannot seek in features file!");
    first_stage = false;

    // Generate features
    eprintf("Generating features: ");
    vector<classifier_instance> train_instances, heldout_instances;
    generate_instances(train_data, stage.templates, train_instances, true);
    generate_instances(heldout_data, stage.templates, heldout_instances, false);
    eprintf("done\n");

    // Train and encode the recognizer
    vector<double> outcomes(bilou_entity::total(entities.size()));
    eprintf("Training network classifier.\n");
    if (!stage.network.train(stage.templates.get_total_features(), outcomes.size(), train_instances, heldout_instances, parameters, true))
      runtime_errorf("Cannot train the network classifier!");

    // Use the trained classifier to compute previous_stage
    compute_previous_stage(train_data, stage.templates, stage.network, outcomes);
    compute_previous_stage(heldout_data, stage.templates, stage.network, outcomes);
  }

  // Encode the recognizer
  eprintf("Encoding the recognizer: \n");
  if (!entities.save(out_ner)) runtime_error("Cannot save entity map!");
  if (fputc(stages_len, out_ner) == EOF) runtime_error("Cannot save number of stages!");
  for (auto& stage : stages) {
    if (!stage.templates.save(out_ner)) runtime_error("Cannot save feature templates!");
    if (!stage.network.save(out_ner)) runtime_error("Cannot save classifier network!");
  }
}

void bilou_ner_trainer::load_data(FILE* f, const tagger& tagger, vector<labelled_sentence>& data, entity_map& entity_map, bool add_entities) {
  vector<string> words, entities;
  vector<string_piece> forms;

  data.clear();

  string line;
  vector<string> tokens;
  for (bool eof; true; ) {
    eof = !getline(f, line);
    if (eof || line.empty()) {
      if (!words.empty()) {
        // Tag the sentence
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
            runtime_errorf("Cannot parse entity type %s!", entities[i].c_str());

        // Start a new sentence
        words.clear();
        forms.clear();
        entities.clear();
      }
      if (eof) break;
    } else {
      split(line, '\t', tokens);
      if (tokens.size() != 2) runtime_errorf("The NER data line '%s' does not contain two columns!", line.c_str());
      words.emplace_back(tokens[0]);
      forms.emplace_back(words.back());
      entities.emplace_back(tokens[1]);
    }
  }
}

void bilou_ner_trainer::generate_instances(vector<labelled_sentence>& data, const feature_templates& templates, vector<classifier_instance>& instances, bool add_features) {
  string buffer;

  for (auto& sentence : data) {
    sentence.sentence.clear_features();
    sentence.sentence.clear_probabilities_local_filled();

    // Sentence processors
    templates.process_sentence(sentence.sentence, buffer, add_features);

    // TODO: Form processors

    // Create classifier instances
    for (unsigned i = 0; i < sentence.sentence.size; i++)
     instances.emplace_back(sentence.sentence.features[i], sentence.outcomes[i]);
  }
}

void bilou_ner_trainer::compute_previous_stage(vector<labelled_sentence>& data, const feature_templates& templates, const network_classifier& network, vector<double>& outcomes) {
  string buffer;

  for (auto& labelled_sentence : data) {
    auto& sentence = labelled_sentence.sentence;

    sentence.clear_features();
    sentence.clear_probabilities_local_filled();

    // Sentence processors
    templates.process_sentence(sentence, buffer);

    // Form processors
    for (unsigned i = 0; i < sentence.size; i++) {
      templates.process_form(i, sentence, buffer);
      if (!sentence.probabilities[i].local_filled) {
        network.classify(sentence.features[i], outcomes);
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
