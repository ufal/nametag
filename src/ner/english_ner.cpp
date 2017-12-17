// This file is part of NameTag <http://github.com/ufal/nametag/>.
//
// Copyright 2016 Institute of Formal and Applied Linguistics, Faculty of
// Mathematics and Physics, Charles University in Prague, Czech Republic.
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#include "english_ner.h"

#include "tokenizer/morphodita_tokenizer_wrapper.h"

namespace ufal {
namespace nametag {

tokenizer* english_ner::new_tokenizer() const {
  return new morphodita_tokenizer_wrapper(morphodita::tokenizer::new_english_tokenizer());
}

} // namespace nametag
} // namespace ufal
