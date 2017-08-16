// This file is part of NameTag <http://github.com/ufal/nametag/>.
//
// Copyright 2016 Institute of Formal and Applied Linguistics, Faculty of
// Mathematics and Physics, Charles University in Prague, Czech Republic.
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#pragma once

#include "common.h"
#include "morphodita/tagger/tagger.h"
#include "tagger.h"
#include "utils/threadsafe_stack.h"


namespace ufal {
namespace nametag {

class morphodita_tagger : public tagger {
 public:
  virtual void tag(const vector<string_piece>& forms, ner_sentence& sentence) const override;

 protected:
  virtual bool load(istream& is) override;
  virtual bool create_and_encode(const string& params, ostream& os) override;

 private:
  unique_ptr<morphodita::tagger> tagger;
  const morphodita::morpho* morpho;

  struct cache {
    vector<morphodita::tagged_lemma> tags, analyses;
    string lemma_cased;
  };
  mutable threadsafe_stack<cache> caches;
};

} // namespace nametag
} // namespace ufal
