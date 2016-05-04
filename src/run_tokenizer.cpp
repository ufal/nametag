// This file is part of NameTag <http://github.com/ufal/nametag/>.
//
// Copyright 2016 Institute of Formal and Applied Linguistics, Faculty of
// Mathematics and Physics, Charles University in Prague, Czech Republic.
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#include "ner/ner.h"
#include "utils/getpara.h"
#include "utils/iostreams.h"
#include "utils/options.h"
#include "utils/process_args.h"
#include "utils/xml_encoded.h"
#include "version/version.h"

using namespace ufal::nametag;

static void tokenize_vertical(istream& in, ostream& os, tokenizer& tokenizer);
static void tokenize_xml(istream& is, ostream& os, tokenizer& tokenizer);

int main(int argc, char* argv[]) {
  iostreams_init();

  options::map options;
  if (!options::parse({{"output",options::value{"vertical","xml"}},
                       {"version", options::value::none},
                       {"help", options::value::none}}, argc, argv, options) ||
      options.count("help") ||
      (argc < 2 && !options.count("version")))
    runtime_failure("Usage: " << argv[0] << " [options] ner_model [file[:output_file]]...\n"
                    "Options: --output=vertical|xml\n"
                    "         --version\n"
                    "         --help");
  if (options.count("version"))
    return cout << version::version_and_copyright() << endl, 0;

  cerr << "Loading ner: ";
  unique_ptr<ner> recognizer(ner::load(argv[1]));
  if (!recognizer) runtime_failure("Cannot load ner from file '" << argv[1] << "'!");
  cerr << "done" << endl;

  unique_ptr<tokenizer> tokenizer(recognizer->new_tokenizer());
  if (!tokenizer) runtime_failure("No tokenizer is defined for the supplied model!");

  if (options.count("output") && options["output"] == "vertical") process_args(2, argc, argv, tokenize_vertical, *tokenizer);
  else process_args(2, argc, argv, tokenize_xml, *tokenizer);

  return 0;
}

void tokenize_vertical(istream& is, ostream& os, tokenizer& tokenizer) {
  string para;
  vector<string_piece> forms;
  while (getpara(is, para)) {
    // Tokenize
    tokenizer.set_text(para);
    while (tokenizer.next_sentence(&forms, nullptr)) {
      for (auto&& form : forms) {
        os << form << '\n';
      }
      os << '\n' << flush;
    }
  }
}

static void tokenize_xml(istream& is, ostream& os, tokenizer& tokenizer) {
  string para;
  vector<string_piece> forms;
  while (getpara(is, para)) {
    // Tokenize
    tokenizer.set_text(para);
    const char* unprinted = para.c_str();
    while (tokenizer.next_sentence(&forms, nullptr))
      for (unsigned i = 0; i < forms.size(); i++) {
        if (unprinted < forms[i].str) os << xml_encoded(string_piece(unprinted, forms[i].str - unprinted));
        if (!i) os << "<sentence>";
        os << "<token>" << xml_encoded(forms[i]) << "</token>";
        if (i + 1 == forms.size()) os << "</sentence>";
        unprinted = forms[i].str + forms[i].len;
      }

    if (unprinted < para.c_str() + para.size()) os << xml_encoded(string_piece(unprinted, para.c_str() + para.size() - unprinted));
    os << flush;
  }
}
