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

#include "external_tagger.h"
#include "morphodita_tagger.h"
#include "tagger.h"
#include "tagger_ids.h"
#include "trivial_tagger.h"

namespace ufal {
namespace nametag {

tagger* tagger::load_instance(FILE* f) {
  unique_ptr<tagger> res(create(tagger_id(fgetc(f))));

  if (!res) return nullptr;
  if (!res->load(f)) return nullptr;

  return res.release();
}

tagger* tagger::create_and_encode_instance(const string& tagger_id_and_params, FILE* f) {
  string tagger_id, params;

  // Split the id and params using optional :
  auto colon = tagger_id_and_params.find(':');
  if (colon == string::npos) {
    tagger_id = tagger_id_and_params;
  } else {
    tagger_id = tagger_id_and_params.substr(0, colon);
    params = tagger_id_and_params.substr(colon + 1);
  }

  // Parse tagger_id
  tagger_ids::tagger_id id;
  if (!tagger_ids::parse(tagger_id, id)) return eprintf("Unknown tagger_id '%s'!\n", tagger_id.c_str()), nullptr;

  // Create instance
  unique_ptr<tagger> res(create(id));
  if (!res) return eprintf("Cannot create instance for tagger_id '%s'!\n", tagger_id.c_str()), nullptr;

  // Load and encode the tagger
  fputc(id, f);
  if (!res->create_and_encode(params, f)) return eprintf("Cannot encode instance of tagger_id '%s'!\n", tagger_id.c_str()), nullptr;

  return res.release();
}

tagger* tagger::create(tagger_id id) {
  switch (id) {
    case tagger_ids::TRIVIAL:
      return new trivial_tagger();
    case tagger_ids::EXTERNAL:
      return new external_tagger();
    case tagger_ids::MORPHODITA:
      return new morphodita_tagger();
  }

  return nullptr;
}

} // namespace nametag
} // namespace ufal
