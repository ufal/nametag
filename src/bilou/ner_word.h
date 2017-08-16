// This file is part of NameTag <http://github.com/ufal/nametag/>.
//
// Copyright 2016 Institute of Formal and Applied Linguistics, Faculty of
// Mathematics and Physics, Charles University in Prague, Czech Republic.
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#pragma once

#include "common.h"

namespace ufal {
namespace nametag {

struct ner_word {
  string form;
  string raw_lemma;
  vector<string> raw_lemmas_all;
  string lemma_id;
  string lemma_comments;
  string tag;

  ner_word() {}
  ner_word(const string& form) : form(form) {}
};

} // namespace nametag
} // namespace ufal
