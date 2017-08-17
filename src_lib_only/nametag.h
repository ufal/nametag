// This file is part of NameTag <http://github.com/ufal/nametag/>.
//
// Copyright 2016 Institute of Formal and Applied Linguistics, Faculty of
// Mathematics and Physics, Charles University in Prague, Czech Republic.
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#ifndef NAMETAG_H
#define NAMETAG_H

#include <cstdio>
#include <cstring>
#include <string>
#include <vector>

namespace ufal {
namespace nametag {

namespace utils {
struct string_piece {
  const char* str;
  size_t len;

  string_piece() : str(NULL), len(0) {}
  string_piece(const char* str) : str(str), len(strlen(str)) {}
  string_piece(const char* str, size_t len) : str(str), len(len) {}
  string_piece(const std::string& str) : str(str.c_str()), len(str.size()) {}
};
}
typedef utils::string_piece string_piece;

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

class version {
 public:
  unsigned major;
  unsigned minor;
  unsigned patch;
  std::string prerelease;

  // Returns current NameTag version.
  static version current();
};

class tokenizer {
 public:
  virtual ~tokenizer() {}

  virtual void set_text(string_piece text, bool make_copy = false) = 0;
  virtual bool next_sentence(std::vector<string_piece>* forms, std::vector<token_range>* tokens) = 0;

  // Static factory method
  static tokenizer* new_vertical_tokenizer();
};

class ner {
 public:
  virtual ~ner() {}

  static ner* load(const char* fname);
  static ner* load(FILE* f);

  // Perform named entity recognition on a tokenizes sentence and return found
  // named entities in the given vector.
  virtual void recognize(const std::vector<string_piece>& forms, std::vector<named_entity>& entities) const = 0;

  // Return the possible entity types
  virtual void entity_types(std::vector<std::string>& types) const = 0;

  // Return gazetteers used by the recognizer, if any, optionally with the index of entity type
  virtual void gazetteers(std::vector<std::string>& gazetteers, std::vector<int>* gazetteer_types) const = 0;

  // Construct a new tokenizer instance appropriate for this recognizer.
  // Can return NULL if no such tokenizer exists.
  virtual tokenizer* new_tokenizer() const = 0;
};

} // namespace nametag
} // namespace ufal

#endif
