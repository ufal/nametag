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

#include <algorithm>

#include "sentence_processor.h"
#include "utils/encode_utils.h"
#include "utils/file_ptr.h"
#include "utils/input.h"
#include "utils/parse_int.h"
#include "utils/url_detector.h"
#include "utils/utf8.h"

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
// Sentence processor instances (ordered lexicographically) //
//////////////////////////////////////////////////////////////
namespace sentence_processors {

// BrownClusters
class brown_clusters : public sentence_processor {
 public:
  virtual bool parse(int window, const vector<string>& args, entity_map& entities, ner_feature* total_features) override {
    if (!sentence_processor::parse(window, args, entities, total_features)) return false;
    if (args.size() < 1) return eprintf("BrownCluster requires a cluster file as the first argument!\n"), false;

    file_ptr f = fopen(args[0].c_str(), "r");
    if (!f) return eprintf("Cannot open Brown clusters file '%s'!\n", args[0].c_str()), false;

    vector<size_t> substrings;
    substrings.emplace_back(string::npos);
    for (unsigned i = 1; i < args.size(); i++) {
      int len = parse_int(args[i].c_str(), "BrownCluster_prefix_length");
      if (len <= 0)
        return eprintf("Wrong prefix length '%d' in BrownCluster specification!\n", len);
      else
        substrings.emplace_back(len);
    }

    clusters.clear();
    unordered_map<string, unsigned> cluster_map;
    unordered_map<string, ner_feature> prefixes_map;
    string line;
    vector<string> tokens;
    while (getline(f, line)) {
      split(line, '\t', tokens);
      if (tokens.size() != 2) return eprintf("Wrong line '%s' in Brown cluster file '%s'!\n", line.c_str(), args[0].c_str());

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
      if (!map.emplace(form, it->second).second) return eprintf("Form '%s' is present twice in Brown cluster file '%s'!\n", form.c_str(), args[0].c_str());
    }

    *total_features += (2*window + 1) * prefixes_map.size();
    return true;
  }

  virtual void load(binary_decoder& data) override {
    sentence_processor::load(data);

    clusters.resize(data.next_4B());
    for (auto&& cluster : clusters) {
      cluster.resize(data.next_4B());
      for (auto&& feature : cluster)
        feature = data.next_4B();
    }
  }

