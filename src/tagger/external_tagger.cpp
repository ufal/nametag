// This file is part of NameTag <http://github.com/ufal/nametag/>.
//
// Copyright 2016 Institute of Formal and Applied Linguistics, Faculty of
// Mathematics and Physics, Charles University in Prague, Czech Republic.
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#include "external_tagger.h"

namespace ufal {
namespace nametag {

inline static size_t strnchrpos(const char* str, char c, size_t len) {
  size_t pos = 0;
  for (; len--; str++, pos++)
    if (*str == c)
      return pos;

  return pos;
}

bool external_tagger::load(istream& /*is*/) {
  return true;
}

bool external_tagger::create_and_encode(const string& /*params*/, ostream& /*os*/) {
  return true;
}

void external_tagger::tag(const vector<string_piece>& forms, ner_sentence& sentence) const {
  sentence.resize(forms.size());
  for (unsigned i = 0; i < forms.size(); i++) {
    string_piece form = forms[i];

    size_t space = strnchrpos(form.str, ' ', form.len);
    if (space < form.len) {
      sentence.words[i].form.assign(form.str, space);
      form.len -= space + 1;
      form.str += space + 1;

      space = strnchrpos(form.str, ' ', form.len);
      if (space < form.len) {
        sentence.words[i].raw_lemma.assign(form.str, space);
        form.len -= space + 1;
        form.str += space + 1;

        sentence.words[i].tag.assign(form.str, strnchrpos(form.str, ' ', form.len));
      } else {
        sentence.words[i].raw_lemma.assign(form.str, form.len);
        sentence.words[i].tag.clear();
      }
    } else {
      sentence.words[i].form.assign(form.str, form.len);
      sentence.words[i].raw_lemma = sentence.words[i].form;
      sentence.words[i].tag.clear();
    }
    sentence.words[i].raw_lemmas_all.assign(1, sentence.words[i].raw_lemma);
    sentence.words[i].lemma_id = sentence.words[i].raw_lemma;
    sentence.words[i].lemma_comments.clear();
  }
}

} // namespace nametag
} // namespace ufal
