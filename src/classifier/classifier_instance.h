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
#include "classifier_feature.h"
#include "classifier_outcome.h"

namespace ufal {
namespace nametag {

class classifier_instance {
 public:
  classifier_features features;
  classifier_outcome outcome;

  classifier_instance(const classifier_features& features, const classifier_outcome& outcome) : features(features), outcome(outcome) {}
};

} // namespace nametag
} // namespace ufal
