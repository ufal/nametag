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
