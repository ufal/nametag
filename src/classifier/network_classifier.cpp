// This file is part of NameTag <http://github.com/ufal/nametag/>.
//
// Copyright 2016 Institute of Formal and Applied Linguistics, Faculty of
// Mathematics and Physics, Charles University in Prague, Czech Republic.
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#include <algorithm>
#include <cmath>
#include <cstring>
#include <random>

#include "network_classifier.h"
#include "utils/compressor.h"
#include "utils/unaligned_access.h"

namespace ufal {
namespace nametag {

bool network_classifier::load(istream& is) {
  binary_decoder data;
  if (!compressor::load(is, data)) return false;

  try {
    // Direct connections
    load_matrix(data, indices);
    missing_weight = unaligned_load<double>(data.next<double>(1));
    load_matrix(data, weights);

    // Hidden layer
    hidden_weights[0].clear();
    hidden_weights[1].clear();
    hidden_layer.resize(data.next_2B());
    if (!hidden_layer.empty()) {
      load_matrix(data, hidden_weights[0]);
      load_matrix(data, hidden_weights[1]);
    }

    // Output layer
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
  for (auto&& row : m) {
    row.resize(data.next_2B());
    if (!row.empty())
      memcpy((unsigned char*) row.data(), data.next<T>(row.size()), row.size() * sizeof(T));
  }
}

bool network_classifier::train(unsigned features, unsigned outcomes, const vector<classifier_instance>& train,
                               const vector<classifier_instance>& heldout, const network_parameters& parameters, bool verbose) {
  // Assertions
  if (features <= 0) { if (verbose) cerr << "There must be more than zero features!" << endl; return false; }
  if (outcomes <= 0) { if (verbose) cerr << "There must be more than zero features!" << endl; return false; }
  if (train.empty()) { if (verbose) cerr << "No training data!" << endl; return false; }
  for (auto&& instance : train) {
    if (instance.outcome >= outcomes) { if (verbose) cerr << "Training instances out of range!" << endl; return false; }
    for(auto& feature : instance.features)
      if (feature >= features) { if (verbose) cerr << "Training instances out of range!" << endl; return false; }
  }
  for (auto&& instance : heldout)
    for(auto& feature : instance.features)
      if (feature >= features) { if (verbose) cerr << "Heldout instances out of range!" << endl; return false; }

  mt19937 generator(42);
  uniform_real_distribution<float> uniform(-0.1, 0.1);

  // Compute indices from existing feature-outcome pairs
  indices.clear();
  indices.resize(features);
  for (auto&& instance : train)
    for (auto&& feature : instance.features)
      indices[feature].emplace_back(instance.outcome);

  for (auto&& row : indices) {
    sort(row.begin(), row.end());
    row.resize(unique(row.begin(), row.end()) - row.begin());
  }

  // Initialize direct connections
  weights.clear();
  for (auto&& row : indices)
    weights.emplace_back(row.size());
  missing_weight = parameters.missing_weight;

  // Initialize hidden layer
  hidden_layer.resize(parameters.hidden_layer);
  if (!hidden_layer.empty()) {
    hidden_error.resize(hidden_layer.size());

    hidden_weights[0].resize(features);
    for (auto&& row : hidden_weights[0])
      for (auto&& weight : row.resize(hidden_layer.size()), row)
        weight = uniform(generator) + uniform(generator) + uniform(generator);

    hidden_weights[1].resize(hidden_layer.size());
    for (auto&& row : hidden_weights[1])
      for (auto&& weight : row.resize(outcomes), row)
        weight = uniform(generator) + uniform(generator) + uniform(generator);
  }

  // Initialize output layer
  output_layer.resize(outcomes);
  output_error.resize(outcomes);

  // Normalize gaussian_sigma
  double gaussian_sigma = parameters.gaussian_sigma / train.size();

  // Train
  vector<int> permutation;
  for (unsigned i = 0; i < train.size(); i++)
    permutation.push_back(i);

  for (int iteration = 0; iteration < parameters.iterations; iteration++) {
    if (verbose) cerr << "Iteration " << iteration + 1 << ": ";

    double learning_rate = parameters.final_learning_rate && parameters.iterations > 1 ?
        exp(((parameters.iterations - 1 - iteration) * log(parameters.initial_learning_rate) + iteration * log(parameters.final_learning_rate)) / (parameters.iterations-1)) :
        parameters.initial_learning_rate;
    double logprob = 0;
    int training_correct = 0;

    // Process instances in random order
    shuffle(permutation.begin(), permutation.end(), generator);
    for (auto&& train_index : permutation) {
      auto& instance = train[train_index];
      propagate(instance.features);

      // Update logprob and training_correct
      logprob += log(output_layer[instance.outcome]);
      training_correct += best_outcome() == instance.outcome;

      // Improve network weights according to correct outcome
      backpropagate(instance, learning_rate, gaussian_sigma);
    }
    if (verbose)
      cerr << "a " << fixed << setprecision(3) << learning_rate
           << ", logprob " << scientific << setprecision(4) << logprob
           << ", training acc " << fixed << setprecision(2) << training_correct * 100. / train.size()
           << "%, ";

    // Evaluate heldout accuracy if heldout data are present
    if (!heldout.empty()) {
      int heldout_correct = 0;
      for (auto&& instance : heldout) {
        propagate(instance.features);
        heldout_correct += best_outcome() == instance.outcome;
      }
      if (verbose) cerr << "heldout acc " << heldout_correct * 100. / heldout.size() << ", ";
    }
    if (verbose) cerr << "done." << endl;
  }
  return true;
}

void network_classifier::classify(const classifier_features& features, vector<double>& outcomes, vector<double>& buffer) const {
  if (outcomes.size() != output_layer.size()) outcomes.resize(output_layer.size());
  if (buffer.size() != hidden_layer.size()) buffer.resize(hidden_layer.size());

  // Propagation
  propagate(features, buffer, outcomes);
}

void network_classifier::propagate(const classifier_features& features) {
  propagate(features, hidden_layer, output_layer);
}

void network_classifier::propagate(const classifier_features& features, vector<double>& hidden_layer, vector<double>& output_layer) const {
  output_layer.assign(output_layer.size(), features.size() * missing_weight);

  // Direct connections
  for (auto&& feature : features)
    if (feature < indices.size())
      for (unsigned i = 0; i < indices[feature].size(); i++)
        output_layer[indices[feature][i]] += weights[feature][i] - missing_weight;

  // Hidden layer
  if (!hidden_layer.empty()) {
    for (auto&& weight : hidden_layer)
      weight = 0;

    // Propagate to hidden layer
    for (auto&& feature : features)
      if (feature < hidden_weights[0].size())
        for (unsigned i = 0; i < hidden_layer.size(); i++) {
          hidden_layer[i] += hidden_weights[0][feature][i];
        }

    // Apply logistic sigmoid to hidden layer
    for (auto&& weight : hidden_layer)
      weight = 1 / (1 + exp(-weight));

    // Propagate to output_layer
    for (unsigned h = 0; h < hidden_layer.size(); h++)
      for (unsigned i = 0; i < output_layer.size(); i++)
        output_layer[i] += hidden_layer[h] * hidden_weights[1][h][i];
  }

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

  // Update direct connections
  for (auto&& feature : instance.features)
    for (unsigned i = 0; i < indices[feature].size(); i++)
      weights[feature][i] += learning_rate * output_error[indices[feature][i]] - weights[feature][i] * gaussian_sigma;

  // Update hidden layer
  if (!hidden_layer.empty()) {
    // Backpropagate output_error into hidden_error
    for (unsigned h = 0; h < hidden_layer.size(); h++) {
      hidden_error[h] = 0;
      for (unsigned i = 0; i < output_layer.size(); i++)
        hidden_error[h] += hidden_weights[1][h][i] * output_error[i];
      hidden_error[h] *= hidden_layer[h] * (1-hidden_layer[h]);
    }

    // Update hidden_weights[1]
    for (unsigned h = 0; h < hidden_layer.size(); h++)
      for (unsigned i = 0; i < output_layer.size(); i++)
        hidden_weights[1][h][i] += learning_rate * hidden_layer[h] * output_error[i] - hidden_weights[1][h][i] * gaussian_sigma;

    // Update hidden_weights[0]
    for (auto&& feature : instance.features)
      for (unsigned i = 0; i < hidden_layer.size(); i++)
        hidden_weights[0][feature][i] += learning_rate * hidden_error[i] - hidden_weights[0][feature][i] * gaussian_sigma;
  }
}

} // namespace nametag
} // namespace ufal
