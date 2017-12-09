// This file is part of NameTag <http://github.com/ufal/nametag/>.
//
// Copyright 2016 Institute of Formal and Applied Linguistics, Faculty of
// Mathematics and Physics, Charles University in Prague, Czech Republic.
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#include <algorithm>
#include <ctime>

#include "ner/ner.h"
#include "utils/getpara.h"
#include "utils/iostreams.h"
#include "utils/options.h"
#include "utils/process_args.h"
#include "utils/xml_encoded.h"
#include "version/version.h"

using namespace ufal::nametag;

static void sort_entities(vector<named_entity>& entities);
static void recognize_conll(istream& is, ostream& os, const ner& recognizer, tokenizer& tokenizer);
static void recognize_vertical(istream& is, ostream& os, const ner& recognizer, tokenizer& tokenizer);
static void recognize_untokenized(istream& is, ostream& os, const ner& recognizer, tokenizer& tokenizer);

int main(int argc, char* argv[]) {
  iostreams_init();

  options::map options;
  if (!options::parse({{"input",options::value{"untokenized", "vertical"}},
                       {"output",options::value{"vertical","xml", "conll"}},
                       {"version", options::value::none},
                       {"help", options::value::none}}, argc, argv, options) ||
      options.count("help") ||
      (argc < 2 && !options.count("version")))
    runtime_failure("Usage: " << argv[0] << " [options] recognizer_model [file[:output_file]]...\n"
                    "Options: --input=untokenized|vertical\n"
                    "         --output=vertical|xml\n"
                    "         --version\n"
                    "         --help");
  if (options.count("version"))
    return cout << version::version_and_copyright() << endl, 0;

  cerr << "Loading ner: ";
  unique_ptr<ner> recognizer(ner::load(argv[1]));
  if (!recognizer) runtime_failure("Cannot load ner from file '" << argv[1] << "'!");
  cerr << "done" << endl;

  unique_ptr<tokenizer> tokenizer(options.count("input") && options["input"] == "vertical" ? tokenizer::new_vertical_tokenizer() : recognizer->new_tokenizer());
  if (!tokenizer) runtime_failure("No tokenizer is defined for the supplied model!");

  clock_t now = clock();
  if (options.count("output") && options["output"] == "vertical")  process_args(2, argc, argv, recognize_vertical, *recognizer, *tokenizer);
  else if (options.count("output") && options["output"] == "conll")  process_args(2, argc, argv, recognize_conll, *recognizer, *tokenizer);
  else process_args(2, argc, argv, recognize_untokenized, *recognizer, *tokenizer);
  cerr << "Recognizing done, in " << fixed << setprecision(3) << (clock() - now) / double(CLOCKS_PER_SEC) << " seconds." << endl;

  return 0;
}

void recognize_conll(istream& is, ostream& os, const ner& recognizer, tokenizer& tokenizer) {
  string para;
  vector<string_piece> forms;
  vector<named_entity> entities;

  while (getpara(is, para)) {
    // Tokenize and tag
    tokenizer.set_text(para);
    while (tokenizer.next_sentence(&forms, nullptr)) {
      recognizer.recognize(forms, entities);
      sort_entities(entities);

      string entity_type;
      unsigned in_entity = 0;
      bool entity_start;
      for (unsigned i = 0, e = 0; i < forms.size(); i++) {
        if (!in_entity && e < entities.size() && entities[e].start == i) {
          in_entity = entities[e].length;
          entity_start = true;
          entity_type = entities[e].type;
          e++;
        }

        os << forms[i] << '\t';
        if (in_entity) {
          os << (entity_start ? "B-" : "I-") << entity_type;
          entity_start = false;
          in_entity--;
        } else {
          os << '_';
        }
        os << '\n';
      }

      os << '\n' << flush;
    }
  }
}

void recognize_vertical(istream& is, ostream& os, const ner& recognizer, tokenizer& tokenizer) {
  string para;
  vector<string_piece> forms;
  vector<named_entity> entities;
  unsigned total_tokens = 0;
  string entity_ids, entity_text;

  while (getpara(is, para)) {
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
        os << entity_ids << '\t' << entity.type << '\t' << entity_text << '\n';
      }
      os << flush;
      total_tokens += forms.size() + 1;
    }
  }
}

void recognize_untokenized(istream& is, ostream& os, const ner& recognizer, tokenizer& tokenizer) {
  string para;
  vector<string_piece> forms;
  vector<named_entity> entities;
  vector<size_t> entity_ends;

  while (getpara(is, para)) {
    // Tokenize the text and find named entities
    tokenizer.set_text(para);
    const char* unprinted = para.c_str();
    while (tokenizer.next_sentence(&forms, nullptr)) {
      recognizer.recognize(forms, entities);
      sort_entities(entities);

      for (unsigned i = 0, e = 0; i < forms.size(); i++) {
        if (unprinted < forms[i].str) os << xml_encoded(string_piece(unprinted, forms[i].str - unprinted));
        if (i == 0) os << "<sentence>";

        // Open entities starting at current token
        for (; e < entities.size() && entities[e].start == i; e++) {
          os << "<ne type=\"" << xml_encoded(entities[e].type, true) << "\">";
          entity_ends.push_back(entities[e].start + entities[e].length - 1);
        }

        // The token itself
        os << "<token>" << xml_encoded(forms[i]) << "</token>";

        // Close entities ending after current token
        while (!entity_ends.empty() && entity_ends.back() == i) {
          os << "</ne>";
          entity_ends.pop_back();
        }
        if (i + 1 == forms.size()) os << "</sentence>";
        unprinted = forms[i].str + forms[i].len;
      }
    }
    // Write rest of the text (should be just spaces)
    if (unprinted < para.c_str() + para.size()) os << xml_encoded(string_piece(unprinted, para.c_str() + para.size() - unprinted));
    os << flush;
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
