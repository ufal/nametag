// This file is part of NameTag <http://github.com/ufal/nametag/>.
//
// Copyright 2016 Institute of Formal and Applied Linguistics, Faculty of
// Mathematics and Physics, Charles University in Prague, Czech Republic.
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#include <algorithm>
#include <fstream>

#include "morphodita_tagger.h"
#include "unilib/unicode.h"
#include "unilib/utf8.h"

namespace ufal {
namespace nametag {

bool morphodita_tagger::load(istream& is) {
  tagger.reset(morphodita::tagger::load(is));
  morpho = tagger ? tagger->get_morpho() : nullptr;
  return tagger && morpho;
}

bool morphodita_tagger::create_and_encode(const string& params, ostream& os) {
  if (params.empty()) return cerr << "Missing tagger_file argument to morphodita_tagger!" << endl, false;

  ifstream in(params, ifstream::in | ifstream::binary);
  if (!in.is_open()) return cerr << "Cannot open morphodita tagger file '" << params << "'!" << endl, false;
  if (!load(in)) return cerr << "Cannot load morphodita tagger from file '" << params << "'!" << endl, false;

  if (!in.seekg(0, ifstream::beg)) return cerr << "Cannot seek in morphodita tagger file '" << params << "'!" << endl, false;
  os << in.rdbuf();

  return bool(os);
}

void morphodita_tagger::tag(const vector<string_piece>& forms, ner_sentence& sentence) const {
  sentence.resize(0);
  if (!tagger || !morpho) return;

  // Obtain a cache
  cache* c = caches.pop();
  if (!c) c = new cache();

  // Tag
  tagger->tag(forms, c->tags);

  // Fill sentence
  if (c->tags.size() >= forms.size()) {
    sentence.resize(forms.size());
    for (unsigned i = 0; i < forms.size(); i++) {
      sentence.words[i].form.assign(forms[i].str, morpho->raw_form_len(forms[i]));

      const string& lemma = c->tags[i].lemma;

      unsigned raw_lemma_len = morpho->raw_lemma_len(lemma);
      sentence.words[i].raw_lemma.assign(lemma, 0, raw_lemma_len);

      morpho->analyze(forms[i], morphodita::morpho::GUESSER, c->analyses);
      sentence.words[i].raw_lemmas_all.clear();
      for (auto&& analysis : c->analyses)
        sentence.words[i].raw_lemmas_all.emplace_back(analysis.lemma, 0, morpho->raw_lemma_len(analysis.lemma));
      sort(sentence.words[i].raw_lemmas_all.begin(), sentence.words[i].raw_lemmas_all.end());
      sentence.words[i].raw_lemmas_all.erase(unique(sentence.words[i].raw_lemmas_all.begin(), sentence.words[i].raw_lemmas_all.end()),
                                             sentence.words[i].raw_lemmas_all.end());

      unsigned lemma_id_len = morpho->lemma_id_len(lemma);
      sentence.words[i].lemma_id.assign(sentence.words[i].raw_lemma).append(lemma, raw_lemma_len, lemma_id_len - raw_lemma_len);
      sentence.words[i].lemma_comments.assign(lemma, lemma_id_len, string::npos);
      sentence.words[i].tag = c->tags[i].tag;
    }
  }

  caches.push(c);
}

} // namespace nametag
} // namespace ufal
