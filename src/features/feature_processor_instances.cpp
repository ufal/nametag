// This file is part of NameTag <http://github.com/ufal/nametag/>.
//
// Copyright 2016 Institute of Formal and Applied Linguistics, Faculty of
// Mathematics and Physics, Charles University in Prague, Czech Republic.
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#include <algorithm>
#include <fstream>
#include <unordered_map>

#include "feature_processor.h"
#include "unilib/unicode.h"
#include "unilib/utf8.h"
#include "utils/parse_int.h"
#include "utils/split.h"
#include "utils/url_detector.h"

namespace ufal {
namespace nametag {

// Helper functions defined as macros so that they can access arguments without passing them
#define apply_in_window(I, Feature) apply_in_range(I, Feature, -window, window)

#define apply_in_range(I, Feature, Left, Right) {                                                   \
  ner_feature _feature = (Feature);                                                                 \
  if (_feature != ner_feature_unknown)                                                              \
    for (int _w = int(I) + (Left) < 0 ? 0 : int(I) + (Left),                                        \
           _end = int(I) + (Right) + 1 < int(sentence.size) ? int(I) + (Right) + 1 : sentence.size; \
         _w < _end; _w++)                                                                           \
      sentence.features[_w].emplace_back(_feature + _w - int(I));                                   \
}

#define apply_outer_words_in_window(Feature) {                   \
  ner_feature _outer_feature = (Feature);                        \
  if (_outer_feature != ner_feature_unknown)                     \
    for (int _i = 1; _i <= window; _i++) {                       \
        apply_in_window(-_i, _outer_feature);                    \
        apply_in_window(sentence.size - 1 + _i, _outer_feature); \
    }                                                            \
}

#define lookup_empty() /* lookup(string()) always returns */(window)


//////////////////////////////////////////////////////////////
// Feature processor instances (ordered lexicographically) //
//////////////////////////////////////////////////////////////
namespace feature_processors {

// BrownClusters
class brown_clusters : public feature_processor {
 public:
  virtual bool parse(int window, const vector<string>& args, entity_map& entities,
                     ner_feature* total_features, const nlp_pipeline& pipeline) override {
    if (!feature_processor::parse(window, args, entities, total_features, pipeline)) return false;
    if (args.size() < 1) return cerr << "BrownCluster requires a cluster file as the first argument!" << endl, false;

    ifstream in(args[0]);
    if (!in.is_open()) return cerr << "Cannot open Brown clusters file '" << args[0] << "'!" << endl, false;

    vector<size_t> substrings;
    substrings.emplace_back(string::npos);
    for (unsigned i = 1; i < args.size(); i++) {
      int len = parse_int(args[i].c_str(), "BrownCluster_prefix_length");
      if (len <= 0)
        return cerr << "Wrong prefix length '" << len << "' in BrownCluster specification!" << endl, false;
      else
        substrings.emplace_back(len);
    }

    clusters.clear();
    unordered_map<string, unsigned> cluster_map;
    unordered_map<string, ner_feature> prefixes_map;
    string line;
    vector<string> tokens;
    while (getline(in, line)) {
      split(line, '\t', tokens);
      if (tokens.size() != 2) return cerr << "Wrong line '" << line << "' in Brown cluster file '" << args[0] << "'!" << endl, false;

      string cluster = tokens[0], form = tokens[1];
      auto it = cluster_map.find(cluster);
      if (it == cluster_map.end()) {
        unsigned id = clusters.size();
        clusters.emplace_back();
        for (auto&& substring : substrings)
          if (substring == string::npos || substring < cluster.size())
            clusters.back().emplace_back(prefixes_map.emplace(cluster.substr(0, substring), *total_features + (2*window + 1) * prefixes_map.size() + window).first->second);
        it = cluster_map.emplace(cluster, id).first;
      }
      if (!map.emplace(form, it->second).second) return cerr << "Form '" << form << "' is present twice in Brown cluster file '" << args[0] << "'!" << endl, false;
    }

    *total_features += (2*window + 1) * prefixes_map.size();
    return true;
  }

  virtual void load(binary_decoder& data, const nlp_pipeline& pipeline) override {
    feature_processor::load(data, pipeline);

    clusters.resize(data.next_4B());
    for (auto&& cluster : clusters) {
      cluster.resize(data.next_4B());
      for (auto&& feature : cluster)
        feature = data.next_4B();
    }
  }

  virtual void save(binary_encoder& enc) override {
    feature_processor::save(enc);

    enc.add_4B(clusters.size());
    for (auto&& cluster : clusters) {
      enc.add_4B(cluster.size());
      for (auto&& feature : cluster)
        enc.add_4B(feature);
    }
  }

  virtual void process_sentence(ner_sentence& sentence, ner_feature* /*total_features*/, string& /*buffer*/) const override {
    for (unsigned i = 0; i < sentence.size; i++) {
      auto it = map.find(sentence.words[i].raw_lemma);
      if (it != map.end()) {
        auto& cluster = clusters[it->second];
        for (auto&& feature : cluster)
          apply_in_window(i, feature);
      }
    }
  }

 private:
  vector<vector<ner_feature>> clusters;
};


// CzechAddContainers
class czech_add_containers : public feature_processor {
 public:
  virtual bool parse(int window, const vector<string>& args, entity_map& entities, ner_feature* total_features, const nlp_pipeline& pipeline) override {
    if (window) return cerr << "CzechAddContainers cannot have non-zero window!" << endl, false;

    return feature_processor::parse(window, args, entities, total_features, pipeline);
  }

