// This file is part of NameTag <http://github.com/ufal/nametag/>.
//
// Copyright 2016 Institute of Formal and Applied Linguistics, Faculty of
// Mathematics and Physics, Charles University in Prague, Czech Republic.
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#include <fstream>

#include "ner/bilou_ner_trainer.h"
#include "ner/ner_ids.h"
#include "tagger/tagger.h"
#include "utils/iostreams.h"
#include "utils/options.h"
#include "utils/parse_double.h"
#include "utils/parse_int.h"
#include "version/version.h"

using namespace ufal::nametag;

int main(int argc, char* argv[]) {
  iostreams_init();

  ner_id id;
  options::map options;
  if (argc == 1 || !ner_ids::parse(argv[1], id))
    if (!options::parse({{"version", options::value::none},
                         {"help", options::value::none}}, argc, argv, options) ||
        options.count("help") ||
        (argc < 2 && !options.count("version")))
      runtime_failure("Usage: " << argv[0] << " [options] ner_identifier [ner_identifier_specific_options]\n"
                      "Options: --version\n"
                      "         --help");
  if (options.count("version"))
    return cout << version::version_and_copyright() << endl, 0;

  if (!ner_ids::parse(argv[1], id)) runtime_failure("Cannot parse ner_identifier '" << argv[1] << "'!\n");

  // Switch stdout to binary mode.
  iostreams_init_binary_output();

  switch (id) {
    case ner_ids::CZECH_NER:
    case ner_ids::ENGLISH_NER:
    case ner_ids::GENERIC_NER:
      {
        if (argc < 10) runtime_failure("Usage: " << argv[0] << ' ' << argv[1] << " tagger_id[:tagger_options] features stages iterations missing_weight initial_learning_rate final_learning_rate gaussian hidden_layer [heldout_data]");

        // Encode the ner_id
        cout.put(id);

        // Create and encode the tagger
        unique_ptr<tagger> tagger(tagger::create_and_encode_instance(argv[2], cout));
        if (!tagger) runtime_failure("Cannot load and encode tagger!");

        // Parse options
        network_parameters parameters;
        const char* features_file = argv[3];
        int stages = parse_int(argv[4], "stages");
        parameters.iterations = parse_int(argv[5], "iterations");
        parameters.missing_weight = parse_double(argv[6], "missing_weight");
        parameters.initial_learning_rate = parse_double(argv[7], "initial_learning_rate");
        parameters.final_learning_rate = parse_double(argv[8], "final_learning_rate");
        parameters.gaussian_sigma = parse_double(argv[9], "gaussian");
        parameters.hidden_layer = parse_int(argv[10], "hidden_layer");
        const char* heldout_file = argc == 11 ? nullptr : argv[11];

        // Open needed files
        ifstream features(features_file);
        if (!features.is_open()) runtime_failure("Cannot open features file '" << features_file << "'!");

        ifstream heldout;
        if (heldout_file) {
          heldout.open(heldout_file);
          if (!heldout.is_open()) runtime_failure("Cannot open heldout file '" << heldout_file << "'!");
        } else {
          heldout.setstate(ios::failbit);
        }

        // Encode the ner itself
        bilou_ner_trainer::train(id, stages, parameters, *tagger, features, cin, heldout, cout);

        cerr << "Recognizer saved." << endl;
        break;
      }
    default:
      runtime_failure("Unimplemented ner_identifier '" << argv[1] << "'!\n");
  }

  return 0;
}
