// This file is part of NameTag <http://github.com/ufal/nametag/>.
//
// Copyright 2016 Institute of Formal and Applied Linguistics, Faculty of
// Mathematics and Physics, Charles University in Prague, Czech Republic.
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#include "feature_templates.h"
#include "utils/compressor.h"
#include "utils/parse_int.h"
#include "utils/split.h"

namespace ufal {
namespace nametag {

void feature_templates::parse(istream& is, entity_map& entities, const nlp_pipeline& pipeline) {
  processors.clear();
  total_features = 1; // An omnipresent feature used in process_sentence

  string line;
  vector<string> tokens;
  while (getline(is, line)) {
    // Ignore empty lines and lines commented with #
    if (line.empty() || line[0] == '#') continue;

    split(line, ' ', tokens);
    if (tokens.size() < 1) runtime_failure("Bad line '" << line << "' of feature templates file!");

    vector<string> token0_parts;
    split(tokens[0], '/', token0_parts);
    if (token0_parts.size() < 1 || token0_parts.size() > 2) runtime_failure("Bad feature template description at line '" << line << "' of feature templates file!");

    string template_name = token0_parts[0];
    int window = token0_parts.size() > 1 ? parse_int(token0_parts[1].c_str(), "feature_template_window") : 0;
    vector<string> args;
    for (unsigned i = 1; i < tokens.size(); i++)
      args.emplace_back(tokens[i]);

    // Try initialize the processor
    auto* processor = feature_processor::create(template_name);
    if (processor) {
      if (!processor->parse(window, args, entities, &total_features, pipeline)) runtime_failure("Cannot initialize feature processor '" << template_name << "' from line '" << line << "' of feature templates file!");
      processors.emplace_back(template_name, processor);
      continue;
    }

    // Fail
    runtime_failure("Cannot create feature template '" << template_name << "' from line '" << line << "' of feature templates file!");
  }
}

bool feature_templates::save(ostream& os) {
  binary_encoder enc;

  enc.add_4B(total_features);

  enc.add_4B(processors.size());
  for (auto&& processor : processors) {
    enc.add_str(processor.name);
    processor.processor->save(enc);
  }

  return compressor::save(os, enc);
}

} // namespace nametag
} // namespace ufal
