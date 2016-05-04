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
#include "tagger.h"

namespace ufal {
namespace nametag {

class trivial_tagger : public tagger {
 public:
  virtual bool load(istream& is) override;
  virtual bool create_and_encode(const string& params, ostream& os) override;
  virtual void tag(const vector<string_piece>& forms, ner_sentence& sentence) const override;
};

} // namespace nametag
} // namespace ufal
