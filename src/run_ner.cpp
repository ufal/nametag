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
#include <cstring>
#include <ctime>
#include <memory>

#include "ner/ner.h"
#include "utils/input.h"
#include "utils/output.h"
#include "utils/parse_options.h"
#include "utils/process_args.h"

using namespace ufal::nametag;

static void sort_entities(vector<named_entity>& entities);
static void recognize_vertical(FILE* in, FILE* out, const ner& recognizer, tokenizer& tokenizer);
static void recognize_untokenized(FILE* in, FILE* out, const ner& recognizer, tokenizer& tokenizer);

int main(int argc, char* argv[]) {
  options_map options;
  if (!parse_options({{"input",{"untokenized", "vertical"}},
                      {"output",{"vertical","xml"}}}, argc, argv, options) ||
      argc < 2)
    runtime_errorf("Usage: %s [options] recognizer_model [file[:output_file]]...\n"
                   "Options: --input=untokenized|vertical\n"
                   "         --output=vertical|xml", argv[0]);

  eprintf("Loading ner: ");
  unique_ptr<ner> recognizer(ner::load(argv[1]));
  if (!recognizer) runtime_errorf("Cannot load ner from file '%s'!", argv[1]);
  eprintf("done\n");

  unique_ptr<tokenizer> tokenizer(options.count("input") && options["input"] == "vertical" ? tokenizer::new_vertical_tokenizer() : recognizer->new_tokenizer());
  if (!tokenizer) runtime_errorf("No tokenizer is defined for the supplied model!");

  clock_t now = clock();
  if (options.count("output") && options["output"] == "vertical")  process_args(2, argc, argv, recognize_vertical, *recognizer, *tokenizer);
  else process_args(2, argc, argv, recognize_untokenized, *recognizer, *tokenizer);
  eprintf("Recognizing done, in %.3f seconds.\n", (clock() - now) / double(CLOCKS_PER_SEC));

  return 0;
}

void recognize_vertical(FILE* in, FILE* out, const ner& recognizer, tokenizer& tokenizer) {
  string para;
  vector<string_piece> forms;
  vector<named_entity> entities;
  unsigned total_tokens = 0;
  string entity_ids, entity_text;

  while (getpara(in, para)) {
    // Tokenize and tag
    tokenizer.set_text(para);
    while (tokenizer.next_sentence(&forms, nullptr)) {
      recognizer.recognize(forms, entities);
      sort_entities(entities);

      for (auto&& entity : entities) {
        entity_ids.clear();
        entity_text.clear();
        for (auto i = entity.start; i < entity.start + entity.length; i++) {
          if (i > entity.start) {
            entity_ids += ',';
            entity_text += ' ';
          }
          entity_ids += to_string(total_tokens + i + 1);
          entity_text.append(forms[i].str, forms[i].len);
        }
        fprintf(out, "%s\t%s\t%s\n", entity_ids.c_str(), entity.type.c_str(), entity_text.c_str());
      }
      total_tokens += forms.size() + 1;
    }
  }
}

void recognize_untokenized(FILE* in, FILE* out, const ner& recognizer, tokenizer& tokenizer) {
  string para;
  vector<string_piece> forms;
  vector<named_entity> entities;
  vector<size_t> entity_ends;

  while (getpara(in, para)) {
    // Tokenize the text and find named entities
    tokenizer.set_text(para);
    const char* unprinted = para.c_str();
    while (tokenizer.next_sentence(&forms, nullptr)) {
      recognizer.recognize(forms, entities);
      sort_entities(entities);

      for (unsigned i = 0, e = 0; i < forms.size(); i++) {
        if (unprinted < forms[i].str) print_xml_content(out, unprinted, forms[i].str - unprinted);
        if (i == 0) fputs("<sentence>", out);

        // Open entities starting at current token
        for (; e < entities.size() && entities[e].start == i; e++) {
          fprintf(out, "<ne type=\"%s\">", entities[e].type.c_str());
          entity_ends.push_back(entities[e].start + entities[e].length - 1);
        }

        // The token itself
        fputs("<token>", out);
        print_xml_content(out, forms[i].str, forms[i].len);
        fputs("</token>", out);

        // Close entities ending after current token
        while (!entity_ends.empty() && entity_ends.back() == i) {
          fputs("</ne>", out);
          entity_ends.pop_back();
        }
        if (i + 1 == forms.size()) fputs("</sentence>", out);
        unprinted = forms[i].str + forms[i].len;
      }
    }
    // Write rest of the text (should be just spaces)
    if (unprinted < para.c_str() + para.size()) print_xml_content(out, unprinted, para.c_str() + para.size() - unprinted);
  }
}

void sort_entities(vector<named_entity>& entities) {
  struct named_entity_comparator {
    static bool lt(const named_entity& a, const named_entity& b) {
      return a.start < b.start || (a.start == b.start && a.length > b.length);
    }
  };

  // Many models return entities sorted -- it is worthwhile to check that.
  if (!is_sorted(entities.begin(), entities.end(), named_entity_comparator::lt))
    sort(entities.begin(), entities.end(), named_entity_comparator::lt);
}
