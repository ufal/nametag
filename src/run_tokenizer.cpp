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

#include <memory>

#include "ner/ner.h"
#include "utils/input.h"
#include "utils/output.h"
#include "utils/parse_options.h"
#include "utils/process_args.h"

using namespace ufal::nametag;

static void tokenize_vertical(FILE* in, FILE* out, tokenizer& tokenizer);
static void tokenize_xml(FILE* in, FILE* out, tokenizer& tokenizer);

int main(int argc, char* argv[]) {
  options_map options;
  if (!parse_options({{"output",{"vertical","xml"}}}, argc, argv, options) || argc < 2)
    runtime_errorf("Usage: %s [options] ner_model [file[:output_file]]...\n"
                   "Options: --output=vertical|xml", argv[0]);

  eprintf("Loading ner: ");
  unique_ptr<ner> recognizer(ner::load(argv[1]));
  if (!recognizer) runtime_errorf("Cannot load ner from file '%s'!", argv[1]);
  eprintf("done\n");

  unique_ptr<tokenizer> tokenizer(recognizer->new_tokenizer());
  if (!tokenizer) runtime_errorf("No tokenizer is defined for the supplied model!");

  if (options.count("output") && options["output"] == "vertical") process_args(2, argc, argv, tokenize_vertical, *tokenizer);
  else process_args(2, argc, argv, tokenize_xml, *tokenizer);

  return 0;
}

void tokenize_vertical(FILE* in, FILE* out, tokenizer& tokenizer) {
  string para;
  vector<string_piece> forms;
  while (getpara(in, para)) {
    // Tokenize
    tokenizer.set_text(para);
    while (tokenizer.next_sentence(&forms, nullptr)) {
      for (auto&& form : forms) {
        fwrite(form.str, 1, form.len, out);
        fputc('\n', out);
      }
      fputc('\n', out);
    }
  }
}

static void tokenize_xml(FILE* in, FILE* out, tokenizer& tokenizer) {
  string para;
  vector<string_piece> forms;
  while (getpara(in, para)) {
    // Tokenize
    tokenizer.set_text(para);
    const char* unprinted = para.c_str();
    while (tokenizer.next_sentence(&forms, nullptr))
      for (unsigned i = 0; i < forms.size(); i++) {
        if (unprinted < forms[i].str) print_xml_content(out, unprinted, forms[i].str - unprinted);
        if (!i) fputs("<sentence>", out);
        fputs("<token>", out);
        print_xml_content(out, forms[i].str, forms[i].len);
        fputs("</token>", out);
        if (i + 1 == forms.size()) fputs("</sentence>", out);
        unprinted = forms[i].str + forms[i].len;
      }

    if (unprinted < para.c_str() + para.size()) print_xml_content(out, unprinted, para.c_str() + para.size() - unprinted);
  }
}
