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

#ifndef NAMETAG_H
#define NAMETAG_H

#include <cstdio>
#include <cstring>
#include <string>
#include <vector>

// Import attributes
#if defined(_WIN32) && !defined(NAMETAG_STATIC)
  #define NAMETAG_IMPORT __declspec(dllimport)
#else
  #define NAMETAG_IMPORT
#endif

namespace ufal {
namespace nametag {

struct string_piece {
  const char* str;
  size_t len;

  string_piece() : str(NULL), len(0) {}
  string_piece(const char* str) : str(str), len(strlen(str)) {}
  string_piece(const char* str, size_t len) : str(str), len(len) {}
  string_piece(const std::string& str) : str(str.c_str()), len(str.size()) {}
};

struct token_range {
  size_t start;
  size_t length;

  token_range() {}
  token_range(size_t start, size_t length) : start(start), length(length) {}
};

struct named_entity {
  size_t start;
  size_t length;
  std::string type;

  named_entity() {}
  named_entity(size_t start, size_t length, const std::string& type) : start(start), length(length), type(type) {}
};

class NAMETAG_IMPORT tokenizer {
 public:
  virtual ~tokenizer() {}

  virtual void set_text(string_piece text, bool make_copy = false) = 0;
  virtual bool next_sentence(std::vector<string_piece>* forms, std::vector<token_range>* tokens) = 0;

  // Static factory method
  static tokenizer* new_vertical_tokenizer();
};

class NAMETAG_IMPORT ner {
 public:
  virtual ~ner() {}

  static ner* load(const char* fname);
  static ner* load(FILE* f);

  // Perform named entity recognition on a tokenizes sentence and return found
  // named entities in the given vector.
  virtual void recognize(const std::vector<string_piece>& forms, std::vector<named_entity>& entities) const = 0;

  // Construct a new tokenizer instance appropriate for this recognizer.
  // Can return NULL if no such tokenizer exists.
  virtual tokenizer* new_tokenizer() const = 0;
};

} // namespace nametag
} // namespace ufal

#endif
