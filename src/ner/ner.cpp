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

#include <memory>

#include "bilou_ner.h"
#include "ner.h"
#include "ner_ids.h"
#include "utils/file_ptr.h"
#include "utils/new_unique_ptr.h"

namespace ufal {
namespace nametag {

ner* ner::load(FILE* f) {
  switch (fgetc(f)) {
    case ner_ids::BILOU_NER:
      {
        auto res = new_unique_ptr<bilou_ner>();
        if (res->load(f)) return res.release();
        break;
      }
  }

  return nullptr;
}

ner* ner::load(const char* fname) {
  file_ptr f = fopen(fname, "rb");
  if (!f) return nullptr;

  return load(f);
}

} // namespace nametag
} // namespace ufal
