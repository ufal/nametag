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

#include "morphodita_tagger.h"
#include "utils/file_ptr.h"

namespace ufal {
namespace nametag {

bool morphodita_tagger::load(FILE* f) {
  tagger.reset(ufal::morphodita::tagger::load(f));
  morpho = tagger ? tagger->get_morpho() : nullptr;
  return tagger && morpho;
}

bool morphodita_tagger::create_and_encode(const string& params, FILE* f) {
  if (params.empty()) return eprintf("Missing tagger_file argument to morphodita_tagger!\n"), false;

  file_ptr in = fopen(params.c_str(), "rb");
  if (!in) return eprintf("Cannot open morphodita tagger file '%s'!\n", params.c_str()), false;

  if (!load(in)) {
    eprintf("Cannot load morphodita tagger fromf ile '%s'!\n", params.c_str());
    return false;
  }

  long in_pos = ftell(in);
  if (fseek(in, 0, SEEK_SET) != 0) return eprintf("Cannot seek in morphodita tagger file '%s'!\n", params.c_str()), false;

  while (in_pos--) fputc(fgetc(in), f);
  return true;
}

void morphodita_tagger::tag(const vector<raw_form>& forms, ner_sentence& sentence) const {
  sentence.resize(0);
  if (!tagger || !morpho) return;

  // Obtain a cache
  cache* c = caches.pop();
  if (!c) c = new cache();

  // Tag
  c->forms.clear();
  for (auto& form : forms)
    c->forms.emplace_back(form.form, form.form_len);
  tagger->tag(c->forms, c->tags);

  // Fill sentence
  if (c->tags.size() >= forms.size()) {
    sentence.resize(forms.size());
    for (unsigned i = 0; i < forms.size(); i++) {
      sentence.words[i].form.assign(forms[i].form, forms[i].form_len);
      const string& lemma = c->tags[i].lemma;
      unsigned raw_lemma_len = morpho->raw_lemma_len(lemma.c_str(), lemma.size());
      sentence.words[i].raw_lemma = raw_lemma_len == lemma.size() ? lemma : lemma.substr(0, raw_lemma_len);
      unsigned lemma_id_len = morpho->lemma_id_len(lemma.c_str(), lemma.size());
      sentence.words[i].lemma_id = lemma_id_len == lemma.size() ? lemma : lemma.substr(0, lemma_id_len);
      sentence.words[i].lemma_comments = lemma_id_len == lemma.size() ? string() : lemma.substr(lemma_id_len);
      sentence.words[i].tag = c->tags[i].tag;
    }
  }

  caches.push(c);
}

} // namespace nametag
} // namespace ufal