  virtual void process_entities(ner_sentence& /*sentence*/, vector<named_entity>& entities, vector<named_entity>& buffer) const override {
    buffer.clear();

    for (unsigned i = 0; i < entities.size(); i++) {
      // P if ps+ pf+
      if (entities[i].type.compare("pf") == 0 && (!i || entities[i-1].start + entities[i-1].length < entities[i].start || entities[i-1].type.compare("pf") != 0)) {
        unsigned j = i + 1;
        while (j < entities.size() && entities[j].start == entities[j-1].start + entities[j-1].length && entities[j].type.compare("pf") == 0) j++;
        if (j < entities.size() && entities[j].start == entities[j-1].start + entities[j-1].length && entities[j].type.compare("ps") == 0) {
          j++;
          while (j < entities.size() && entities[j].start == entities[j-1].start + entities[j-1].length && entities[j].type.compare("ps") == 0) j++;
          buffer.emplace_back(entities[i].start, entities[j - 1].start + entities[j - 1].length - entities[i].start, "P");
        }
      }

      // T if td tm ty | td tm
      if (entities[i].type.compare("td") == 0 && i+1 < entities.size() && entities[i+1].start == entities[i].start + entities[i].length && entities[i+1].type.compare("tm") == 0) {
        unsigned j = i + 2;
        if (j < entities.size() && entities[j].start == entities[j-1].start + entities[j-1].length && entities[j].type.compare("ty") == 0) j++;
        buffer.emplace_back(entities[i].start, entities[j - 1].start + entities[j - 1].length - entities[i].start, "T");
      }
      // T if !td tm ty
      if (entities[i].type.compare("tm") == 0 && (!i || entities[i-1].start + entities[i-1].length < entities[i].start || entities[i-1].type.compare("td") != 0))
        if (i+1 < entities.size() && entities[i+1].start == entities[i].start + entities[i].length && entities[i+1].type.compare("ty") == 0)
          buffer.emplace_back(entities[i].start, entities[i + 1].start + entities[i + 1].length - entities[i].start, "T");

      buffer.push_back(entities[i]);
    }

    if (buffer.size() > entities.size()) entities = buffer;
  }

  // CzechAddContainers used to be entity_processor which had empty load and save methods.
  virtual void load(binary_decoder& /*data*/, const nlp_pipeline& /*pipeline*/) override {}
  virtual void save(binary_encoder& /*enc*/) override {}
};


// CzechLemmaTerm
class czech_lemma_term : public feature_processor {
 public:
  virtual void process_sentence(ner_sentence& sentence, ner_feature* total_features, string& buffer) const override {
    for (unsigned i = 0; i < sentence.size; i++) {
      for (unsigned pos = 0; pos + 2 < sentence.words[i].lemma_comments.size(); pos++)
        if (sentence.words[i].lemma_comments[pos] == '_' && sentence.words[i].lemma_comments[pos+1] == ';') {
          buffer.assign(1, sentence.words[i].lemma_comments[pos+2]);
          apply_in_window(i, lookup(buffer, total_features));
        }
    }
  }
};


// Form
class form : public feature_processor {
 public:
  virtual void process_sentence(ner_sentence& sentence, ner_feature* total_features, string& /*buffer*/) const override {
    for (unsigned i = 0; i < sentence.size; i++)
      apply_in_window(i, lookup(sentence.words[i].form, total_features));

    apply_outer_words_in_window(lookup_empty());
  }
};


// FormCapitalization
class form_capitalization : public feature_processor {
 public:
  virtual void process_sentence(ner_sentence& sentence, ner_feature* total_features, string& buffer) const override {
    using namespace unilib;

    ner_feature fst_cap = lookup(buffer.assign("f"), total_features);
    ner_feature all_cap = lookup(buffer.assign("a"), total_features);
    ner_feature mixed_cap = lookup(buffer.assign("m"), total_features);

    for (unsigned i = 0; i < sentence.size; i++) {
      bool was_upper = false, was_lower = false;

      auto* form = sentence.words[i].form.c_str();
      char32_t chr;
      for (bool first = true; (chr = utf8::decode(form)); first = false) {
        auto category = unicode::category(chr);
        was_upper = was_upper || category & unicode::Lut;
        was_lower = was_lower || category & unicode::Ll;

        if (first && was_upper) apply_in_window(i, fst_cap);
      }
      if (was_upper && !was_lower) apply_in_window(i, all_cap);
      if (was_upper && was_lower) apply_in_window(i, mixed_cap);
    }
  }
};


// FormCaseNormalized
class form_case_normalized : public feature_processor {
 public:
  virtual void process_sentence(ner_sentence& sentence, ner_feature* total_features, string& buffer) const override {
    using namespace unilib;

    for (unsigned i = 0; i < sentence.size; i++) {
      buffer.clear();
      for (auto&& chr : utf8::decoder(sentence.words[i].form))
        utf8::append(buffer, buffer.empty() ? chr : unicode::lowercase(chr));
      apply_in_window(i, lookup(buffer, total_features));
    }

    apply_outer_words_in_window(lookup_empty());
  }
};


// Gazetteers
class gazetteers : public feature_processor {
 public:
  enum { G = 0, U = 1, B = 2, L = 3, I = 4 };

