// This file is part of NameTag <http://github.com/ufal/nametag/>.
//
// Copyright 2016 Institute of Formal and Applied Linguistics, Faculty of
// Mathematics and Physics, Charles University in Prague, Czech Republic.
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#include <algorithm>

#include "nametag_service.h"
#include "unilib/unicode.h"
#include "unilib/uninorms.h"
#include "unilib/utf8.h"

namespace ufal {
namespace nametag {

// Init the NameTag service -- load the models
bool nametag_service::init(const vector<model_description>& model_descriptions) {
  if (model_descriptions.empty()) return false;

  // Load models
  models.clear();
  rest_models_map.clear();
  for (auto& model_description : model_descriptions) {
    Ner* ner = Ner::load(model_description.file.c_str());
    if (!ner) return false;

    // Store the model
    models.emplace_back(model_description.rest_id, ner, model_description.acknowledgements);
  }

  // Fill rest_models_map with model name and aliases
  for (auto& model : models) {
    // Fail if this model id is aready in use.
    if (!rest_models_map.emplace(model.rest_id, &model).second) return false;

    // Create (but not overwrite) id without version.
    for (unsigned i = 0; i+1+6 < model.rest_id.size(); i++)
      if (model.rest_id[i] == '-') {
        bool is_version = true;
        for (unsigned j = i+1; j < i+1+6; j++)
          is_version = is_version && model.rest_id[j] >= '0' && model.rest_id[j] <= '9';
        if (is_version)
          rest_models_map.emplace(model.rest_id.substr(0, i) + model.rest_id.substr(i+1+6), &model);
      }

    // Create (but not overwrite) hyphen-separated prefixes.
    for (unsigned i = 0; i < model.rest_id.size(); i++)
      if (model.rest_id[i] == '-')
        rest_models_map.emplace(model.rest_id.substr(0, i), &model);
  }
  // Default model
  rest_models_map.emplace(string(), &models.front());

  // Init REST service
  json_models.clear().object().indent().key("models").indent().object();
  for (auto& model : models) {
    json_models.indent().key(model.rest_id).indent().array();
    json_models.value("recognize");
    if (model.can_tokenize) json_models.value("tokenize");
    json_models.close();
  }
  json_models.indent().close().indent().key("default_model").indent().value(model_descriptions.front().rest_id).finish(true);

  return true;
}

// Handlers with their URLs
unordered_map<string, bool (nametag_service::*)(microrestd::rest_request&)> nametag_service::handlers = {
  // REST service
  {"/models", &nametag_service::handle_rest_models},
  {"/recognize", &nametag_service::handle_rest_recognize},
  {"/tokenize", &nametag_service::handle_rest_tokenize},
};

// Handle a request using the specified URL/handler map
bool nametag_service::handle(microrestd::rest_request& req) {
  auto handler_it = handlers.find(req.url);
  return handler_it == handlers.end() ? req.respond_not_found() : (this->*handler_it->second)(req);
}

// Load selected model
const nametag_service::model_info* nametag_service::load_rest_model(const string& rest_id, string& error) {
  auto model_it = rest_models_map.find(rest_id);
  if (model_it == rest_models_map.end())
    return error.assign("Requested model '").append(rest_id).append("' does not exist.\n"), nullptr;

  return model_it->second;
}

// REST service
inline microrestd::string_piece sp(string_piece str) { return microrestd::string_piece(str.str, str.len); }
inline microrestd::string_piece sp(const char* str, size_t len) { return microrestd::string_piece(str, len); }

const char* nametag_service::json_mime = "application/json";
const char* nametag_service::operation_not_supported = "Required operation is not supported by the chosen model.\n";
const char* nametag_service::infclen_header = "X-Billing-Input-NFC-Len";

nametag_service::rest_response_generator::rest_response_generator(const model_info* model, rest_output_mode output)
  : first(true), last(false), output(output) {
  json.object();
  json.indent().key("model").indent().value(model->rest_id);
  json.indent().key("acknowledgements").indent().array();
  json.indent().value("http://ufal.mff.cuni.cz/nametag/1#nametag_acknowledgements");
  if (!model->acknowledgements.empty()) json.indent().value(model->acknowledgements);
  json.indent().close().indent().key("result").indent();
}

bool nametag_service::rest_response_generator::generate() {
  if (last) return false;

  if (!next(first)) {
    json.finish(true);
    last = true;
  }
  first = false;
  return true;
}

bool nametag_service::rest_output_mode::parse(const string& str, rest_output_mode& output) {
  if (str.compare("xml") == 0) return output.mode = XML, true;
  if (str.compare("vertical") == 0) return output.mode = VERTICAL, true;
  if (str.compare("conll") == 0) return output.mode = CONLL, true;
  return false;
}

// REST service handlers

bool nametag_service::handle_rest_models(microrestd::rest_request& req) {
  return req.respond(json_mime, json_models);
}

bool nametag_service::handle_rest_recognize(microrestd::rest_request& req) {
  string error;
  auto rest_id = get_rest_model_id(req);
  auto model = load_rest_model(rest_id, error);
  if (!model) return req.respond_error(error);

  string data; int infclen; if (!get_data(req, data, infclen, error)) return req.respond_error(error);
  unique_ptr<Tokenizer> tokenizer(get_tokenizer(req, model, error)); if (!tokenizer) return req.respond_error(error);
  rest_output_mode output(XML); if (!get_output_mode(req, output, error)) return req.respond_error(error);

  class generator : public rest_response_generator {
   public:
    generator(const model_info* model, string&& data, const Ner* ner, Tokenizer* tokenizer, rest_output_mode output)
        : rest_response_generator(model, output), data(data), ner(ner), tokenizer(tokenizer), unprinted(this->data.c_str()) {
      tokenizer->set_text(this->data);
    }

    void sort_entities(vector<named_entity>& entities) {
      struct named_entity_comparator {
        static bool lt(const named_entity& a, const named_entity& b) {
          return a.start < b.start || (a.start == b.start && a.length > b.length);
        }
      };

      // Many models return entities sorted -- it is worthwhile to check that.
      if (!is_sorted(entities.begin(), entities.end(), named_entity_comparator::lt))
        sort(entities.begin(), entities.end(), named_entity_comparator::lt);
    }

    bool next(bool /*first*/) {
      if (!tokenizer->next_sentence(&forms, nullptr)) {
        if (output.mode == XML && *unprinted) json.value_xml_escape(unprinted, true);
        return false;
      }

      ner->recognize(forms, entities);
      sort_entities(entities);

      if (output.mode == CONLL) {
        vector<named_entity> stack;
        for (size_t i = 0, e = 0; i < forms.size(); i++) {
          for (; e < entities.size() && entities[e].start == i; e++)
            stack.push_back(entities[e]);

          json.value(sp(forms[i]), true);
          json.value("\t", true);
          if (stack.size()) {
            for (size_t j = 0; j < stack.size(); j++) {
              if (j) json.value("|", true);
              json.value(stack[j].start == i ? "B-" : "I-", true);
              json.value(stack[j].type, true);
            }
          } else {
            json.value("O", true);
          }

          for (size_t j = stack.size(); j--; )
            if (stack[j].start + stack[j].length == i + 1)
              stack.erase(stack.begin() + j);
          json.value("\n", true);
        }
        json.value("\n", true);
      } else if (output.mode == VERTICAL) {
        for (auto&& entity : entities) {
          for (size_t i = entity.start; i < entity.start + entity.length; i++) {
            sprintf(token_number, "%zu", total_tokens + i + 1);
            if (i > entity.start) json.value(",", true);
            json.value(token_number, true);
          }
          json.value("\t", true).value(entity.type, true).value("\t", true);
          for (size_t i = entity.start; i < entity.start + entity.length; i++) {
            if (i > entity.start) json.value(" ", true);
            json.value(sp(forms[i]), true);
          }
          json.value("\n", true);
        }
      } else {
        for (unsigned i = 0, e = 0; i < forms.size(); i++) {
          if (unprinted < forms[i].str) json.value_xml_escape(sp(unprinted, forms[i].str - unprinted), true);
          if (i == 0) json.value("<sentence>", true);

          // Open entities starting at current token
          for (; e < entities.size() && entities[e].start == i; e++) {
            json.value("<ne type=\"", true).value_xml_escape(entities[e].type, true).value("\">", true);
            entity_ends.push_back(entities[e].start + entities[e].length - 1);
          }

          // The token itself
          json.value("<token>", true).value_xml_escape(sp(forms[i]), true).value("</token>", true);

          // Close entities ending after current token
          while (!entity_ends.empty() && entity_ends.back() == i) {
            json.value("</ne>", true);
            entity_ends.pop_back();
          }
          if (i + 1 == forms.size()) json.value("</sentence>", true);
          unprinted = forms[i].str + forms[i].len;
        }
      }

      total_tokens += forms.size() + 1;
      return true;
    }

   private:
    string data;
    const Ner* ner;
    unique_ptr<Tokenizer> tokenizer;
    const char* unprinted;
    vector<string_piece> forms;
    vector<named_entity> entities;
    vector<size_t> entity_ends;
    size_t total_tokens = 0;
    char token_number[sizeof(size_t) * 3/*ceil(log_10(256))*/];
  };
  return req.respond(json_mime, new generator(model, move(data), model->ner.get(), tokenizer.release(), output), {{infclen_header, to_string(infclen).c_str()}});
}

bool nametag_service::handle_rest_tokenize(microrestd::rest_request& req) {
  string error;
  auto rest_id = get_rest_model_id(req);
  auto model = load_rest_model(rest_id, error);
  if (!model) return req.respond_error(error);
  if (!model->can_tokenize) return req.respond_error(operation_not_supported);

  string data; int infclen; if (!get_data(req, data, infclen, error)) return req.respond_error(error);
  rest_output_mode output(XML); if (!get_output_mode(req, output, error)) return req.respond_error(error);
  if (output.mode == CONLL) return req.respond_error("Unsupported output mode 'conll'");

  class generator : public rest_response_generator {
   public:
    generator(const model_info* model, string&& data, rest_output_mode output, Tokenizer* tokenizer)
        : rest_response_generator(model, output), data(data), tokenizer(tokenizer), unprinted(this->data.c_str()) {
      tokenizer->set_text(this->data);
    }

    bool next(bool /*first*/) {
      if (!tokenizer->next_sentence(&forms, nullptr)) {
        if (output.mode == XML && *unprinted) json.value_xml_escape(unprinted, true);
        return false;
      }

      for (unsigned i = 0; i < forms.size(); i++) {
        switch (output.mode) {
          case VERTICAL:
            json.value(sp(forms[i]), true).value("\n", true);
            break;
          case XML:
            if (unprinted < forms[i].str) json.value_xml_escape(sp(unprinted, forms[i].str - unprinted), true);
            if (!i) json.value("<sentence>", true);
            json.value("<token>", true).value_xml_escape(sp(forms[i]), true).value("</token>", true);
            break;
          case CONLL: // Keep the compiler happy
            break;
        }
        unprinted = forms[i].str + forms[i].len;
      }
      if (output.mode == VERTICAL) json.value("\n", true);
      if (output.mode == XML) json.value("</sentence>", true);

      return true;
    }

   private:
    string data;
    unique_ptr<Tokenizer> tokenizer;
    const char* unprinted;
    vector<string_piece> forms;
  };
  return req.respond(json_mime, new generator(model, move(data), output, model->ner->new_tokenizer()), {{infclen_header, to_string(infclen).c_str()}});
}

// REST service helpers

const string& nametag_service::get_rest_model_id(microrestd::rest_request& req) {
  static string empty;

  auto model_it = req.params.find("model");
  return model_it == req.params.end() ? empty : model_it->second;
}

bool nametag_service::get_data(microrestd::rest_request& req, string& data, int& infclen, string& error) {
  auto data_it = req.params.find("data");
  if (data_it == req.params.end()) return error.assign("Required argument 'data' is missing.\n"), false;

  u32string codepoints;
  unilib::utf8::decode(data_it->second, codepoints);
  unilib::uninorms::nfc(codepoints);
  infclen = 0;
  for (auto&& codepoint : codepoints)
    infclen += !(unilib::unicode::category(codepoint) & (unilib::unicode::C | unilib::unicode::Z));
  unilib::utf8::encode(codepoints, data);
  return true;
}

tokenizer* nametag_service::get_tokenizer(microrestd::rest_request& req, const model_info* model, string& error) {
  auto input_it = req.params.find("input");
  if (input_it == req.params.end() || input_it->second.compare("untokenized") == 0) {
    if (!model->can_tokenize) return error.assign("No tokenizer is defined for the requested model.\n"), nullptr;
    return model->ner->new_tokenizer();
  }
  if (input_it->second.compare("vertical") == 0) return tokenizer::new_vertical_tokenizer();
  return error.assign("Value '").append(input_it->second).append("' of parameter input is not either 'untokenized' or 'vertical'.\n"), nullptr;
}

bool nametag_service::get_output_mode(microrestd::rest_request& req, rest_output_mode& output, string& error) {
  auto output_it = req.params.find("output");
  if (output_it != req.params.end() && !rest_output_mode::parse(output_it->second, output))
    return error.assign("Unknown output mode '").append(output_it->second).append("'.\n"), false;
  return true;
}

} // namespace nametag
} // namespace ufal
