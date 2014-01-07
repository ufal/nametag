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

%rename(NamedEntity) named_entity;
struct named_entity {
  int start;
  int length;
  std::string type;

  named_entity() {}
  named_entity(size_t start, size_t length, const std::string& type) : start(start), length(length), type(type) {}
};
%template(NamedEntities) std::vector<named_entity>;
typedef std::vector<named_entity> NamedEntities;


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
      for (auto& form : forms)
        string_pieces.emplace_back(form);
      $self->recognize(string_pieces, entities);
    }

    %rename(tokenizeAndRecognize) tokenize_and_recognize;
    void tokenize_and_recognize(const char* text, std::vector<named_entity>& entities) const {
      $self->tokenize_and_recognize(text, entities, true);
    }
  }
};