  virtual bool parse(int window, const vector<string>& args, entity_map& entities,
                     ner_feature* total_features, const nlp_pipeline& pipeline) override {
    cerr << "The 'Gazetteers' feature template is deprecated, use 'GazetteersEnhanced' !" << endl;

    if (!feature_processor::parse(window, args, entities, total_features, pipeline)) return false;

    gazetteers_info.clear();
    for (auto&& arg : args) {
      ifstream in(arg.c_str());
      if (!in.is_open()) return cerr << "Cannot open gazetteers file '" << arg << "'!" << endl, false;

      unsigned longest = 0;
      string gazetteer;
      string line;
      vector<string> tokens;
      while (getline(in, line)) {
        split(line, ' ', tokens);
        for (unsigned i = 0; i < tokens.size(); i++)
          if (!tokens[i][0])
            tokens.erase(tokens.begin() + i--);
        if (tokens.size() > longest) longest = tokens.size();

        gazetteer.clear();
        for (unsigned i = 0; i < tokens.size(); i++) {
          if (i) gazetteer += ' ';
          gazetteer += tokens[i];
          auto it = map.emplace(gazetteer, gazetteers_info.size()).first;
          if (it->second == gazetteers_info.size()) gazetteers_info.emplace_back();
          auto& info = gazetteers_info[it->second];
          if (i + 1 < tokens.size())
            info.prefix_of_longer |= true;
          else
            if (find(info.features.begin(), info.features.end(), *total_features + window) == info.features.end())
              info.features.emplace_back(*total_features + window);
        }
      }
      *total_features += (2*window + 1) * (longest == 0 ? 0 : longest == 1 ? U+1 : longest == 2 ? L+1 : I+1);
    }

    return true;
  }

  virtual void load(binary_decoder& data, const nlp_pipeline& pipeline) override {
    feature_processor::load(data, pipeline);

    gazetteers_info.resize(data.next_4B());
    for (auto&& gazetteer : gazetteers_info) {
      gazetteer.prefix_of_longer = data.next_1B();
      gazetteer.features.resize(data.next_1B());
      for (auto&& feature : gazetteer.features)
        feature = data.next_4B();
    }
  }

  virtual void save(binary_encoder& enc) override {
    feature_processor::save(enc);

    enc.add_4B(gazetteers_info.size());
    for (auto&& gazetteer : gazetteers_info) {
      enc.add_1B(gazetteer.prefix_of_longer);
      enc.add_1B(gazetteer.features.size());
      for (auto&& feature : gazetteer.features)
        enc.add_4B(feature);
    }
  }

  virtual void process_sentence(ner_sentence& sentence, ner_feature* /*total_features*/, string& buffer) const override {
    for (unsigned i = 0; i < sentence.size; i++) {
      auto it = map.find(sentence.words[i].raw_lemma);
      if (it == map.end()) continue;

      // Apply regular gazetteer feature G + unigram gazetteer feature U
      for (auto&& feature : gazetteers_info[it->second].features) {
        apply_in_window(i, feature + G * (2*window + 1));
        apply_in_window(i, feature + U * (2*window + 1));
      }

      for (unsigned j = i + 1; gazetteers_info[it->second].prefix_of_longer && j < sentence.size; j++) {
        if (j == i + 1) buffer.assign(sentence.words[i].raw_lemma);
        buffer += ' ';
        buffer += sentence.words[j].raw_lemma;
        it = map.find(buffer);
        if (it == map.end()) break;

        // Apply regular gazetteer feature G + position specific gazetteers B, I, L
        for (auto&& feature : gazetteers_info[it->second].features)
          for (unsigned g = i; g <= j; g++) {
            apply_in_window(g, feature + G * (2*window + 1));
            apply_in_window(g, feature + (g == i ? B : g == j ? L : I) * (2*window + 1));
          }
      }
    }
  }

 private:
  struct gazetteer_info {
    vector<ner_feature> features;
    bool prefix_of_longer;
  };
  vector<gazetteer_info> gazetteers_info;
};


// GazetteersEnhanced
class gazetteers_enhanced : public feature_processor {
 public:
  enum { G = 0, U = 1, B = 2, L = 3, I = 4, TOTAL = 5 };

  virtual bool parse(int window, const vector<string>& args, entity_map& entities,
                     ner_feature* total_features, const nlp_pipeline& pipeline) override {
    if (!feature_processor::parse(window, args, entities, total_features, pipeline)) return false;

    gazetteer_metas.clear();
    gazetteer_lists.clear();

    if (args.size() < 4) return cerr << "Not enough parameters to GazetteersEnhanced!" << endl, false;
    if (args.size() & 1) return cerr << "Odd number of parameters to GazetteersEnhanced!" << endl, false;

    if (args[0] == "form") match = MATCH_FORM;
    else if (args[0] == "rawlemma") match = MATCH_RAWLEMMA;
    else if (args[0] == "rawlemmas") match = MATCH_RAWLEMMAS;
    else return cerr << "First parameter of GazetteersEnhanced not one of form/rawlemma/rawlemmas!" << endl, false;

    if (args[1] == "embed_in_model") embed = EMBED_IN_MODEL;
    else if (args[1] == "out_of_model") embed = OUT_OF_MODEL;
    else return cerr << "Second parameter of GazetteersEnhanced not one of [embed_in|out_of]_model!" << endl, false;

    for (unsigned i = 2; i < args.size(); i += 2) {
      gazetteer_metas.emplace_back();
      gazetteer_metas.back().basename = args[i];
      gazetteer_metas.back().feature = *total_features + window; *total_features += TOTAL * (2 * window + 1);
      gazetteer_metas.back().entity = args[i + 1] == "NONE" ? -1 : entities.parse(args[i + 1].c_str(), true);
    }

    entity_list.clear();
    for (entity_type i = 0; i < entities.size(); i++)
      entity_list.push_back(entities.name(i));

    if (!load_gazetteer_lists(pipeline, embed == EMBED_IN_MODEL)) return false;

    return true;
  }

