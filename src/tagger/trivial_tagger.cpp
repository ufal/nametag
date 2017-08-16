// This file is part of NameTag <http://github.com/ufal/nametag/>.
//
// Copyright 2016 Institute of Formal and Applied Linguistics, Faculty of
// Mathematics and Physics, Charles University in Prague, Czech Republic.
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#include "trivial_tagger.h"

namespace ufal {
namespace nametag {

bool trivial_tagger::load(istream& /*is*/) {
  return true;
}

bool trivial_tagger::create_and_encode(const string& /*params*/, ostream& /*os*/) {
  return true;
}

void trivial_tagger::tag(const vector<string_piece>& forms, ner_sentence& sentence) const {
  sentence.resize(forms.size());
  for (unsigned i = 0; i < forms.size(); i++) {
    sentence.words[i].form.assign(forms[i].str, forms[i].len);
    sentence.words[i].raw_lemma = sentence.words[i].form;
    sentence.words[i].raw_lemmas_all.assign(1, sentence.words[i].raw_lemma);
    sentence.words[i].lemma_id = sentence.words[i].form;
    sentence.words[i].lemma_comments.clear();
    sentence.words[i].tag.clear();
  }
}

} // namespace nametag
} // namespace ufal
