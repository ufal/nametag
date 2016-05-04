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
#include <iomanip>
#include <iostream>
#include <memory>
#include <string>
#include <vector>

#include "nametag.h"
#include "../src/utils/getpara.h"
#include "../src/utils/xml_encoded.h"

using namespace ufal::nametag;
using namespace ufal::nametag::utils;
using namespace std;

static void sort_entities(vector<named_entity>& entities);

int main(int argc, char* argv[]) {
  if (argc < 2) return cerr << "Usage: " << argv[0] << " ner_file" << endl, 1;

  cerr << "Loading ner: ";
  unique_ptr<ner> recognizer(ner::load(argv[1]));
  if (!recognizer) return cerr << "Cannot load ner from file '" << argv[1] << "'!" << endl, 1;
  cerr << "done" << endl;

  unique_ptr<tokenizer> tokenizer(recognizer->new_tokenizer());
  if (!tokenizer) return cerr << "No tokenizer is defined for the supplied model!" << endl, 1;

  string para;
  vector<string_piece> forms;
  vector<named_entity> entities;
  vector<size_t> entity_ends;

  clock_t now = clock();
  while (getpara(cin, para)) {
    // Tokenize the text and find named entities
    tokenizer->set_text(para);
    const char* unprinted = para.c_str();
    while (tokenizer->next_sentence(&forms, nullptr)) {
      recognizer->recognize(forms, entities);
      sort_entities(entities);

      for (unsigned i = 0, e = 0; i < forms.size(); i++) {
        if (unprinted < forms[i].str) cout << xml_encoded(string_piece(unprinted, forms[i].str - unprinted));
        if (i == 0) cout << "<sentence>";

        // Open entities starting at current token
        for (; e < entities.size() && entities[e].start == i; e++) {
          cout << "<ne type=\"" << xml_encoded(entities[e].type, true) << "\">";
          entity_ends.push_back(entities[e].start + entities[e].length - 1);
        }

        // The token itself
        cout << "<token>" << xml_encoded(forms[i]) << "</token>";

        // Close entities ending after current token
        while (!entity_ends.empty() && entity_ends.back() == i) {
          cout << "</ne>";
          entity_ends.pop_back();
        }
        if (i + 1 == forms.size()) cout << "</sentence>";
        unprinted = forms[i].str + forms[i].len;
      }
    }
    // Write rest of the text (should be just spaces)
    if (unprinted < para.c_str() + para.size()) cout << xml_encoded(string_piece(unprinted, para.c_str() + para.size() - unprinted));
    cout << flush;
  }
  cerr << "Recognizing done, in " << fixed << setprecision(3) << (clock() - now) / double(CLOCKS_PER_SEC) << " seconds." << endl;

  return 0;
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
