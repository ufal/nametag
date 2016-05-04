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

#include "feature_templates.h"
#include "utils/compressor.h"
#include "utils/parse_int.h"
#include "utils/split.h"

namespace ufal {
namespace nametag {

void feature_templates::parse(istream& is, entity_map& entities) {
  sentence_processors.clear();
  entity_processors.clear();
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

    // Try sentence processor
    auto* maybe_sentence_processor = sentence_processor::create(template_name);
    if (maybe_sentence_processor) {
      if (!maybe_sentence_processor->parse(window, args, entities, &total_features)) runtime_failure("Cannot initialize feature template sentence processor '" << template_name << "' from line '" << line << "' of feature templates file!");
      sentence_processors.emplace_back(template_name, maybe_sentence_processor);
      continue;
    }

    // Try entity processor
    auto* maybe_entity_processor = entity_processor::create(template_name);
    if (maybe_entity_processor) {
      if (window) cerr << "Ignoring window of " << window << " specified in entity_processor '" << template_name << "'." << endl;
      if (!maybe_entity_processor->parse(args, entities)) runtime_failure("Cannot initialize feature template entity processor '" << template_name << "' from line '" << line << "' of feature templates file!");
      entity_processors.emplace_back(template_name, maybe_entity_processor);
      continue;
    }

    // Fail
    runtime_failure("Cannot create feature template '" << template_name << "' from line '" << line << "' of feature templates file!");
  }
}

bool feature_templates::save(ostream& os) {
  binary_encoder enc;

  enc.add_4B(total_features);

  enc.add_4B(sentence_processors.size() + entity_processors.size());
  for (auto&& processor : sentence_processors) {
    enc.add_str(processor.name);
    processor.processor->save(enc);
  }
  for (auto&& processor : entity_processors) {
    enc.add_str(processor.name);
    processor.processor->save(enc);
  }

  return compressor::save(os, enc);
}

} // namespace nametag
} // namespace ufal