  virtual void load(binary_decoder& data, const nlp_pipeline& pipeline) override {
    feature_processor::load(data, pipeline);

    match = data.next_4B();
    embed = OUT_OF_MODEL;

    gazetteer_metas.resize(data.next_4B());
    for (auto&& gazetteer_meta : gazetteer_metas) {
      data.next_str(gazetteer_meta.basename);
      gazetteer_meta.feature = data.next_4B();
      gazetteer_meta.entity = data.next_4B();
    }

    gazetteer_lists.resize(data.next_4B());
    for (auto&& gazetteer_list : gazetteer_lists) {
      gazetteer_list.gazetteers.resize(data.next_4B());
      for (auto&& gazetteer : gazetteer_list.gazetteers)
        data.next_str(gazetteer);
      gazetteer_list.feature = data.next_4B();
      gazetteer_list.entity = data.next_4B();
      gazetteer_list.mode = data.next_4B();
    }

    entity_list.resize(data.next_4B());
    for (auto&& entity : entity_list)
      data.next_str(entity);

    load_gazetteer_lists(pipeline, false);
  }

  virtual void save(binary_encoder& enc) override {
    feature_processor::save(enc);

    enc.add_4B(match);

    enc.add_4B(gazetteer_metas.size());
    for (auto&& gazetteer_meta : gazetteer_metas) {
      enc.add_str(gazetteer_meta.basename);
      enc.add_4B(gazetteer_meta.feature);
      enc.add_4B(gazetteer_meta.entity);
    }

    if (embed == EMBED_IN_MODEL) {
      enc.add_4B(gazetteer_lists.size());
      for (auto&& gazetteer_list : gazetteer_lists) {
        enc.add_4B(gazetteer_list.gazetteers.size());
        for (auto&& gazetteer : gazetteer_list.gazetteers)
          enc.add_str(gazetteer);
        enc.add_4B(gazetteer_list.feature);
        enc.add_4B(gazetteer_list.entity);
        enc.add_4B(gazetteer_list.mode);
      }
    } else {
      enc.add_4B(0);
    }

    enc.add_4B(entity_list.size());
    for (auto&& entity : entity_list)
      enc.add_str(entity);
  }

  virtual void process_sentence(ner_sentence& sentence, ner_feature* /*total_features*/, string& /*buffer*/) const override {
    vector<unsigned> nodes, new_nodes;
    vector<vector<ner_feature>> features(sentence.size);

    vector<vector<string>> recased_match_sources(sentence.size);
    for (unsigned i = 0; i < sentence.size; i++)
      recase_match_source(sentence.words[i], RECASE_ANY, recased_match_sources[i]);

    for (unsigned i = 0; i < sentence.size; i++) {
      unsigned hard_pre_length = 0, hard_pre_node = -1;
      bool hard_pre_possible = true;
      nodes.assign(1, 0);
      for (unsigned j = i; j < sentence.size && !nodes.empty(); j++) {
        new_nodes.clear();
        for (auto&& node : nodes)
          if (!gazetteers_trie[node].children.empty())
            for (auto&& match_source : recased_match_sources[j]) {
              auto range = gazetteers_trie[node].children.equal_range(match_source);
              for (auto&& it = range.first; it != range.second; it++)
                append_unless_exists(new_nodes, it->second);
            }

        hard_pre_possible = hard_pre_possible && !sentence.probabilities[j].local_filled;
        if (hard_pre_possible)
          for (auto&& node : new_nodes)
            if (gazetteers_trie[node].mode == HARD_PRE &&
                ((j - i + 1) > hard_pre_length || node < hard_pre_node))
              hard_pre_length = j - i + 1, hard_pre_node = node;

        // Fill features
        for (auto&& node : new_nodes)
          for (auto&& feature : gazetteers_trie[node].features)
            for (unsigned k = i; k <= j; k++) {
              bilou_type type = j == i ? bilou_type_U : k == i ? bilou_type_B : k == j ? bilou_type_L : bilou_type_I;
              append_unless_exists(features[k], feature + G * (2 * window + 1));
              append_unless_exists(features[k], feature + type * (2 * window + 1));
            }

        nodes.swap(new_nodes);
      }

      if (hard_pre_length)
        for (unsigned j = i; j < i + hard_pre_length; j++) {
          for (auto&& bilou : sentence.probabilities[j].local.bilou) {
            bilou.probability = 0.;
            bilou.entity = entity_type_unknown;
          }
          bilou_type type = hard_pre_length == 1 ? bilou_type_U :
              j == i ? bilou_type_B : j + 1 == i + hard_pre_length ? bilou_type_L : bilou_type_I;
          sentence.probabilities[j].local.bilou[type].probability = 1.;
          sentence.probabilities[j].local.bilou[type].entity = gazetteers_trie[hard_pre_node].entity;
          sentence.probabilities[j].local_filled = true;
        }
    }

    // Apply generated features
    for (unsigned i = 0; i < sentence.size; i++)
      for (auto&& feature : features[i])
        apply_in_window(i, feature);
  }

