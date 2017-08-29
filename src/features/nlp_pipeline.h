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
#include "tagger/tagger.h"
#include "tokenizer/tokenizer.h"

namespace ufal {
namespace nametag {

struct nlp_pipeline {
  ufal::nametag::tokenizer* tokenizer;
  const ufal::nametag::tagger* tagger;

  nlp_pipeline(ufal::nametag::tokenizer* tokenizer, const ufal::nametag::tagger* tagger) : tokenizer(tokenizer), tagger(tagger) {}
};

} // namespace nametag
} // namespace ufal
