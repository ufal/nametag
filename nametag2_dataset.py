#!/usr/bin/env python3
# coding=utf-8
#
# Copyright 2021 Institute of Formal and Applied Linguistics, Faculty of
# Mathematics and Physics, Charles University, Czech Republic.
#
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http://mozilla.org/MPL/2.0/.

"""NameTag2Dataset class to handle NE tagged data."""

import io
import pickle
import sys

import numpy as np

import wembedding_service.wembeddings.wembeddings as wembeddings

class NameTag2Dataset:
    """Class for loading NE tagged datasets in vertical format.
    The dataset is composed of factors (by default FORMS and TAGS),
    each an object containing the following fields:
    - strings: Strings of the original words.
    - word_ids: Word ids of the original words (uses <unk> and <pad>).
    - words_map: String -> word_id map.
    - words: Word_id -> string list.
    - alphabet_map: Character -> char_id map.
    - alphabet: Char_id -> character list.
    - charseq_ids: Character_sequence ids of the original words.
    - charseqs_map: String -> character_sequence_id map.
    - charseqs: Character_sequence_id -> [characters], where character is an index
        to the dataset alphabet.
    """
    FORMS = 0
    TAGS = 1
    FACTORS = 2

    class _Factor:
        def __init__(self, train=None):
            self.words_map = train.words_map if train else {'<pad>': 0, '<unk>': 1, '<eow>': 2, '<bos>': 3}
            self.words = train.words if train else ['<pad>', '<unk>', '<eow>', '<bos>']
            self.word_ids = []
            self.alphabet_map = train.alphabet_map if train else {'<pad>': 0, '<unk>': 1, '<bow>': 2, '<eow>': 3}
            self.alphabet = train.alphabet if train else ['<pad>', '<unk>', '<bow>', '<eow>']
            self.charseqs_map = {'<pad>': 0}
            self.charseqs = [[self.alphabet_map['<pad>']]]
            self.charseq_ids = []
            self.strings = []

    def __init__(self, filename=None, text=None, train=None,
                 shuffle_batches=True, max_sentences=None,
                 add_bow_eow=False, seq2seq=False,
                 bert_filename=None, bert=None):
        """Load dataset from file in vertical format.
        Arguments:
        - add_bow_eow: Whether to add BOW/EOW characters to the word characters.
        - seq2seq: Multiple labels may be predicted.
        - train: If given, the words and alphabets are reused from the training data.
        """

        # Create word maps
        self._factors = []
        for f in range(self.FACTORS):
            self._factors.append(self._Factor(train._factors[f] if train else None))

        # Load the sentences
        with open(filename, "r", encoding="utf-8") if filename is not None else io.StringIO(text) as file:
            in_sentence = False
            for line in file:
                line = line.rstrip("\r\n")
                if line:
                    columns = line.split("\t")
                    for f in range(self.FACTORS):
                        factor = self._factors[f]
                        if not in_sentence:
                            factor.word_ids.append([])
                            factor.charseq_ids.append([])
                            factor.strings.append([])
                        column = columns[f] if f < len(columns) else '<pad>'
                        words = []
                        if f == self.TAGS and seq2seq:
                            words = column.split("|")
                            words.append("<eow>")
                        else:
                            words = [column]
                        for word in words:
                            factor.strings[-1].append(word)

                            # Character-level information
                            if word not in factor.charseqs_map:
                                factor.charseqs_map[word] = len(factor.charseqs)
                                factor.charseqs.append([])
                                if add_bow_eow:
                                    factor.charseqs[-1].append(factor.alphabet_map['<bow>'])
                                for c in word:
                                    if c not in factor.alphabet_map:
                                        if train:
                                            c = '<unk>'
                                        else:
                                            factor.alphabet_map[c] = len(factor.alphabet)
                                            factor.alphabet.append(c)
                                    factor.charseqs[-1].append(factor.alphabet_map[c])
                                if add_bow_eow:
                                    factor.charseqs[-1].append(factor.alphabet_map['<eow>'])
                            factor.charseq_ids[-1].append(factor.charseqs_map[word])

                            # Word-level information
                            if word not in factor.words_map:
                                if train:
                                    word = '<unk>'
                                else:
                                    factor.words_map[word] = len(factor.words)
                                    factor.words.append(word)
                            factor.word_ids[-1].append(factor.words_map[word])
                    in_sentence = True
                else:
                    in_sentence = False
                    if max_sentences is not None and len(self._factors[self.FORMS].word_ids) >= max_sentences:
                        break

        # Compute sentence lengths
        sentences = len(self._factors[self.FORMS].word_ids)
        self._sentence_lens = np.zeros([sentences], np.int32)
        for i in range(len(self._factors[self.FORMS].word_ids)):
            self._sentence_lens[i] = len(self._factors[self.FORMS].word_ids[i])

        # Compute tag lengths
        tags = len(self._factors[self.TAGS].word_ids)
        self._tag_lens = np.zeros([tags], np.int32)
        for i in range(len(self._factors[self.TAGS].word_ids)):
            self._tag_lens[i] = len(self._factors[self.TAGS].word_ids[i])

        self._shuffle_batches = shuffle_batches
        self._permutation = np.random.permutation(len(self._sentence_lens)) if self._shuffle_batches else np.arange(len(self._sentence_lens))

        ### BERT embeddings ###

        # Default values: no BERT embeddings
        self._bert_dim = 0
        self._bert = None               # ip:port
        self._bert_embeddings = None    # [sentences x words x bert_embeddings]
        
        if bert:
            print("BERT will be computed for each batch by requests to {}".format(bert), file=sys.stderr, flush=True)
            # BERT will be computed online for each batch by requests to server
            self._bert = wembeddings.WEmbeddings.ClientNetwork(bert) # ip:port
            self._bert_dim = 768
        elif bert_filename:
            # BERT embeddings from file
            print("Loading BERT from " + str(bert_filename), file=sys.stderr, flush=True)
            self._bert_embeddings = []  # [sentences x words x bert_embeddings]
            with np.load(bert_filename, allow_pickle=True) as embeddings_file:
                for _, value in embeddings_file.items():
                    self._bert_embeddings.append(value)
            self._bert_dim = self._bert_embeddings[0].shape[1]


    @property
    def bert_embeddings(self):
        return self._bert_embeddings

    @property
    def sentence_lens(self):
        return self._sentence_lens

    @property
    def tag_lens(self):
        return self._tag_lens

    @property
    def factors(self):
        """Return the factors of the dataset.
        The result is an array of factors, each an object containing:
        strings: Strings of the original words.
        word_ids: Word ids of the original words (uses <unk> and <pad>).
        words_map: String -> word_id map.
        words: Word_id -> string list.
        alphabet_map: Character -> char_id map.
        alphabet: Char_id -> character list.
        charseq_ids: Character_sequence ids of the original words.
        charseqs_map: String -> character_sequence_id map.
        charseqs: Character_sequence_id -> [characters], where character is an index
          to the dataset alphabet.
        """

        return self._factors

    def save_mappings(self, path):
        mappings = NameTag2Dataset.__new__(NameTag2Dataset)
        mappings._factors = []
        for factor in self._factors:
            mappings._factors.append(NameTag2Dataset._Factor.__new__(NameTag2Dataset._Factor))
            for member in ["words_map", "words", "alphabet_map", "alphabet"]:
                setattr(mappings._factors[-1], member, getattr(factor, member))
        mappings._bert_dim = self._bert_dim

        with open(path, "wb") as mappings_file:
            pickle.dump(mappings, mappings_file, protocol=3)

    def next_batch(self, batch_size, including_charseqs=False, seq2seq=False):
        """Return the next batch.
        Arguments:
        including_charseqs: if True, also batch_charseq_ids, batch_charseqs and batch_charseq_lens are returned
        Returns: 
        {sentence_lens, batch_word_ids, batch_charseq_ids, batch_charseqs, batch_pretrained_wes}
        sequence_lens: batch of sentence_lens
        batch_word_ids: for each factor, batch of words_id
        batch_charseq_ids: For each factor, batch of charseq_ids
          (the same shape as words_id, but with the ids pointing into batch_charseqs).
          Returned only if including_charseqs is True.
        batch_charseqs: For each factor, all unique charseqs in the batch,
          indexable by batch_charseq_ids. Contains indices of characters from self.alphabet.
          Returned only if including_charseqs is True.
        batch_charseq_lens: For each factor, length of charseqs in batch_charseqs.
          Returned only if including_charseqs is True.
        """

        batch_size = min(batch_size, len(self._permutation))
        batch_perm = self._permutation[:batch_size]
        self._permutation = self._permutation[batch_size:]
        return self._next_batch(batch_perm, including_charseqs, seq2seq)

    def epoch_finished(self):
        if len(self._permutation) == 0:
            self._permutation = np.random.permutation(len(self._sentence_lens)) if self._shuffle_batches else np.arange(len(self._sentence_lens))
            return True
        return False
    
    def bert_dim(self):
        return self._bert_dim

    def _next_batch(self, batch_perm, including_charseqs, seq2seq=False):
        batch_size = len(batch_perm)
        batch_dict = dict()

        # General data
        batch_sentence_lens = self._sentence_lens[batch_perm]
        max_sentence_len = np.max(batch_sentence_lens)

        if seq2seq:
            batch_tag_lens = self._tag_lens[batch_perm]
            max_tag_len = np.max(batch_tag_lens)

        # Word-level data
        batch_word_ids = []
        for f in range(self.FACTORS):
            factor = self._factors[f]
            if f == self.TAGS and seq2seq:
                batch_word_ids.append(np.zeros([batch_size, max_tag_len], np.int32))
                for i in range(batch_size):
                    batch_word_ids[-1][i, 0:batch_tag_lens[i]] = factor.word_ids[batch_perm[i]]
            else:
                batch_word_ids.append(np.zeros([batch_size, max_sentence_len], np.int32))
                for i in range(batch_size):
                    batch_word_ids[-1][i, 0:batch_sentence_lens[i]] = factor.word_ids[batch_perm[i]]

        batch_dict["sentence_lens"] = self._sentence_lens[batch_perm]
        batch_dict["word_ids"] = batch_word_ids

        # Character-level data
        if including_charseqs: 
            batch_charseq_ids, batch_charseqs, batch_charseq_lens = [], [], []

            for f in range(self.FACTORS):
                if not (f == self.TAGS and seq2seq):
                    factor = self._factors[f]
                    batch_charseq_ids.append(np.zeros([batch_size, max_sentence_len], np.int32))
                    charseqs_map = {}
                    charseqs = []
                    for i in range(batch_size):
                        for j, charseq_id in enumerate(factor.charseq_ids[batch_perm[i]]):
                            if charseq_id not in charseqs_map:
                                charseqs_map[charseq_id] = len(charseqs)
                                charseqs.append(factor.charseqs[charseq_id])
                            batch_charseq_ids[-1][i, j] = charseqs_map[charseq_id]

                    batch_charseq_lens.append(np.array([len(charseq) for charseq in charseqs], np.int32))
                    batch_charseqs.append(np.zeros([len(charseqs), np.max(batch_charseq_lens[-1])], np.int32))
                    for i in range(len(charseqs)):
                        batch_charseqs[-1][i, 0:len(charseqs[i])] = charseqs[i]
            batch_dict["batch_charseq_ids"] = batch_charseq_ids
            batch_dict["batch_charseqs"] = batch_charseqs
            batch_dict["batch_charseq_lens"] = batch_charseq_lens

        # Precomputed BERT embeddings from file
        if self._bert_embeddings:
            we_size = self.bert_dim()
            batch_bert = np.zeros([batch_size, max_sentence_len, we_size], np.float32)
            for i in range(batch_size):
                batch_bert[i, 0:batch_sentence_lens[i]] = self._bert_embeddings[batch_perm[i]]
            batch_dict["batch_bert_wes"] = batch_bert

        # Compute BERT embeddings for this batch by requesting server.
        # Rewrites the BERT offline embeddings, if both are given.
        if self._bert:
            batch_bert = np.zeros([batch_size, max_sentence_len, self.bert_dim()], np.float32)
            batch_sentences = [ self._factors[self.FORMS].strings[batch_perm[i]] for i in range(batch_size) ]

            # Compute BERT embeddings for this batch by requesting server self._bert
            for i, embeddings in enumerate(self._bert.compute_embeddings("bert-base-multilingual-uncased-last4", batch_sentences)):
                batch_bert[i, :len(batch_sentences[i])] = embeddings
            batch_dict["batch_bert_wes"] = batch_bert

        return batch_dict
