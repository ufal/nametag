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

using namespace ufal::nametag;

static void recognize_vertical(const ner& recognizer);
static void recognize_untokenized(const ner& recognizer);

int main(int argc, char* argv[]) {
  bool use_vertical = false;

  int argi = 1;
  if (argi < argc && strcmp(argv[argi], "-v") == 0) argi++, use_vertical = true;
  if (argi + 1 < argc) runtime_errorf("Usage: %s [-v] ner_model", argv[0]);

  eprintf("Loading ner: ");
  unique_ptr<ner> recognizer(ner::load(argv[argi]));
  if (!recognizer) runtime_errorf("Cannot load ner from file '%s'!", argv[argi]);
  eprintf("done\n");

  eprintf("Recognizing: ");
  clock_t now = clock();
  if (use_vertical) recognize_vertical(*recognizer);
  else recognize_untokenized(*recognizer);
  eprintf("done, in %.3f seconds.\n", (clock() - now) / double(CLOCKS_PER_SEC));

  return 0;
}

void recognize_vertical(const ner& recognizer) {
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
    while ((not_eof = getline(stdin, line)) && !line.empty()) {
      auto tab = line.find('\t');
      words.emplace_back(tab == string::npos ? line : line.substr(0, tab));
      forms.emplace_back(words.back());
    }

    // Find named entities in the sentence
    if (!forms.empty()) {
      recognizer.recognize(forms, entities);

      for (auto& entity : entities) {
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
        printf("%s\t%s\t%s\n", entity_ids.c_str(), entity.type.c_str(), entity_text.c_str());
      }
    }

    total_lines += forms.size() + 1;
  }
}

static void encode_entities_and_print(const char* text, size_t length);

void recognize_untokenized(const ner& recognizer) {
  string line, text;
  vector<named_entity> entities;
  vector<size_t> entity_ends;

  for (bool not_eof = true; not_eof; ) {
    // Read block of text
    text.clear();
    while ((not_eof = getline(stdin, line)) && !line.empty()) {
      text += line;
      text += '\n';
    }
    if (not_eof) text += '\n';

    // Tokenize the text and find named entities
    size_t unprinted = 0;
    recognizer.tokenize_and_recognize(text.c_str(), entities);
    for (auto& entity : entities) {
      // Close entities that end sooned than current entity
      while (!entity_ends.empty() && entity_ends.back() < entity.start) {
        if (unprinted < entity_ends.back()) encode_entities_and_print(text.c_str() + unprinted, entity_ends.back() - unprinted);
        unprinted = entity_ends.back();
        entity_ends.pop_back();
        fputs("</ne>", stdout);
      }

      // Print text just before the entity, open it and add end to the stack
      if (unprinted < entity.start) encode_entities_and_print(text.c_str() + unprinted, entity.start - unprinted);
      unprinted = entity.start;
      printf("<ne type='%s'>", entity.type.c_str());
      entity_ends.push_back(entity.start + entity.length);
    }

    // Close unclosed entities
    while (!entity_ends.empty()) {
      if (unprinted < entity_ends.back()) encode_entities_and_print(text.c_str() + unprinted, entity_ends.back() - unprinted);
      unprinted = entity_ends.back();
      entity_ends.pop_back();
      fputs("</ne>", stdout);
    }
    // Write rest of the text (should be just spaces)
    if (unprinted < text.size()) encode_entities_and_print(text.c_str() + unprinted, text.size() - unprinted);

    if (not_eof) printf("\n");
  }
}

void encode_entities_and_print(const char* text, size_t length) {
  const char* to_print = text;
  while (length) {
    while (length && *text != '<' && *text != '>' && *text != '&' && *text != '"')
      text++, length--;

    if (length) {
      if (to_print < text) fwrite(to_print, 1, text - to_print, stdout);
      fputs(*text == '<' ? "&lt;" : *text == '>' ? "&gt;" : *text == '&' ? "&amp;" : "&quot;", stdout);
      text++, length--;
      to_print = text;
    }
  }
  if (to_print < text) fwrite(to_print, 1, text - to_print, stdout);
}
