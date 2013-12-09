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

#pragma once

#include "common.h"
#include "tokenizer.h"

namespace ufal {
namespace nametag {

class czech_tokenizer : public tokenizer {
 public:
  virtual void set_text(const char* text) override;

  virtual bool next_sentence(vector<string_piece>* forms, vector<token_range>* tokens) override;

 private:
  bool split_hyphenated_words;

  const char* text;
  size_t chars;
};

} // namespace nametag
} // namespace ufal
