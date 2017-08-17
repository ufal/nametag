// This file is part of NameTag <http://github.com/ufal/nametag/>.
//
// Copyright 2016 Institute of Formal and Applied Linguistics, Faculty of
// Mathematics and Physics, Charles University in Prague, Czech Republic.
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

%include "nametag_stl.i"

%{
#include "nametag.h"
using namespace ufal::nametag;
%}

%template(Ints) std::vector<int>;
typedef std::vector<int> Ints;

%template(Forms) std::vector<std::string>;
typedef std::vector<std::string> Forms;

%rename(TokenRange) token_range;
struct token_range {
  size_t start;
  size_t length;
};
%template(TokenRanges) std::vector<token_range>;
typedef std::vector<token_range> TokenTanges;

%rename(NamedEntity) named_entity;
struct named_entity {
  size_t start;
  size_t length;
  std::string type;

  named_entity() {}
  named_entity(size_t start, size_t length, const std::string& type) : start(start), length(length), type(type) {}
};
%template(NamedEntities) std::vector<named_entity>;
typedef std::vector<named_entity> NamedEntities;

%rename(Version) version;
class version {
 public:
  unsigned major;
  unsigned minor;
  unsigned patch;
  std::string prerelease;

  static version current();
};

%rename(Tokenizer) tokenizer;
%nodefaultctor tokenizer;
class tokenizer {
 public:
  virtual ~tokenizer() {}

  %extend {
    %rename(setText) set_text;
    void set_text(const char* text) {
      $self->set_text(text, true);
    }

    %rename(nextSentence) next_sentence;
    bool next_sentence(std::vector<std::string>* forms, std::vector<token_range>* tokens) {
      if (!forms) return $self->next_sentence(NULL, tokens);

      std::vector<string_piece> string_pieces;
      bool result = $self->next_sentence(&string_pieces, tokens);
      forms->resize(string_pieces.size());
      for (unsigned i = 0; i < string_pieces.size(); i++)
        forms->operator[](i).assign(string_pieces[i].str, string_pieces[i].len);
      return result;
    }
  }

  %rename(newVerticalTokenizer) new_vertical_tokenizer;
  %newobject new_vertical_tokenizer;
  static tokenizer* new_vertical_tokenizer();
};

%rename(Ner) ner;
%nodefaultctor ner;
class ner {
 public:
  virtual ~ner() {}

  %newobject load;
  static ner* load(const char* fname);

  %extend {
    void recognize(const std::vector<std::string>& forms, std::vector<named_entity>& entities) const {
      std::vector<string_piece> string_pieces;
      string_pieces.reserve(forms.size());
      for (auto&& form : forms)
        string_pieces.emplace_back(form);
      $self->recognize(string_pieces, entities);
    }
  }

  %rename(entityTypes) entity_types;
  virtual void entity_types(std::vector<std::string>& types) const;

  virtual void gazetteers(std::vector<std::string>& gazetteers, std::vector<int>* gazetteer_types) const;

  %rename(newTokenizer) new_tokenizer;
  %newobject new_tokenizer;
  virtual tokenizer* new_tokenizer() const;
};
