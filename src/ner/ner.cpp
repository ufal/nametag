// This file is part of NameTag <http://github.com/ufal/nametag/>.
//
// Copyright 2016 Institute of Formal and Applied Linguistics, Faculty of
// Mathematics and Physics, Charles University in Prague, Czech Republic.
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#include <fstream>

#include "bilou_ner.h"
#include "ner.h"
#include "ner_ids.h"

namespace ufal {
namespace nametag {

ner* ner::load(istream& is) {
  ner_id id = ner_id(is.get());
  switch (id) {
    case ner_ids::CZECH_NER:
    case ner_ids::ENGLISH_NER:
    case ner_ids::GENERIC_NER:
      {
        unique_ptr<bilou_ner> res(new bilou_ner(id));
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
