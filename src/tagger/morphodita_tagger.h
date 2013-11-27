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

#include <memory>

#include "common.h"
#include "tagger.h"
#include "utils/threadsafe_stack.h"

#include "morphodita.h"

namespace ufal {
namespace nametag {

class morphodita_tagger : public tagger {
 public:
  virtual void tag(const vector<raw_form>& forms, ner_sentence& sentence) const override;

 protected:
  virtual bool load(FILE* f) override;
  virtual bool create_and_encode(const string& params, FILE* f) override;

 private:
  unique_ptr<ufal::morphodita::tagger> tagger;
  const ufal::morphodita::morpho* morpho;

  struct cache {
    vector<ufal::morphodita::raw_form> forms;
    vector<ufal::morphodita::tagged_lemma> tags;
  };
  mutable threadsafe_stack<cache> caches;
};

} // namespace nametag
} // namespace ufal
