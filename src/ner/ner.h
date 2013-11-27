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

namespace ufal {
namespace nametag {

struct EXPORT_ATTRIBUTES raw_form {
  const char* form;
  int form_len;

  raw_form(const char* form, int form_len) : form(form), form_len(form_len) {}
};

struct EXPORT_ATTRIBUTES named_entity {
  int start;
  int len;
  string type;

  named_entity(int start, int len, const string& type) : start(start), len(len), type(type) {}
};

class EXPORT_ATTRIBUTES ner {
 public:
  virtual ~ner() {}

  static ner* load(FILE* f);
  static ner* load(const char* fname);

  virtual void recognize(const vector<raw_form>& forms, vector<named_entity>& entities) const = 0;
};

} // namespace nametag
} // namespace ufal
