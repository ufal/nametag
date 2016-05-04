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

#include <sstream>

#include "morphodita/version/version.h"
#include "unilib/version.h"
#include "version.h"

namespace ufal {
namespace nametag {

version version::current() {
  return {1, 0, 1, "devel"};
}

// Returns multi-line formated version and copyright string.
string version::version_and_copyright(const string& other_libraries) {
  ostringstream info;

  auto nametag = version::current();
  auto unilib = unilib::version::current();
  auto morphodita = morphodita::version::current();

  info << "NameTag version " << nametag.major << '.' << nametag.minor << '.' << nametag.patch
       << (nametag.prerelease.empty() ? "" : "-") << nametag.prerelease
       << " (using UniLib " << unilib.major << '.' << unilib.minor << '.' << unilib.patch
       << (unilib.prerelease.empty() ? "" : "-") << unilib.prerelease
       << ", MorphoDiTa " << morphodita.major << '.' << morphodita.minor << '.' << unilib.patch
       << (morphodita.prerelease.empty() ? "" : "-") << morphodita.prerelease
       << (other_libraries.empty() ? "" : "\nand ") << other_libraries << ")\n"
          "Copyright 2016 by Institute of Formal and Applied Linguistics, Faculty of\n"
          "Mathematics and Physics, Charles University in Prague, Czech Republic.";

  return info.str();
}

} // namespace nametag
} // namespace ufal
