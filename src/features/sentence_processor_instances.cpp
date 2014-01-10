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
#include "utils/file_ptr.h"
#include "utils/input.h"
#include "utils/parse_int.h"
#include "utils/utf8.h"

namespace ufal {
namespace nametag {

// Helper functions defined as macros so that they can access arguments without passing them
#define apply_in_window(I, Feature) {                                                             \
  ner_feature _feature = (Feature);                                                               \
  if (_feature != ner_feature_unknown)                                                            \
    for (int _w = int(I) - window < 0 ? 0 : int(I) - window,                                      \
           _end = int(I) + window + 1 < int(sentence.size) ? int(I) + window + 1 : sentence.size; \
         _w < _end; _w++)                                                                         \
      sentence.features[_w].emplace_back(offset + _feature + _w - int(I));                        \
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
  virtual bool init(int window, const vector<string>& args) override {
    if (!sentence_processor::init(window, args)) return false;
    if (args.size() < 1) return eprintf("BrownCluster requires a cluster file as the first argument!\n"), false;

    map.clear();
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
        clusters.emplace_back(substrings.size());
        for (auto& substring : substrings)
          if (substring == string::npos || substring < cluster.size())
            clusters.back().emplace_back(prefixes_map.emplace(cluster.substr(0, substring), (2*window + 1) * prefixes_map.size() + window).first->second);
        it = cluster_map.emplace(cluster, id).first;
      }
      if (!map.emplace(form, it->second).second) return eprintf("Form '%s' is present twice in Brown cluster file '%s'!\n", form.c_str(), args[0].c_str());
    }

    total_features = (2*window + 1) * prefixes_map.size();
    return true;
  }

  virtual void load(binary_decoder& data) override {
    sentence_processor::load(data);

    clusters.resize(data.next_4B());
    for (auto& cluster : clusters) {
      cluster.resize(data.next_4B());
      for (auto& feature : cluster)
        feature = data.next_4B();
    }
  }

  virtual void save(binary_encoder& enc) override {
    sentence_processor::save(enc);

    enc.add_4B(clusters.size());
    for (auto& cluster : clusters) {
      enc.add_4B(cluster.size());
      for (auto& feature : cluster)
        enc.add_4B(feature);
    }
  }

  ner_feature freeze(entity_map& entities) {
    sentence_processor::freeze(entities);
    return total_features;
  }

  virtual void process_sentence(ner_sentence& sentence, ner_feature offset, string& /*buffer*/) const override {
    for (unsigned i = 0; i < sentence.size; i++) {
      auto it = map.find(sentence.words[i].raw_lemma);
      if (it != map.end()) {
        auto& cluster = clusters[it->second];
        for (auto& feature : cluster)
          apply_in_window(i, feature);
      }
    }
  }

 private:
  ner_feature total_features;
  vector<vector<ner_feature>> clusters;
};


// CzechLemmaTerm
class czech_lemma_term : public sentence_processor {
 public:
  virtual void process_sentence(ner_sentence& sentence, ner_feature offset, string& buffer) const override {
    for (unsigned i = 0; i < sentence.size; i++) {
      for (unsigned pos = 0; pos + 2 < sentence.words[i].lemma_comments.size(); pos++)
        if (sentence.words[i].lemma_comments[pos] == '_' && sentence.words[i].lemma_comments[pos+1] == ';') {
          buffer.assign(1, sentence.words[i].lemma_comments[pos+2]);
          apply_in_window(i, lookup(buffer));
        }
    }
  }
};


// Form
class form : public sentence_processor {
 public:
  virtual void process_sentence(ner_sentence& sentence, ner_feature offset, string& /*buffer*/) const override {
    for (unsigned i = 0; i < sentence.size; i++)
      apply_in_window(i, lookup(sentence.words[i].form));

    apply_outer_words_in_window(lookup_empty());
  }
};


// Gazeteers
class gazeteers : public sentence_processor {
 public:
  enum { G = 0, U = 1, B = 2, L = 3, I = 4 };

  virtual bool init(int window, const vector<string>& args) override {
    if (!sentence_processor::init(window, args)) return false;

    total_features = 0;
    gazeteers.clear();
    for (auto& arg : args) {
      file_ptr f = fopen(arg.c_str(), "r");
      if (!f) return eprintf("Cannot open gazeteers file '%s'!\n", arg.c_str()), false;

      unsigned longest = 0;
      string gazeteer;
      string line;
      vector<string> tokens;
      while (getline(f, line)) {
        split(line, ' ', tokens);
        for (unsigned i = 0; i < tokens.size(); i++)
          if (!tokens[i][0])
            tokens.erase(tokens.begin() + i--);
        if (tokens.size() > longest) longest = tokens.size();

        gazeteer.clear();
        for (unsigned i = 0; i < tokens.size(); i++) {
          if (i) gazeteer += ' ';
          gazeteer += tokens[i];
          auto it = map.emplace(gazeteer, gazeteers.size()).first;
          if (it->second == gazeteers.size()) gazeteers.emplace_back();
          auto& info = gazeteers[it->second];
          if (i + 1 < tokens.size())
            info.prefix_of_longer |= true;
          else
            if (find(info.features.begin(), info.features.end(), total_features + window) == info.features.end())
              info.features.emplace_back(total_features + window);
        }
      }
      total_features += (2*window + 1) * (longest == 0 ? 0 : longest == 1 ? U+1 : longest == 2 ? L+1 : I+1);
    }

    return true;
  }

