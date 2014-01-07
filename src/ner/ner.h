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
#include "tokenizer/tokenizer.h"
#include "utils/threadsafe_stack.h"

namespace ufal {
namespace nametag {

struct named_entity {
  size_t start;
  size_t length;
  string type;

  named_entity() {}
  named_entity(size_t start, size_t length, const string& type) : start(start), length(length), type(type) {}
};

class EXPORT_ATTRIBUTES ner {
 public:
  virtual ~ner() {}

  static ner* load(FILE* f);
  static ner* load(const char* fname);

  // Perform named entity recognition on a tokenizes sentence and return found
  // named entities in the given vector.
  virtual void recognize(const vector<string_piece>& forms, vector<named_entity>& entities) const = 0;

  // Perform tokenization and named entity recognition and return found named
  // entities in the given vector. Return the entity ranges either in UTF8
  // bytes, Unicode characters or both.
  void tokenize_and_recognize(const char* text, vector<named_entity>* entities_utf8, vector<named_entity>* entities_unicode) const;

 protected:
  virtual tokenizer* new_tokenizer() const = 0;

  struct cache {
    unique_ptr<tokenizer> t;
    vector<string_piece> forms;
    vector<token_range> tokens;
    vector<named_entity> entities;

    cache(const ner& self) : t(self.new_tokenizer()) {}
  };
  mutable threadsafe_stack<cache> caches;
};

} // namespace nametag
} // namespace ufal
