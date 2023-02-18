// This file is part of MorphoDiTa <http://github.com/ufal/morphodita/>.
//
// Copyright 2019 Institute of Formal and Applied Linguistics, Faculty of
// Mathematics and Physics, Charles University in Prague, Czech Republic.
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#pragma once

#include "common.h"
#include "czech_tokenizer.h"

namespace ufal {
namespace nametag {
namespace morphodita {

class czech_tokenizer_factory_encoder {
 public:
  static void encode(czech_tokenizer::tokenizer_language language, unsigned version, ostream& os);
};

} // namespace morphodita
} // namespace nametag
} // namespace ufal
