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

#include "network_classifier.h"
#include "utils/compressor.h"

namespace ufal {
namespace nametag {

bool network_classifier::save(FILE* f) {
  binary_encoder enc;

  // Direct connections
  save_matrix(enc, indices);
  enc.add_double(missing_weight);
  save_matrix(enc, weights);

  // Hidden layer
  enc.add_2B(hidden_layer.size());
  if (!hidden_layer.empty()) {
    save_matrix(enc, hidden_weights[0]);
    save_matrix(enc, hidden_weights[1]);
  }

  // Output layer
  enc.add_2B(output_layer.size());

  return compressor::save(f, enc);
}

template <class T>
void network_classifier::save_matrix(binary_encoder& enc, const vector<vector<T>>& m) {
  enc.add_4B(m.size());
  for (auto&& row : m) {
    enc.add_2B(row.size());
    enc.add_data((const unsigned char*) row.data(), ((const unsigned char*) row.data()) + row.size() * sizeof(T));
  }
}

} // namespace nametag
} // namespace ufal
