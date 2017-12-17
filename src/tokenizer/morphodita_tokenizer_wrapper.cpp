// This file is part of NameTag <http://github.com/ufal/nametag/>.
//
// Copyright 2017 Institute of Formal and Applied Linguistics, Faculty of
// Mathematics and Physics, Charles University in Prague, Czech Republic.
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#include "morphodita_tokenizer_wrapper.h"

namespace ufal {
namespace nametag {

void morphodita_tokenizer_wrapper::set_text(string_piece text, bool make_copy) {
  morphodita_tokenizer->set_text(text, make_copy);
}

bool morphodita_tokenizer_wrapper::next_sentence(vector<string_piece>* forms, vector<token_range>* tokens) {
  return morphodita_tokenizer->next_sentence(forms, (vector<morphodita::token_range>*) tokens);
}

} // namespace nametag
} // namespace ufal
