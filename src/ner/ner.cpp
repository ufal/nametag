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

#include "czech_ner.h"
#include "ner.h"
#include "ner_ids.h"
#include "utils/file_ptr.h"
#include "utils/new_unique_ptr.h"

namespace ufal {
namespace nametag {

ner* ner::load(FILE* f) {
  switch (fgetc(f)) {
    case ner_ids::CZECH_NER:
      {
        auto res = new_unique_ptr<czech_ner>();
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

void ner::tokenize_and_recognize(const char* text, vector<named_entity>* entities_utf8, vector<named_entity>* entities_unicode) const {
  if (!entities_utf8 && !entities_unicode) return;

  if (entities_utf8) entities_utf8->clear();
  if (entities_unicode) entities_unicode->clear();

  cache* c = caches.pop();
  if (!c) c = new cache(*this);

  c->t->set_text(text);
  while (c->t->next_sentence(&c->forms, entities_unicode ? &c->tokens : nullptr)) {
    recognize(c->forms, c->entities);
    for (auto& entity : c->entities) {
      if (entities_utf8)
        entities_utf8->emplace_back(c->forms[entity.start].str - text, c->forms[entity.start + entity.length - 1].str - text +
                                    c->forms[entity.start + entity.length - 1].len - (c->forms[entity.start].str - text), entity.type);
      if (entities_unicode)
        entities_unicode->emplace_back(c->tokens[entity.start].start, c->tokens[entity.start + entity.length - 1].start +
                                       c->tokens[entity.start + entity.length - 1].length - c->tokens[entity.start].start, entity.type);
    }
  }

  caches.push(c);
}

} // namespace nametag
} // namespace ufal
