// This file is part of NameTag <http://github.com/ufal/nametag/>.
//
// Copyright 2016 Institute of Formal and Applied Linguistics, Faculty of
// Mathematics and Physics, Charles University in Prague, Czech Republic.
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#pragma once

#include <unordered_map>

#include "common.h"
#include "entity_type.h"

namespace ufal {
namespace nametag {

typedef unsigned bilou_type;
enum :bilou_type { bilou_type_B, bilou_type_I, bilou_type_L, bilou_type_O, bilou_type_U, bilou_type_total, bilou_type_unknown = ~0U };

} // namespace nametag
} // namespace ufal
