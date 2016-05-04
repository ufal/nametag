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