  virtual void process_entities(ner_sentence& sentence, vector<named_entity>& entities, vector<named_entity>& buffer) const override {
    vector<unsigned> nodes, new_nodes;

    vector<vector<string>> recased_match_sources(sentence.size);
    for (unsigned i = 0; i < sentence.size; i++)
      recase_match_source(sentence.words[i], RECASE_ANY, recased_match_sources[i]);

    buffer.clear();
    unsigned entity_until = 0;
    for (unsigned i = 0, e = 0; i < sentence.size; i++) {
      while (e < entities.size() && entities[e].start == i) {
        if (i + entities[e].length > entity_until)
          entity_until = i + entities[e].length;
        buffer.push_back(entities[e++]);
      }

      if (entity_until <= i) {
        // There is place for a possible POST gazetteer
        unsigned free_until = e < entities.size() ? entities[e].start : sentence.size;

        unsigned hard_post_length = 0, hard_post_node = -1;
        nodes.assign(1, 0);
        for (unsigned j = i; j < free_until && !nodes.empty(); j++) {
          new_nodes.clear();
          for (auto&& node : nodes)
            if (!gazetteers_trie[node].children.empty())
              for (auto&& match_source : recased_match_sources[j]) {
                auto range = gazetteers_trie[node].children.equal_range(match_source);
                for (auto&& it = range.first; it != range.second; it++)
                  append_unless_exists(new_nodes, it->second);
              }

          for (auto&& node : new_nodes)
            if (gazetteers_trie[node].mode == HARD_POST &&
                ((j - i + 1) > hard_post_length || node < hard_post_node))
              hard_post_length = j - i + 1, hard_post_node = node;

          nodes.swap(new_nodes);
        }

        if (hard_post_length) {
          buffer.emplace_back(i, hard_post_length, entity_list[gazetteers_trie[hard_post_node].entity]);
          entity_until = i + hard_post_length;
        }
      }
    }

    if (buffer.size() != entities.size())
      entities.swap(buffer);
  }

  virtual void gazetteers(vector<string>& gazetteers, vector<int>* gazetteer_types) const override {
    for (auto&& gazetteer_list : gazetteer_lists)
      for (auto&& gazetteer : gazetteer_list.gazetteers) {
        gazetteers.push_back(gazetteer);
        if (gazetteer_types) gazetteer_types->push_back(gazetteer_list.entity);
      }
  }

 private:
  enum { MATCH_FORM = 0, MATCH_RAWLEMMA = 1, MATCH_RAWLEMMAS = 2 };
  int match;

  enum { EMBED_IN_MODEL = 0, OUT_OF_MODEL = 1 };
  int embed;

  enum { SOFT, HARD_PRE, HARD_POST, MODES_TOTAL };
  const static vector<string> basename_suffixes;

  struct gazetteer_meta_info {
    string basename;
    ner_feature feature;
    int entity;
  };
  vector<gazetteer_meta_info> gazetteer_metas;

  struct gazetteer_list_info {
    vector<string> gazetteers;
    ner_feature feature;
    int entity;
    int mode;
  };
  vector<gazetteer_list_info> gazetteer_lists;

  struct gazetteer_trie_node {
    vector<ner_feature> features;
    unordered_multimap<string, unsigned> children;
    int mode = SOFT, entity = -1;
  };
  vector<gazetteer_trie_node> gazetteers_trie;

  vector<string> entity_list;

  template <class T>
  inline static void append_unless_exists(vector<T>& array, T value) {
    size_t i;
    for (i = array.size(); i; i--)
      if (array[i - 1] == value)
        break;

    if (!i)
      array.push_back(value);
  }

