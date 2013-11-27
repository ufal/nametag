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

#include "trivial_tagger.h"

namespace ufal {
namespace nametag {

bool trivial_tagger::load(FILE* /*f*/) {
  return true;
}

bool trivial_tagger::create_and_encode(const string& /*params*/, FILE* /*f*/) {
  return true;
}

void trivial_tagger::tag(const vector<raw_form>& forms, ner_sentence& sentence) const {
  sentence.resize(forms.size());
  for (unsigned i = 0; i < forms.size(); i++) {
    sentence.words[i].form.assign(forms[i].form, forms[i].form_len);
    sentence.words[i].raw_lemma = sentence.words[i].form;
    sentence.words[i].lemma_id = sentence.words[i].form;
    sentence.words[i].lemma_comments.clear();
    sentence.words[i].tag.clear();
  }
}

} // namespace nametag
} // namespace ufal
