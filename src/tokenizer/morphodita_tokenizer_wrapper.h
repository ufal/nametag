// This file is part of NameTag <http://github.com/ufal/nametag/>.
//
// Copyright 2017 Institute of Formal and Applied Linguistics, Faculty of
// Mathematics and Physics, Charles University in Prague, Czech Republic.
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#pragma once

#include "common.h"
#include "tokenizer.h"

#include "morphodita/tokenizer/tokenizer.h"

namespace ufal {
namespace nametag {

class morphodita_tokenizer_wrapper : public tokenizer {
 public:
  morphodita_tokenizer_wrapper(morphodita::tokenizer* morphodita_tokenizer)
      : morphodita_tokenizer(morphodita_tokenizer) {}
  virtual ~morphodita_tokenizer_wrapper() override {}

  virtual void set_text(string_piece text, bool make_copy = false) override;
  virtual bool next_sentence(vector<string_piece>* forms, vector<token_range>* tokens) override;

 private:
  unique_ptr<morphodita::tokenizer> morphodita_tokenizer;
};

} // namespace nametag
} // namespace ufal
