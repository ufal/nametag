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

%module nametag

%{
#include "nametag.h"
using namespace ufal::nametag;
%}

%include "std_string.i"
%include "std_vector.i"

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

  %rename(newTokenizer) new_tokenizer;
  %newobject new_tokenizer;
  virtual tokenizer* new_tokenizer() const;
};
