// This file is part of NameTag <http://github.com/ufal/nametag/>.
//
// Copyright 2016 Institute of Formal and Applied Linguistics, Faculty of
// Mathematics and Physics, Charles University in Prague, Czech Republic.
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#include <fstream>

#include "czech_ner.h"
#include "english_ner.h"
#include "generic_ner.h"
#include "ner.h"
#include "ner_ids.h"

namespace ufal {
namespace nametag {

ner* ner::load(istream& is) {
  switch (is.get()) {
    case ner_ids::CZECH_NER:
      {
        unique_ptr<czech_ner> res(new czech_ner());
        if (res->load(is)) return res.release();
        break;
      }
    case ner_ids::ENGLISH_NER:
      {
        unique_ptr<english_ner> res(new english_ner());
        if (res->load(is)) return res.release();
        break;
      }
    case ner_ids::GENERIC_NER:
      {
        unique_ptr<generic_ner> res(new generic_ner());
        if (res->load(is)) return res.release();
        break;
      }
  }

  return nullptr;
}

ner* ner::load(const char* fname) {
  ifstream in(fname, ifstream::in | ifstream::binary);
  if (!in.is_open()) return nullptr;

  return load(in);
}

} // namespace nametag
} // namespace ufal
