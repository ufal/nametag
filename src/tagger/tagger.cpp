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

#include "external_tagger.h"
#include "morphodita_tagger.h"
#include "tagger.h"
#include "tagger_ids.h"
#include "trivial_tagger.h"

namespace ufal {
namespace nametag {

tagger* tagger::load_instance(istream& is) {
  unique_ptr<tagger> res(create(tagger_id(is.get())));

  if (!res) return nullptr;
  if (!res->load(is)) return nullptr;

  return res.release();
}

tagger* tagger::create_and_encode_instance(const string& tagger_id_and_params, ostream& os) {
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
  if (!tagger_ids::parse(tagger_id, id)) return cerr << "Unknown tagger_id '" << tagger_id << "'!" << endl, nullptr;

  // Create instance
  unique_ptr<tagger> res(create(id));
  if (!res) return cerr << "Cannot create instance for tagger_id '" << tagger_id << "'!" << endl, nullptr;

  // Load and encode the tagger
  os.put(id);
  if (!res->create_and_encode(params, os)) return cerr << "Cannot encode instance of tagger_id '" << tagger_id << "'!" << endl, nullptr;

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
