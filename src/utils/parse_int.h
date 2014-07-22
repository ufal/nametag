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

#include <cerrno>

#include "common.h"

namespace ufal {
namespace nametag {

// Try to parse an int from given string. If the int cannot be parsed or does
// not fit into int, an error is displayed and program exits. If that happens,
// the value_name argument is used in the error message.
inline int parse_int(const char* str, const char* value_name) {
  char* end;

  errno = 0;
  long result = strtol(str, &end, 10);
  if (*end || errno == ERANGE || result != int(result))
    runtime_errorf("Cannot parse %s int value: '%s'!", value_name, str);

  return int(result);
}

} // namespace nametag
} // namespace ufal
