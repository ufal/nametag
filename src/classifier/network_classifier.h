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

#include "common.h"
#include "classifier_instance.h"
#include "network_parameters.h"
#include "utils/binary_decoder.h"
#include "utils/binary_encoder.h"

namespace ufal {
namespace nametag {

class network_classifier {
 public:
  bool load(istream& is);
  bool save(ostream& os);

  bool train(unsigned features, unsigned outcomes, const vector<classifier_instance>& train,
             const vector<classifier_instance>& heldout, const network_parameters& parameters, bool verbose);

  void classify(const classifier_features& features, vector<double>& outcomes, vector<double>& buffer) const;

 private:
  // Direct connections
  vector<vector<float>> weights;
  vector<vector<uint32_t>> indices;
  double missing_weight;

  // Hidden layer, experimental use only
  vector<vector<float>> hidden_weights[2];
  vector<double> hidden_layer, hidden_error;

  // Output layer
  vector<double> output_layer, output_error;

  inline void propagate(const classifier_features& features);
  inline void propagate(const classifier_features& features, vector<double>& hidden_layer, vector<double>& output_layer) const;
  inline void backpropagate(const classifier_instance& instance, double learning_rate, double gaussian_sigma);
  inline classifier_outcome best_outcome();

  template<class T> void load_matrix(binary_decoder& data, vector<vector<T>>& m);
  template<class T> void save_matrix(binary_encoder& enc, const vector<vector<T>>& m);
};

} // namespace nametag
} // namespace ufal