  bool load_gazetteer_lists(const nlp_pipeline& pipeline, bool files_must_exist) {
    string file_name, line;

    // Load raw gazetteers (maybe additional during inference)
    for (auto&& gazetteer_meta : gazetteer_metas)
      for (int mode = 0; mode < MODES_TOTAL; mode++) {
        file_name.assign(gazetteer_meta.basename).append(basename_suffixes[mode]);

        ifstream file(file_name);
        if (!file.is_open()) {
          if (mode == SOFT && files_must_exist)
            return cerr << "Cannot open gazetteers file '" << file_name << "'!" << endl, false;
          continue;
        }

        gazetteer_lists.emplace_back();
        gazetteer_lists.back().feature = gazetteer_meta.feature;
        gazetteer_lists.back().entity = gazetteer_meta.entity;
        gazetteer_lists.back().mode = mode;

        while (getline(file, line))
          if (!line.empty() && line[0] != '#')
            gazetteer_lists.back().gazetteers.push_back(line);
      }

    // Build the gazetteers_trie
    unordered_map<string, int> gazetteer_prefixes;
    vector<string_piece> gazetteer_tokens, gazetteer_tokens_additional, gazetteer_token(1);
    ner_sentence gazetteer_token_tagged;
    vector<string> gazetteer_recased_match_sources;

    gazetteers_trie.clear();
    gazetteers_trie.emplace_back();
    for (auto&& gazetteer_list : gazetteer_lists)
      for (auto&& gazetteer : gazetteer_list.gazetteers) {
        pipeline.tokenizer->set_text(gazetteer);
        if (!pipeline.tokenizer->next_sentence(&gazetteer_tokens, nullptr)) continue;
        while (pipeline.tokenizer->next_sentence(&gazetteer_tokens_additional, nullptr))
          gazetteer_tokens.insert(gazetteer_tokens.end(), gazetteer_tokens_additional.begin(), gazetteer_tokens_additional.end());

        unsigned node = 0;
        string prefix;
        for (unsigned token = 0; token < gazetteer_tokens.size(); token++) {
          if (token) prefix.push_back('\t');
          prefix.append(gazetteer_tokens[token].str, gazetteer_tokens[token].len);
          auto prefix_it = gazetteer_prefixes.find(prefix);
          if (prefix_it == gazetteer_prefixes.end()) {
            unsigned new_node = gazetteers_trie.size();
            gazetteers_trie.emplace_back();
            gazetteer_prefixes.emplace(prefix, new_node);

            gazetteer_token[0] = string_piece(gazetteer_tokens[token]);
            pipeline.tagger->tag(gazetteer_token, gazetteer_token_tagged);
            recase_match_source(gazetteer_token_tagged.words[0], RECASE_NATIVE, gazetteer_recased_match_sources);
            for (auto&& match_source : gazetteer_recased_match_sources)
              gazetteers_trie[node].children.emplace(match_source, new_node);

            node = new_node;
          } else {
            node = prefix_it->second;
          }
        }

        append_unless_exists(gazetteers_trie[node].features, gazetteer_list.feature);
        if ((gazetteer_list.mode == HARD_PRE && gazetteers_trie[node].mode != HARD_PRE) ||
            (gazetteer_list.mode == HARD_POST && gazetteers_trie[node].mode == SOFT)) {
          gazetteers_trie[node].mode = gazetteer_list.mode;
          gazetteers_trie[node].entity = gazetteer_list.entity;
        }
      }

    return true;
  }

  enum { TO_LOWER, TO_TITLE, TO_UPPER, TO_TOTAL };
  static void recase_text(const string& text, int mode, vector<string>& recased) {
    using namespace unilib;

    recased.emplace_back();

    if (mode == TO_UPPER)
      utf8::map(unicode::uppercase, text, recased.back());
    else if (mode == TO_LOWER)
      utf8::map(unicode::lowercase, text, recased.back());
    else if (mode == TO_TITLE)
      for (auto&& chr : utf8::decoder(text))
        utf8::append(recased.back(), recased.back().empty() ? unicode::uppercase(chr) : unicode::lowercase(chr));
  }

  enum { RECASE_NATIVE, RECASE_ANY };
  void recase_match_source(const ner_word& word, int mode, vector<string>& recased) const {
    using namespace unilib;

    bool any_lower = false, first_uc = false, first = true;
    for (auto&& chr : utf8::decoder(word.form)) {
      any_lower = any_lower || (unicode::category(chr) & unicode::Ll);
      if (first) first_uc = unicode::category(chr) & unicode::Lut;
      first = false;
    }

    recased.clear();

    for (int perform = 0; perform < TO_TOTAL; perform++) {
      if (mode == RECASE_NATIVE) {
        if (perform == TO_UPPER && !(first_uc && !any_lower)) continue;
        if (perform == TO_TITLE && !(first_uc && any_lower)) continue;
        if (perform == TO_LOWER && first_uc) continue;
      }
      if (mode == RECASE_ANY) {
        if (perform == TO_UPPER && !(first_uc && !any_lower)) continue;
        if (perform == TO_TITLE && !first_uc) continue;
      }

      if (match == MATCH_FORM)
        recase_text(word.form, perform, recased);
      else if (match == MATCH_RAWLEMMA)
        recase_text(word.raw_lemma, perform, recased);
      else if (match == MATCH_RAWLEMMAS)
        for (auto&& raw_lemma : word.raw_lemmas_all)
          recase_text(raw_lemma, perform, recased);
    }
  }
};
const vector<string> gazetteers_enhanced::basename_suffixes = {".txt", ".hard_pre.txt", ".hard_post.txt"};


// Lemma
class lemma : public feature_processor {
 public:
  virtual void process_sentence(ner_sentence& sentence, ner_feature* total_features, string& /*buffer*/) const override {
    for (unsigned i = 0; i < sentence.size; i++)
      apply_in_window(i, lookup(sentence.words[i].lemma_id, total_features));

    apply_outer_words_in_window(lookup_empty());
  }
};


// NumericTimeValue
class number_time_value : public feature_processor {
 public:
  virtual void process_sentence(ner_sentence& sentence, ner_feature* total_features, string& buffer) const override {
    ner_feature hour = lookup(buffer.assign("H"), total_features);
    ner_feature minute = lookup(buffer.assign("M"), total_features);
    ner_feature time = lookup(buffer.assign("t"), total_features);
    ner_feature day = lookup(buffer.assign("d"), total_features);
    ner_feature month = lookup(buffer.assign("m"), total_features);
    ner_feature year = lookup(buffer.assign("y"), total_features);

    for (unsigned i = 0; i < sentence.size; i++) {
      const char* form = sentence.words[i].form.c_str();
      unsigned num;
      bool digit;

      for (digit = false, num = 0; *form; form++) {
        if (*form < '0' || *form > '9') break;
        digit = true;
        num = num * 10 + *form - '0';
      }
      if (digit && !*form) {
        // We have a number
        if (num < 24) apply_in_window(i, hour);
        if (num < 60) apply_in_window(i, minute);
        if (num >= 1 && num <= 31) apply_in_window(i, day);
        if (num >= 1 && num <= 12) apply_in_window(i, month);
        if (num >= 1000 && num <= 2200) apply_in_window(i, year);;
      }
      if (digit && num < 24 && (*form == '.' || *form == ':')) {
        // Maybe time
        for (digit = false, num = 0, form++; *form; form++) {
          if (*form < '0' || *form > '9') break;
          digit = true;
          num = num * 10 + *form - '0';
        }
        if (digit && !*form && num < 60) apply_in_window(i, time);
      }
    }
  }
};


// PreviousStage
class previous_stage : public feature_processor {
 public:
  virtual void process_sentence(ner_sentence& sentence, ner_feature* total_features, string& buffer) const override {
    for (unsigned i = 0; i < sentence.size; i++)
      if (sentence.previous_stage[i].bilou != bilou_type_unknown) {
        buffer.clear();
        append_encoded(buffer, sentence.previous_stage[i].bilou);
        buffer.push_back(' ');
        append_encoded(buffer, sentence.previous_stage[i].entity);
        apply_in_range(i, lookup(buffer, total_features), 1, window);
      }
  }

