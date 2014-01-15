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

#include "url_detector.h"

namespace ufal {
namespace nametag {

%%{
  machine url_detector;
  write data;
}%%

url_detector::url_type url_detector::detect(string_piece str) {
  int cs;
  const char* p = str.str;
  const char* pe = str.str + str.len;

  url_type result = NO_URL;
  %%{
    action mark_url { result = URL; }
    action mark_email { result = EMAIL; }

    variable eof pe;

    uchar = alnum | "$" | "-" | "_" | "." | "+" | "!" | "*" | "'" | "(" | ")" | "," | "%";
    xchar = uchar | ";" | "/" | "?" | ":" | "@" | "&" | "=";

    urlpath = '/' | '/' xchar* (alnum | "'" | ')');

    port = ':' digit+;

    ip = [0-9] | [1-9] [0-9] | '1' [0-9] [0-9] | '2' [0-4] [0-9] | '25' [0-5];
    hostnumber = ip '.' ip '.' ip '.' ip;
    topdomain = lower{2,} | upper{2,};
    subdomain = alnum | alnum (alnum | "-")* alnum;
    hostname = (subdomain '.')+ topdomain;
    host = hostname | hostnumber;

    username = uchar+;
    password = uchar+;
    user = username (':' password)? '@';

    protocol = alpha{3,} '://';

    url = protocol? user? host port? urlpath?;
    email = username '@' hostname;

    main := (url %mark_url) | (email %mark_email);

    write init;
    write exec;
  }%%
  return result;
}

} // namespace nametag
} // namespace ufal