  virtual void load(binary_decoder& data) override {
    sentence_processor::load(data);

    gazeteers.resize(data.next_4B());
    for (auto& gazeteer : gazeteers) {
      gazeteer.prefix_of_longer = data.next_1B();
      gazeteer.features.resize(data.next_1B());
      for (auto& feature : gazeteer.features)
        feature = data.next_4B();
    }
  }

  virtual void save(binary_encoder& enc) override {
    sentence_processor::save(enc);

    enc.add_4B(gazeteers.size());
    for (auto& gazeteer : gazeteers) {
      enc.add_1B(gazeteer.prefix_of_longer);
      enc.add_1B(gazeteer.features.size());
      for (auto& feature : gazeteer.features)
        enc.add_4B(feature);
    }
  }

  virtual ner_feature freeze(entity_map& entities) override {
    sentence_processor::freeze(entities);
    return total_features;
  }

  virtual void process_sentence(ner_sentence& sentence, ner_feature offset, string& buffer) const override {
    for (unsigned i = 0; i < sentence.size; i++) {
      auto it = map.find(sentence.words[i].raw_lemma);
      if (it == map.end()) continue;

      // Apply regular gazeteer feature G + unigram gazeteer feature U
      for (auto& feature : gazeteers[it->second].features) {
        apply_in_window(i, feature + G * (2*window + 1));
        apply_in_window(i, feature + U * (2*window + 1));
      }

      for (unsigned j = i + 1; gazeteers[it->second].prefix_of_longer && j < sentence.size; j++) {
        if (j == i + 1) buffer.assign(sentence.words[i].raw_lemma);
        buffer += ' ';
        buffer += sentence.words[j].raw_lemma;
        it = map.find(buffer);
        if (it == map.end()) break;

        // Apply regular gazeteer feature G + position specific gazeteers B, I, L
        for (auto& feature : gazeteers[it->second].features)
          for (unsigned g = i; g <= j; g++) {
            apply_in_window(g, feature + G * (2*window + 1));
            apply_in_window(g, feature + (g == i ? B : g == j ? L : I) * (2*window + 1));
          }
      }
    }
  }

 private:
  ner_feature total_features;
  struct gazeteer_info {
    vector<ner_feature> features;
    bool prefix_of_longer;
  };
  vector<gazeteer_info> gazeteers;
};


// Lemma
class lemma : public sentence_processor {
 public:
  virtual void process_sentence(ner_sentence& sentence, ner_feature offset, string& /*buffer*/) const override {
    for (unsigned i = 0; i < sentence.size; i++)
      apply_in_window(i, lookup(sentence.words[i].lemma_id));

    apply_outer_words_in_window(lookup_empty());
  }
};


// RawLemma
class raw_lemma : public sentence_processor {
 public:
  virtual void process_sentence(ner_sentence& sentence, ner_feature offset, string& /*buffer*/) const override {
    for (unsigned i = 0; i < sentence.size; i++)
      apply_in_window(i, lookup(sentence.words[i].raw_lemma));

    apply_outer_words_in_window(lookup_empty());
  }
};


// RawLemmaCapitalization
class raw_lemma_capitalization : public sentence_processor {
 public:
  virtual void process_sentence(ner_sentence& sentence, ner_feature offset, string& /*buffer*/) const override {
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

  virtual ner_feature freeze(entity_map& entities) override {
    lookup_cap_features();
    return sentence_processor::freeze(entities);
  }

  virtual void load(binary_decoder& data) override {
    sentence_processor::load(data);
    lookup_cap_features();
  }

 private:
  ner_feature fst_cap, all_cap, mixed_cap;
  void lookup_cap_features() { fst_cap = lookup("f"); all_cap = lookup("a"); mixed_cap = lookup("m"); }
};


// Tag
class tag : public sentence_processor {
 public:
  virtual void process_sentence(ner_sentence& sentence, ner_feature offset, string& /*buffer*/) const override {
    for (unsigned i = 0; i < sentence.size; i++)
      apply_in_window(i, lookup(sentence.words[i].tag));

    apply_outer_words_in_window(lookup_empty());
  }
};

} // namespace sentence_processors

// Sentence processor factory method
sentence_processor* sentence_processor::create(const string& name) {
  if (name.compare("BrownClusters") == 0) return new sentence_processors::brown_clusters();
  if (name.compare("CzechLemmaTerm") == 0) return new sentence_processors::czech_lemma_term();
  if (name.compare("Form") == 0) return new sentence_processors::form();
  if (name.compare("Gazeteers") == 0) return new sentence_processors::gazeteers();
  if (name.compare("Lemma") == 0) return new sentence_processors::lemma();
  if (name.compare("RawLemma") == 0) return new sentence_processors::raw_lemma();
  if (name.compare("RawLemmaCapitalization") == 0) return new sentence_processors::raw_lemma_capitalization();
  if (name.compare("Tag") == 0) return new sentence_processors::tag();
  return nullptr;
}

} // namespace nametag
} // namespace ufal