 private:
  static void append_encoded(string& str, int value) {
    if (value < 0) {
      str.push_back('-');
      value = -value;
    }
    for (; value; value >>= 4)
      str.push_back("0123456789abcdef"[value & 0xF]);
  }
};


// RawLemma
class raw_lemma : public feature_processor {
 public:
  virtual void process_sentence(ner_sentence& sentence, ner_feature* total_features, string& /*buffer*/) const override {
    for (unsigned i = 0; i < sentence.size; i++)
      apply_in_window(i, lookup(sentence.words[i].raw_lemma, total_features));

    apply_outer_words_in_window(lookup_empty());
  }
};


// RawLemmaCapitalization
class raw_lemma_capitalization : public feature_processor {
 public:
  virtual void process_sentence(ner_sentence& sentence, ner_feature* total_features, string& buffer) const override {
    using namespace unilib;

    ner_feature fst_cap = lookup(buffer.assign("f"), total_features);
    ner_feature all_cap = lookup(buffer.assign("a"), total_features);
    ner_feature mixed_cap = lookup(buffer.assign("m"), total_features);

    for (unsigned i = 0; i < sentence.size; i++) {
      bool was_upper = false, was_lower = false;

      auto* raw_lemma = sentence.words[i].raw_lemma.c_str();
      char32_t chr;
      for (bool first = true; (chr = utf8::decode(raw_lemma)); first = false) {
        auto category = unicode::category(chr);
        was_upper = was_upper || category & unicode::Lut;
        was_lower = was_lower || category & unicode::Ll;

        if (first && was_upper) apply_in_window(i, fst_cap);
      }
      if (was_upper && !was_lower) apply_in_window(i, all_cap);
      if (was_upper && was_lower) apply_in_window(i, mixed_cap);
    }
  }
};


// RawLemmaCaseNormalized
class raw_lemma_case_normalized : public feature_processor {
 public:
  virtual void process_sentence(ner_sentence& sentence, ner_feature* total_features, string& buffer) const override {
    using namespace unilib;

    for (unsigned i = 0; i < sentence.size; i++) {
      buffer.clear();
      for (auto&& chr : utf8::decoder(sentence.words[i].raw_lemma))
        utf8::append(buffer, buffer.empty() ? chr : unicode::lowercase(chr));
      apply_in_window(i, lookup(buffer, total_features));
    }

    apply_outer_words_in_window(lookup_empty());
  }
};


// *Suffix
enum { SUFFIX_SOURCE_FORM, SUFFIX_SOURCE_RAWLEMMA };
enum { SUFFIX_CASE_ORIGINAL, SUFFIX_CASE_NORMALIZED };
class suffix : public feature_processor {
 public:
  suffix(int source, int casing) : source(source), casing(casing) {}

  virtual bool parse(int window, const vector<string>& args, entity_map& entities,
                     ner_feature* total_features, const nlp_pipeline& pipeline) override {
    if (!feature_processor::parse(window, args, entities, total_features, pipeline)) return false;
    if (args.size() != 2) return cerr << "*Suffix features require exactly two arguments -- shortest and longest suffix length!" << endl, false;

    string error;
    if (!parse_int(args[0], "*Suffix shortest length", shortest, error)) return cerr << error << endl, false;
    if (!parse_int(args[1], "*Suffix longest length", longest, error)) return cerr << error << endl, false;
    return true;
  }

  virtual void load(binary_decoder& data, const nlp_pipeline& pipeline) override {
    feature_processor::load(data, pipeline);

    shortest = data.next_4B();
    longest = data.next_4B();
  }

  virtual void save(binary_encoder& enc) override {
    feature_processor::save(enc);

    enc.add_4B(shortest);
    enc.add_4B(longest);
  }

