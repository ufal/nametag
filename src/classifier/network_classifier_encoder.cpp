// This file is part of NameTag <http://github.com/ufal/nametag/>.
//
// Copyright 2016 Institute of Formal and Applied Linguistics, Faculty of
// Mathematics and Physics, Charles University in Prague, Czech Republic.
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#include "network_classifier.h"
#include "utils/compressor.h"

namespace ufal {
namespace nametag {

bool network_classifier::save(ostream& os) {
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

  return compressor::save(os, enc);
}

template <class T>
void network_classifier::save_matrix(binary_encoder& enc, const vector<vector<T>>& m) {
  enc.add_4B(m.size());
  for (auto&& row : m) {
    enc.add_2B(row.size());
    enc.add_data(row.data(), row.size());
  }
}

} // namespace nametag
} // namespace ufal
