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
#include "microrestd/microrestd.h"
#include "ner/ner.h"
#include "tokenizer/tokenizer.h"

namespace ufal {
namespace nametag {

namespace microrestd = ufal::microrestd;

class nametag_service : public microrestd::rest_service {
 public:
  typedef ufal::nametag::tokenizer Tokenizer;
  typedef ufal::nametag::ner Ner;

  struct model_description {
    string rest_id, file, acknowledgements;

    model_description(string rest_id, string file, string acknowledgements)
        : rest_id(rest_id), file(file), acknowledgements(acknowledgements) {}
  };

  bool init(const vector<model_description>& model_descriptions);

  virtual bool handle(microrestd::rest_request& req) override;

 private:
  static unordered_map<string, bool (nametag_service::*)(microrestd::rest_request&)> handlers;

  // Models
  struct model_info {
    model_info(const string& rest_id, ner* ner, const string& acknowledgements)
        : rest_id(rest_id), ner(ner), acknowledgements(acknowledgements) {
      unique_ptr<Tokenizer> tokenizer(ner->new_tokenizer());
      can_tokenize = tokenizer != nullptr;
    }

    string rest_id;
    unique_ptr<Ner> ner;
    bool can_tokenize;
    string acknowledgements;
  };
  vector<model_info> models;
  unordered_map<string, const model_info*> rest_models_map;

  const model_info* load_rest_model(const string& rest_id, string& error);

  // REST service
  enum rest_output_mode_t {
    XML,
    VERTICAL,
  };
  struct rest_output_mode {
    rest_output_mode_t mode;

    rest_output_mode(rest_output_mode_t mode) : mode(mode) {}
    static bool parse(const string& mode, rest_output_mode& output);
  };

  class rest_response_generator : public microrestd::json_response_generator {
   public:
    rest_response_generator(const model_info* model, rest_output_mode output);

    virtual bool next(bool first) = 0;
    virtual bool generate() override;

   protected:
    bool first, last;
    rest_output_mode output;
  };

  bool handle_rest_models(microrestd::rest_request& req);
  bool handle_rest_recognize(microrestd::rest_request& req);
  bool handle_rest_tokenize(microrestd::rest_request& req);

  const string& get_rest_model_id(microrestd::rest_request& req);
  const char* get_data(microrestd::rest_request& req, string& error);
  tokenizer* get_tokenizer(microrestd::rest_request& req, const model_info* model, string& error);
  bool get_output_mode(microrestd::rest_request& req, rest_output_mode& mode, string& error);

  microrestd::json_builder json_models;
  static const char* json_mime;
  static const char* operation_not_supported;
};

} // namespace nametag
} // namespace ufal
