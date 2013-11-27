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

namespace ufal {
namespace nametag {

void feature_templates::parse(FILE* f) {
  sentence_processors.clear();

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

    int window = token0_parts.size() > 1 ? (unsigned) stoi(token0_parts[1]) : 0;
    vector<string> args;
    for (unsigned i = 1; i < tokens.size(); i++)
      args.emplace_back(tokens[i]);

    // Try sentence processor
    auto* maybe_sentence_processor = sentence_processor::create(token0_parts[0]);
    if (maybe_sentence_processor) {
      if (!maybe_sentence_processor->init(window, args)) runtime_errorf("Cannot initialize feature template sentence processor '%s' from line '%s' of feature templates file!", token0_parts[0].c_str(), line.c_str());
      sentence_processors.emplace_back(maybe_sentence_processor);
      continue;
    }

    // Try form processor
    auto* maybe_form_processor = form_processor::create(token0_parts[0]);
    if (maybe_form_processor) {
      if (!maybe_form_processor->init(window, args)) runtime_errorf("Cannot initialize feature template form processor '%s' from line '%s' of feature templates file!", token0_parts[0].c_str(), line.c_str());
      form_processors.emplace_back(maybe_form_processor);
      continue;
    }

    // Try entity processor
    auto* maybe_entity_processor = entity_processor::create(token0_parts[0]);
    if (maybe_entity_processor) {
      if (!maybe_entity_processor->init(args)) runtime_errorf("Cannot initialize feature template entity processor '%s' from line '%s' of feature templates file!", token0_parts[0].c_str(), line.c_str());
      entity_processors.emplace_back(maybe_entity_processor);
      continue;
    }

    // Fail
    runtime_errorf("Cannot create feature template '%s' from line '%s' of feature templates file!", token0_parts[0].c_str(), line.c_str());
  }
}

ner_feature feature_templates::freeze(entity_map& entities) {
  ner_feature offset = 1; //0 is an omnipresent feature

  for (auto& processor : sentence_processors) {
    processor.offset = offset;
    offset += processor.processor->freeze(entities);
  }

  for (auto& processor : form_processors) {
    processor.offset = offset;
    offset += processor.processor->freeze(entities);
  }

  return offset;
}

bool feature_templates::save(FILE* f) {
  binary_encoder enc;

  enc.add_4B(sentence_processors.size());
  for (auto& processor : sentence_processors) {
    processor.processor->save_instance(enc);
    enc.add_4B(processor.offset);
  }

  enc.add_4B(form_processors.size());
  for (auto& processor : form_processors) {
    processor.processor->save_instance(enc);
    enc.add_4B(processor.offset);
  }

  enc.add_4B(entity_processors.size());
  for (auto& processor : entity_processors)
    processor->save_instance(enc);

  return compressor::save(f, enc);
}

} // namespace nametag
} // namespace ufal