  virtual void save(binary_encoder& enc) override {
    sentence_processor::save(enc);

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


// CzechLemmaTerm
class czech_lemma_term : public sentence_processor {
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
class form : public sentence_processor {
 public:
  virtual void process_sentence(ner_sentence& sentence, ner_feature* total_features, string& /*buffer*/) const override {
    for (unsigned i = 0; i < sentence.size; i++)
      apply_in_window(i, lookup(sentence.words[i].form, total_features));

    apply_outer_words_in_window(lookup_empty());
  }
};


// Gazetteers
class gazetteers : public sentence_processor {
 public:
  enum { G = 0, U = 1, B = 2, L = 3, I = 4 };

  virtual bool parse(int window, const vector<string>& args, entity_map& entities, ner_feature* total_features) override {
    if (!sentence_processor::parse(window, args, entities, total_features)) return false;

    gazetteers.clear();
    for (auto&& arg : args) {
      file_ptr f = fopen(arg.c_str(), "r");
      if (!f) return eprintf("Cannot open gazetteers file '%s'!\n", arg.c_str()), false;

      unsigned longest = 0;
      string gazetteer;
      string line;
      vector<string> tokens;
      while (getline(f, line)) {
        split(line, ' ', tokens);
        for (unsigned i = 0; i < tokens.size(); i++)
          if (!tokens[i][0])
            tokens.erase(tokens.begin() + i--);
        if (tokens.size() > longest) longest = tokens.size();

        gazetteer.clear();
        for (unsigned i = 0; i < tokens.size(); i++) {
          if (i) gazetteer += ' ';
          gazetteer += tokens[i];
          auto it = map.emplace(gazetteer, gazetteers.size()).first;
          if (it->second == gazetteers.size()) gazetteers.emplace_back();
          auto& info = gazetteers[it->second];
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

  virtual void load(binary_decoder& data) override {
    sentence_processor::load(data);

    gazetteers.resize(data.next_4B());
    for (auto&& gazetteer : gazetteers) {
      gazetteer.prefix_of_longer = data.next_1B();
      gazetteer.features.resize(data.next_1B());
      for (auto&& feature : gazetteer.features)
        feature = data.next_4B();
    }
  }

  virtual void save(binary_encoder& enc) override {
    sentence_processor::save(enc);

    enc.add_4B(gazetteers.size());
    for (auto&& gazetteer : gazetteers) {
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
      for (auto&& feature : gazetteers[it->second].features) {
        apply_in_window(i, feature + G * (2*window + 1));
        apply_in_window(i, feature + U * (2*window + 1));
      }

      for (unsigned j = i + 1; gazetteers[it->second].prefix_of_longer && j < sentence.size; j++) {
        if (j == i + 1) buffer.assign(sentence.words[i].raw_lemma);
        buffer += ' ';
        buffer += sentence.words[j].raw_lemma;
        it = map.find(buffer);
        if (it == map.end()) break;

        // Apply regular gazetteer feature G + position specific gazetteers B, I, L
        for (auto&& feature : gazetteers[it->second].features)
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
  vector<gazetteer_info> gazetteers;
};


// Lemma
class lemma : public sentence_processor {
 public:
  virtual void process_sentence(ner_sentence& sentence, ner_feature* total_features, string& /*buffer*/) const override {
    for (unsigned i = 0; i < sentence.size; i++)
      apply_in_window(i, lookup(sentence.words[i].lemma_id, total_features));

    apply_outer_words_in_window(lookup_empty());
  }
};


// PreviousStage
class previous_stage : public sentence_processor {
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
};


// RawLemma
class raw_lemma : public sentence_processor {
 public:
  virtual void process_sentence(ner_sentence& sentence, ner_feature* total_features, string& /*buffer*/) const override {
    for (unsigned i = 0; i < sentence.size; i++)
      apply_in_window(i, lookup(sentence.words[i].raw_lemma, total_features));

    apply_outer_words_in_window(lookup_empty());
  }
};


// RawLemmaCapitalization
class raw_lemma_capitalization : public sentence_processor {
 public:
  virtual void process_sentence(ner_sentence& sentence, ner_feature* total_features, string& buffer) const override {
    ner_feature fst_cap = lookup(buffer.assign("f"), total_features);
    ner_feature all_cap = lookup(buffer.assign("a"), total_features);
    ner_feature mixed_cap = lookup(buffer.assign("m"), total_features);

    for (unsigned i = 0; i < sentence.size; i++) {
      bool was_upper = false, was_lower = false;

      auto* raw_lemma = sentence.words[i].raw_lemma.c_str();
      char32_t chr;
      for (bool first = true; (chr = utf8::decode(raw_lemma)); first = false) {
        was_upper |= utf8::is_Lut(chr);
        was_lower |= utf8::is_Ll(chr);

        if (first && was_upper) apply_in_window(i, fst_cap);
      }
      if (was_upper && !was_lower) apply_in_window(i, all_cap);
      if (was_upper && was_lower) apply_in_window(i, mixed_cap);
    }
  }
};


// Tag
class tag : public sentence_processor {
 public:
  virtual void process_sentence(ner_sentence& sentence, ner_feature* total_features, string& /*buffer*/) const override {
    for (unsigned i = 0; i < sentence.size; i++)
      apply_in_window(i, lookup(sentence.words[i].tag, total_features));

    apply_outer_words_in_window(lookup_empty());
  }
};


// URLEmailDetector
class url_email_detector : public sentence_processor {
 public:
  virtual bool parse(int window, const vector<string>& args, entity_map& entities, ner_feature* total_features) override {
    if (!sentence_processor::parse(window, args, entities, total_features)) return false;
    if (args.size() != 2) return eprintf("URLEmailDetector requires exactly two arguments -- named entity types for URL and email!\n"), false;

    url = entities.parse(args[0].c_str(), true);
    email = entities.parse(args[1].c_str(), true);

    if (url == entity_type_unknown || email == entity_type_unknown)
      return eprintf("Cannot create entities '%s' and '%s' in URLEmailDetector!\n", args[0].c_str(), args[1].c_str()), false;
    return true;
  }

  virtual void load(binary_decoder& data) override {
    sentence_processor::load(data);

    url = data.next_4B();
    email = data.next_4B();
  }

  virtual void save(binary_encoder& enc) override {
    sentence_processor::save(enc);

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


} // namespace sentence_processors

// Sentence processor factory method
sentence_processor* sentence_processor::create(const string& name) {
  if (name.compare("BrownClusters") == 0) return new sentence_processors::brown_clusters();
  if (name.compare("CzechLemmaTerm") == 0) return new sentence_processors::czech_lemma_term();
  if (name.compare("Form") == 0) return new sentence_processors::form();
  if (name.compare("Gazetteers") == 0) return new sentence_processors::gazetteers();
  if (name.compare("Lemma") == 0) return new sentence_processors::lemma();
  if (name.compare("PreviousStage") == 0) return new sentence_processors::previous_stage();
  if (name.compare("RawLemma") == 0) return new sentence_processors::raw_lemma();
  if (name.compare("RawLemmaCapitalization") == 0) return new sentence_processors::raw_lemma_capitalization();
  if (name.compare("Tag") == 0) return new sentence_processors::tag();
  if (name.compare("URLEmailDetector") == 0) return new sentence_processors::url_email_detector();
  return nullptr;
}

} // namespace nametag
} // namespace ufal
