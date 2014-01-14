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
#include "utils/input.h"
#include "utils/parse_int.h"

namespace ufal {
namespace nametag {

void feature_templates::parse(FILE* f, entity_map& entities) {
  sentence_processors.clear();
  form_processors.clear();
  entity_processors.clear();
  total_features = 1; // An omnipresent feature used in process_sentence

  string line;
  vector<string> tokens;
  while (getline(f, line)) {
    // Ignore empty lines and lines commented with #
    if (line.empty() || line[0] == '#') continue;

    split(line, ' ', tokens);
    if (tokens.size() < 1) runtime_errorf("Bad line '%s' of feature templates file!", line.c_str());

    vector<string> token0_parts;
    split(tokens[0], '/', token0_parts);
    if (token0_parts.size() < 1 || token0_parts.size() > 2) runtime_errorf("Bad feature template description at line '%s' of feature templates file!", line.c_str());

    string template_name = token0_parts[0];
    int window = token0_parts.size() > 1 ? parse_int(token0_parts[1].c_str(), "feature_template_window") : 0;
    vector<string> args;
    for (unsigned i = 1; i < tokens.size(); i++)
      args.emplace_back(tokens[i]);

    // Try sentence processor
    auto* maybe_sentence_processor = sentence_processor::create(template_name);
    if (maybe_sentence_processor) {
      if (!maybe_sentence_processor->parse(window, args, entities, &total_features)) runtime_errorf("Cannot initialize feature template sentence processor '%s' from line '%s' of feature templates file!", template_name.c_str(), line.c_str());
      sentence_processors.emplace_back(template_name, maybe_sentence_processor);
      continue;
    }

    // Try form processor
    auto* maybe_form_processor = form_processor::create(template_name);
    if (maybe_form_processor) {
      if (!maybe_form_processor->parse(window, args, entities, &total_features)) runtime_errorf("Cannot initialize feature template form processor '%s' from line '%s' of feature templates file!", template_name.c_str(), line.c_str());
      form_processors.emplace_back(template_name, maybe_form_processor);
      continue;
    }

    // Try entity processor
    auto* maybe_entity_processor = entity_processor::create(template_name);
    if (maybe_entity_processor) {
      if (window) eprintf("Ignoring window of %d specified in entity_processor '%s'.\n", window, template_name.c_str());
      if (!maybe_entity_processor->parse(args, entities)) runtime_errorf("Cannot initialize feature template entity processor '%s' from line '%s' of feature templates file!", template_name.c_str(), line.c_str());
      entity_processors.emplace_back(template_name, maybe_entity_processor);
      continue;
    }

    // Fail
    runtime_errorf("Cannot create feature template '%s' from line '%s' of feature templates file!", template_name.c_str(), line.c_str());
  }
}

bool feature_templates::save(FILE* f) {
  binary_encoder enc;

  enc.add_4B(total_features);

  enc.add_4B(sentence_processors.size());
  for (auto& processor : sentence_processors) {
    enc.add_1B(processor.name.size());
    enc.add_str(processor.name);
    processor.processor->save(enc);
  }

  enc.add_4B(form_processors.size());
  for (auto& processor : form_processors) {
    enc.add_1B(processor.name.size());
    enc.add_str(processor.name);
    processor.processor->save(enc);
  }

  enc.add_4B(entity_processors.size());
  for (auto& processor : entity_processors) {
    enc.add_1B(processor.name.size());
    enc.add_str(processor.name);
    processor.processor->save(enc);
  }

  return compressor::save(f, enc);
}

} // namespace nametag
} // namespace ufal
