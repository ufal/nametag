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
#include "bilou_ner.h"

namespace ufal {
namespace nametag {

class english_ner : public bilou_ner {
 protected:
  virtual tokenizer* new_tokenizer() const override;
};

} // namespace nametag
} // namespace ufal
