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

#include <cstring>
#include <ctime>
#include <memory>

#include "ner/ner.h"
#include "utils/input.h"
#include "utils/output.h"
#include "utils/process_args.h"

using namespace ufal::nametag;

static void recognize_vertical(FILE* in, FILE* out, const ner& recognizer);
static void recognize_untokenized(FILE* in, FILE* out, const ner& recognizer);

int main(int argc, char* argv[]) {
  bool use_vertical = false;

  int argi = 1;
  if (argi < argc && strcmp(argv[argi], "-v") == 0) argi++, use_vertical = true;
  if (argi >= argc) runtime_errorf("Usage: %s [-v] ner_model", argv[0]);

  eprintf("Loading ner: ");
  unique_ptr<ner> recognizer(ner::load(argv[argi]));
  if (!recognizer) runtime_errorf("Cannot load ner from file '%s'!", argv[argi]);
  eprintf("done\n");
  argi++;

  eprintf("Recognizing: ");
  clock_t now = clock();
  if (use_vertical) process_args(argi, argc, argv, recognize_vertical, *recognizer);
  else process_args(argi, argc, argv, recognize_untokenized, *recognizer);
  eprintf("done, in %.3f seconds.\n", (clock() - now) / double(CLOCKS_PER_SEC));

  return 0;
}

void recognize_vertical(FILE* in, FILE* out, const ner& recognizer) {
  string line;
  unsigned total_lines = 0;

  vector<string> words;
  vector<string_piece> forms;
  vector<named_entity> entities;
  string entity_ids, entity_text;

  for (bool not_eof = true; not_eof; ) {
    // Read sentence
    words.clear();
    forms.clear();
    while ((not_eof = getline(in, line)) && !line.empty()) {
      auto tab = line.find('\t');
      words.emplace_back(tab == string::npos ? line : line.substr(0, tab));
      forms.emplace_back(words.back());
    }

    // Find named entities in the sentence
    if (!forms.empty()) {
      recognizer.recognize(forms, entities);

      for (auto&& entity : entities) {
        entity_ids.clear();
        entity_text.clear();
        for (auto i = entity.start; i < entity.start + entity.length; i++) {
          if (i > entity.start) {
            entity_ids += ',';
            entity_text += ' ';
          }
          entity_ids += to_string(total_lines + i + 1);
          entity_text += words[i];
        }
        fprintf(out, "%s\t%s\t%s\n", entity_ids.c_str(), entity.type.c_str(), entity_text.c_str());
      }
    }

    total_lines += forms.size() + 1;
  }
}

void recognize_untokenized(FILE* in, FILE* out, const ner& recognizer) {
  string para;
  vector<string_piece> forms;
  vector<named_entity> entities;
  vector<const char*> entity_ends;

  unique_ptr<tokenizer> tokenizer(recognizer.new_tokenizer());

  while (getpara(in, para)) {
    // Tokenize the text and find named entities
    tokenizer->set_text(para);
    const char* unprinted = para.c_str();
    while (tokenizer->next_sentence(&forms, nullptr)) {
      recognizer.recognize(forms, entities);
      for (auto&& entity : entities) {
        // Close entities that end sooned than current entity
        while (!entity_ends.empty() && entity_ends.back() < forms[entity.start].str) {
          if (unprinted < entity_ends.back()) print_xml_content(out, unprinted, entity_ends.back() - unprinted);
          unprinted = entity_ends.back();
          entity_ends.pop_back();
          fputs("</ne>", out);
        }

        // Print text just before the entity, open it and add end to the stack
        if (unprinted < forms[entity.start].str) print_xml_content(out, unprinted, forms[entity.start].str - unprinted);
        unprinted = forms[entity.start].str;
        fprintf(out, "<ne type=\"%s\">", entity.type.c_str());
        entity_ends.push_back(forms[entity.start + entity.length - 1].str + forms[entity.start + entity.length - 1].len);
      }

      // Close unclosed entities
      while (!entity_ends.empty()) {
        if (unprinted < entity_ends.back()) print_xml_content(out, unprinted, entity_ends.back() - unprinted);
        unprinted = entity_ends.back();
        entity_ends.pop_back();
        fputs("</ne>", out);
      }
    }
    // Write rest of the text (should be just spaces)
    if (unprinted < para.c_str() + para.size()) print_xml_content(out, unprinted, para.c_str() + para.size() - unprinted);
  }
}
