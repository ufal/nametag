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

#include "external_tagger.h"
#include "utils/small_stringops.h"

namespace ufal {
namespace nametag {

bool external_tagger::load(FILE* /*f*/) {
  return true;
}

bool external_tagger::create_and_encode(const string& /*params*/, FILE* /*f*/) {
  return true;
}

void external_tagger::tag(const vector<string_piece>& forms, ner_sentence& sentence) const {
  sentence.resize(forms.size());
  for (unsigned i = 0; i < forms.size(); i++) {
    string_piece form = forms[i];

    size_t tab = small_strnchrpos(form.str, '\t', form.len);
    if (tab < form.len) {
      sentence.words[i].form.assign(form.str, tab);
      form.len -= tab + 1;
      form.str += tab + 1;

      tab = small_strnchrpos(form.str, '\t', form.len);
      if (tab < form.len) {
        sentence.words[i].raw_lemma.assign(form.str, tab);
        form.len -= tab + 1;
        form.str += tab + 1;

        sentence.words[i].tag.assign(form.str, small_strnchrpos(form.str, '\t', form.len));
      } else {
        sentence.words[i].raw_lemma.assign(form.str, form.len);
        sentence.words[i].tag.clear();
      }
    } else {
      sentence.words[i].form.assign(form.str, form.len);
      sentence.words[i].raw_lemma = sentence.words[i].form;
      sentence.words[i].tag.clear();
    }
    sentence.words[i].lemma_id = sentence.words[i].raw_lemma;
    sentence.words[i].lemma_comments.clear();
  }
}

} // namespace nametag
} // namespace ufal
