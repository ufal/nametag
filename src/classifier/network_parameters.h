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

namespace ufal {
namespace nametag {

struct network_parameters {
  int iterations;
  double missing_weight;
  double initial_learning_rate;
  double final_learning_rate;
  double gaussian_sigma;
  int hidden_layer; // Experimental use only.
};

} // namespace nametag
} // namespace ufal
