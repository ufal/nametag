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

#include <algorithm>
#include <cmath>
#include <cstring>

#include "distribution.h"
#include "network_classifier.h"
#include "utils/compressor.h"

namespace ufal {
namespace nametag {

bool network_classifier::load(FILE* f) {
  binary_decoder data;
  if (!compressor::load(f, data)) return false;

  try {
    load_matrix(data, indices);
    missing_weight = *data.next<double>(1);
    load_matrix(data, weights);

    unsigned outcomes = data.next_2B();
    output_layer.resize(outcomes);
    output_error.resize(outcomes);
  } catch (binary_decoder_error&) {
    return false;
  }

  return data.is_end();
}

template<class T>
void network_classifier::load_matrix(binary_decoder& data, vector<vector<T>>& m) {
  m.resize(data.next_4B());
  for (auto& row : m) {
    row.resize(data.next_2B());
    memcpy((unsigned char*) row.data(), data.next<T>(row.size()), row.size() * sizeof(T));
  }
}

bool network_classifier::train(unsigned features, unsigned outcomes, const vector<classifier_instance>& train,
                               const vector<classifier_instance>& heldout, const network_parameters& parameters, bool verbose) {
  // Assertions
  if (features <= 0) { if (verbose) eprintf("There must be more than zero features!\n"); return false; }
  if (outcomes <= 0) { if (verbose) eprintf("There must be more than zero features!\n"); return false; }
  if (train.empty()) { if (verbose) eprintf("No training data!\n"); return false; }
  for (auto& instance : train) {
    if (instance.outcome >= outcomes) { if (verbose) eprintf("Training instances out of range!\n"); return false; }
    for(auto& feature : instance.features)
      if (feature >= features) { if (verbose) eprintf("Training instances out of range!\n"); return false; }
  }
  for (auto& instance : heldout)
    for(auto& feature : instance.features)
      if (feature >= features) { if (verbose) eprintf("Heldout instances out of range!\n"); return false; }

  distribution d(42);

  // Compute indices from existing feature-outcome pairs
  indices.clear();
  indices.resize(features);
  for (auto& instance : train)
    for (auto& feature : instance.features)
      indices[feature].emplace_back(instance.outcome);

  for (auto& row : indices) {
    sort(row.begin(), row.end());
    row.resize(unique(row.begin(), row.end()) - row.begin());
  }

  // Initialize weight matrix and output vectors
  weights.clear();
  for (auto& row : indices)
    weights.emplace_back(row.size());
  missing_weight = parameters.missing_weight;
  output_layer.resize(outcomes);
  output_error.resize(outcomes);

  // Normalize gaussian_sigma
  double gaussian_sigma = parameters.gaussian_sigma / train.size();

  // Train
  vector<int> permutation;
  for (int iteration = 0; iteration < parameters.iterations; iteration++) {
    if (verbose) eprintf("Iteration %d: ", iteration + 1);

    double learning_rate = parameters.final_learning_rate && parameters.iterations > 1 ?
        exp(((parameters.iterations - 1 - iteration) * log(parameters.initial_learning_rate) + iteration * log(parameters.final_learning_rate)) / (parameters.iterations-1)) :
        parameters.initial_learning_rate;
    double logprob = 0;
    int training_correct = 0;

    // Process instances in random order
    d.permutation(0, train.size(), permutation);
    for (auto& train_index : permutation) {
      auto& instance = train[train_index];
      propagate(instance.features);

      // Update logprob and training_correct
      logprob += log(output_layer[instance.outcome]);
      training_correct += best_outcome() == instance.outcome;

      // Improve network weights according to correct outcome
      backpropagate(instance, learning_rate, gaussian_sigma);
    }
    if (verbose) eprintf("a %.3f, logprob %.4e, training acc %.2f%%, ", learning_rate, logprob, training_correct * 100. / train.size());

    // Evaluate heldout accuracy if heldout data are present
    if (!heldout.empty()) {
      int heldout_correct = 0;
      for (auto& instance : heldout) {
        propagate(instance.features);
        heldout_correct += best_outcome() == instance.outcome;
      }
      if (verbose) eprintf("heldout acc %.2f%%, ", heldout_correct * 100. / heldout.size());
    }
    if (verbose) eprintf("done.\n");
  }

//  int cnt = 0;
//  for (auto& instance : train) {
//    propagate(instance.features);
//
//    for (auto& feature : instance.features) eprintf("%u ", feature);
//    unsigned B_best = 0; double B_best_prob = output_layer[3];
//    unsigned U_best = 0; double U_best_prob = output_layer[4];
//    for (unsigned i = 5; i < output_layer.size(); i += 2) {
//      if (output_layer[i] > B_best_prob) B_best = (i - 3) / 2, B_best_prob = output_layer[i];
//      if (output_layer[i + 1] > U_best_prob) U_best = (i - 3) / 2, U_best_prob = output_layer[i + 1];
//    }
//    eprintf("B%.2g(%u)", B_best_prob, B_best);
//    eprintf(" I%.2g", output_layer[0]);
//    eprintf(" L%.2g", output_layer[1]);
//    eprintf(" O%.2g", output_layer[2]);
//    eprintf(" U%.2g(%u)\n", U_best_prob, U_best);
//    if (++cnt > 100) break;
//  }

  return true;
}

unsigned network_classifier::outcomes() const {
  return output_layer.size();
}

void network_classifier::classify(const classifier_features& features, vector<double>& outcomes) const {
  // Assertions
  assert(outcomes.size() == output_layer.size());
  for (auto& p : features) assert(p < indices.size());

  // Propagation
  propagate(features, outcomes);
}

void network_classifier::propagate(const classifier_features& features) {
  propagate(features, output_layer);
}

void network_classifier::propagate(const classifier_features& features, vector<double>& output_layer) const {
  output_layer.assign(output_layer.size(), features.size() * missing_weight);

  for (auto& feature : features)
    for (unsigned i = 0; i < indices[feature].size(); i++)
      output_layer[indices[feature][i]] += weights[feature][i] - missing_weight;

  // Apply softmax sigmoid to output_layer layer
  double sum = 0;
  for (unsigned i = 0; i < output_layer.size(); sum += output_layer[i], i++)
    output_layer[i] = exp(output_layer[i]);
  sum = 1 / sum;
  for (unsigned i = 0; i < output_layer.size(); i++)
    output_layer[i] *= sum;
}

classifier_outcome network_classifier::best_outcome() {
  classifier_outcome best = 0;
  for (unsigned i = 1; i < output_layer.size(); i++)
    if (output_layer[i] > output_layer[best])
      best = i;

  return best;
}

void network_classifier::backpropagate(const classifier_instance& instance, double learning_rate, double gaussian_sigma) {
  // Compute error vector
  for (unsigned i = 0; i < output_error.size(); i++)
    output_error[i] = (i == instance.outcome) - output_layer[i];

  // Update weights
  for (auto& feature : instance.features)
    for (unsigned i = 0; i < indices[feature].size(); i++)
      weights[feature][i] += learning_rate * output_error[indices[feature][i]] - weights[feature][i] * gaussian_sigma;
}

} // namespace nametag
} // namespace ufal
