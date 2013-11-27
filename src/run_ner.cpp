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
#include "utils/tokenizer/tokenizer.h"

using namespace ufal::nametag;

void recognize_vertical(const ner& n) {
  string line;
  unsigned total_lines = 0;

  vector<string> forms;
  vector<raw_form> raw_forms;
  vector<named_entity> entities;
  string entity_ids, entity_text;

  for (bool not_eof = true; not_eof; ) {
    // Read sentence
    forms.clear();
    raw_forms.clear();
    while ((not_eof = getline(stdin, line)) && !line.empty()) {
      auto tab = line.find('\t');
      forms.emplace_back(tab == string::npos ? line : line.substr(0, tab));
      raw_forms.emplace_back(forms.back().c_str(), forms.back().size());
    }

    // Find named entities in the sentence
    if (!forms.empty()) {
      n.recognize(raw_forms, entities);

      for (auto& entity : entities) {
        entity_ids.clear();
        entity_text.clear();
        for (int i = entity.start; i < entity.start + entity.len; i++) {
          if (i > entity.start) {
            entity_ids += ',';
            entity_text += ' ';
          }
          entity_ids.append(to_string(total_lines + i + 1));
          entity_text.append(raw_forms[i].form, raw_forms[i].form_len);
        }
        printf("%s\t%s\t%s\n", entity_ids.c_str(), entity.type.c_str(), entity_text.c_str());
      }
    }

    total_lines += forms.size() + 1;
  }
}

void encode_and_print(const char* text, int length, FILE* f) {
  const char* to_print = text;
  while (length) {
    while (length && *text != '<' && *text != '>' && *text != '&')
      text++, length--;

    if (length) {
      if (to_print < text) fwrite(to_print, 1, text - to_print, f);
      fputs(*text == '<' ? "&lt;" : *text == '>' ? "&gt;" : "&amp;", f);
      text++, length--;
      to_print = text;
    }
  }
  if (to_print < text) fwrite(to_print, 1, text - to_print, f);
}

void recognize_tokenized(const ner& n, const tokenizer<raw_form>& t) {
  string line;
  string text;
  vector<raw_form> raw_forms;
  vector<named_entity> entities;
  vector<const char*> entity_ends;

  for (bool not_eof = true; not_eof; ) {
    // Read block of text
    text.clear();
    while ((not_eof = getline(stdin, line)) && !line.empty()) {
      text += line;
      text += '\n';
    }

    // Tokenize the text and find named entities
    const char* unprocessed_text = text.c_str();
    const char* to_print = unprocessed_text;
    while(t.next_sentence(unprocessed_text, raw_forms)) {
      n.recognize(raw_forms, entities);

      for (auto& entity : entities) {
        const char* entity_start = raw_forms[entity.start].form;

        // Close entities that end sooned than current entity
        while (!entity_ends.empty() && entity_ends.back() < entity_start) {
          if (to_print < entity_ends.back()) encode_and_print(to_print, entity_ends.back() - to_print, stdout);
          to_print = entity_ends.back();
          entity_ends.pop_back();
          fputs("</ne>", stdout);
        }

        // Print text just before the entity, open it and add end to the stack
        if (to_print < entity_start) encode_and_print(to_print, entity_start - to_print, stdout);
        to_print = entity_start;
        printf("<ne type='%s'>", entity.type.c_str());
        entity_ends.push_back(raw_forms[entity.start + entity.len - 1].form + raw_forms[entity.start + entity.len - 1].form_len);
      }

      // Close unclosed entities
      while (!entity_ends.empty()) {
        if (to_print < entity_ends.back()) encode_and_print(to_print, entity_ends.back() - to_print, stdout);
        to_print = entity_ends.back();
        entity_ends.pop_back();
        fputs("</ne>", stdout);
      }
    }
    // Write rest of the text (should be just spaces)
    if (to_print < unprocessed_text) encode_and_print(to_print, unprocessed_text - to_print, stdout);

    if (not_eof) printf("\n");
  }
}

int main(int argc, char* argv[]) {
  if (argc <= 1 || (strcmp(argv[1], "-t") == 0 && argc <= 3))
    runtime_errorf("Usage: %s [-t tokenizer_id] ner_model", argv[0]);

  int argi = 1;
  unique_ptr<tokenizer<raw_form>> t;
  if (strcmp(argv[argi], "-t") == 0) {
    tokenizer_id id;
    if (!tokenizer_ids::parse(argv[argi + 1], id)) runtime_errorf("Unknown tokenizer_id '%s'!", argv[argi + 1]);
    t.reset(tokenizer<raw_form>::create(id, /*split_hyphenated_words*/ true));
    if (!t) runtime_errorf("Cannot create tokenizer '%s'!", argv[argi + 1]);
    argi += 2;
  }

  eprintf("Loading ner: ");
  unique_ptr<ner> n(ner::load(argv[argi]));
  if (!n) runtime_errorf("Cannot load ner from file '%s'!", argv[argi]);
  eprintf("done\n");

  eprintf("Recognizing: ");
  clock_t now = clock();
  if (t) recognize_tokenized(*n, *t);
  else recognize_vertical(*n);
  eprintf("done, in %.3f seconds.\n", (clock() - now) / double(CLOCKS_PER_SEC));

  return 0;
}