  virtual void process_sentence(ner_sentence& sentence, ner_feature* total_features, string& buffer) const override {
    using namespace unilib;

    vector<char32_t> chrs;
    for (unsigned i = 0; i < sentence.size; i++) {
      chrs.clear();
      for (auto&& chr : utf8::decoder(source == SUFFIX_SOURCE_FORM ? sentence.words[i].form : sentence.words[i].raw_lemma))
        chrs.push_back((casing == SUFFIX_CASE_ORIGINAL || chrs.empty()) ? chr : unicode::lowercase(chr));

      buffer.clear();
      for (int s = 1; s <= longest && s <= int(chrs.size()); s++) {
        utf8::append(buffer, chrs[chrs.size() - s]);
        if (s >= shortest) {
          apply_in_window(i, lookup(buffer, total_features));
        }
      }
    }

    apply_outer_words_in_window(lookup_empty());
  }

 private:
  int shortest, longest;
  int source, casing;
};


// Tag
class tag : public feature_processor {
 public:
  virtual void process_sentence(ner_sentence& sentence, ner_feature* total_features, string& /*buffer*/) const override {
    for (unsigned i = 0; i < sentence.size; i++)
      apply_in_window(i, lookup(sentence.words[i].tag, total_features));

    apply_outer_words_in_window(lookup_empty());
  }
};


// URLEmailDetector
class url_email_detector : public feature_processor {
 public:
  virtual bool parse(int window, const vector<string>& args, entity_map& entities,
                     ner_feature* total_features, const nlp_pipeline& pipeline) override {
    if (!feature_processor::parse(window, args, entities, total_features, pipeline)) return false;
    if (args.size() != 2) return cerr << "URLEmailDetector requires exactly two arguments -- named entity types for URL and email!" << endl, false;

    url = entities.parse(args[0].c_str(), true);
    email = entities.parse(args[1].c_str(), true);

    if (url == entity_type_unknown || email == entity_type_unknown)
      return cerr << "Cannot create entities '" << args[0] << "' and '" << args[1] << "' in URLEmailDetector!" << endl, false;
    return true;
  }

  virtual void load(binary_decoder& data, const nlp_pipeline& pipeline) override {
    feature_processor::load(data, pipeline);

    url = data.next_4B();
    email = data.next_4B();
  }

  virtual void save(binary_encoder& enc) override {
    feature_processor::save(enc);

    enc.add_4B(url);
    enc.add_4B(email);
  }

  virtual void process_sentence(ner_sentence& sentence, ner_feature* /*total_features*/, string& /*buffer*/) const override {
    for (unsigned i = 0; i < sentence.size; i++) {
      auto type = url_detector::detect(sentence.words[i].form);
      if (type == url_detector::NO_URL || sentence.probabilities[i].local_filled) continue;

      // We have found URL or email and the word has not yet been determined
      for (auto&& bilou : sentence.probabilities[i].local.bilou) {
        bilou.probability = 0.;
        bilou.entity = entity_type_unknown;
      }
      sentence.probabilities[i].local.bilou[bilou_type_U].probability = 1.;
      sentence.probabilities[i].local.bilou[bilou_type_U].entity = type == url_detector::EMAIL ? email : url;
      sentence.probabilities[i].local_filled = true;
    }
  }

 private:
  entity_type url, email;
};


} // namespace feature_processors

// Feature processor factory method
feature_processor* feature_processor::create(const string& name) {
  using namespace feature_processors;

  if (name.compare("BrownClusters") == 0) return new brown_clusters();
  if (name.compare("CzechAddContainers") == 0) return new czech_add_containers();
  if (name.compare("CzechLemmaTerm") == 0) return new czech_lemma_term();
  if (name.compare("Form") == 0) return new form();
  if (name.compare("FormCapitalization") == 0) return new form_capitalization();
  if (name.compare("FormCaseNormalized") == 0) return new form_case_normalized();
  if (name.compare("FormCaseNormalizedSuffix") == 0) return new suffix(SUFFIX_SOURCE_FORM, SUFFIX_CASE_NORMALIZED);
  if (name.compare("FormSuffix") == 0) return new suffix(SUFFIX_SOURCE_FORM, SUFFIX_CASE_ORIGINAL);
  if (name.compare("Gazetteers") == 0) return new feature_processors::gazetteers();
  if (name.compare("GazetteersEnhanced") == 0) return new gazetteers_enhanced();
  if (name.compare("Lemma") == 0) return new lemma();
  if (name.compare("NumericTimeValue") == 0) return new number_time_value();
  if (name.compare("PreviousStage") == 0) return new previous_stage();
  if (name.compare("RawLemma") == 0) return new raw_lemma();
  if (name.compare("RawLemmaCapitalization") == 0) return new raw_lemma_capitalization();
  if (name.compare("RawLemmaCaseNormalized") == 0) return new raw_lemma_case_normalized();
  if (name.compare("RawLemmaCaseNormalizedSuffix") == 0) return new suffix(SUFFIX_SOURCE_RAWLEMMA, SUFFIX_CASE_NORMALIZED);
  if (name.compare("RawLemmaSuffix") == 0) return new suffix(SUFFIX_SOURCE_RAWLEMMA, SUFFIX_CASE_ORIGINAL);
  if (name.compare("Tag") == 0) return new tag();
  if (name.compare("URLEmailDetector") == 0) return new url_email_detector();
  return nullptr;
}

} // namespace nametag
} // namespace ufal
