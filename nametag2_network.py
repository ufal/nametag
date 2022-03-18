#!/usr/bin/env python3
# coding=utf-8
#
# Copyright 2021 Institute of Formal and Applied Linguistics, Faculty of
# Mathematics and Physics, Charles University, Czech Republic.
#
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http://mozilla.org/MPL/2.0/.

"""NameTag TensorFlow Neural Network.

The main prediction method is Network.predict:

Predicts labels for NameTag2Dataset (see nametag2_dataset.py).
No sanity check of the neural network output is done, which means:

1. Neither correct nesting of the entities, nor correct entity openings and
closing (correct bracketing) are guaranteed.

2. Labels and their encoding (BIO vs. IOB) is the exact same as in the model
trained from and underlying corpus (i.e., IOB found in English CoNLL-2003
dataset).

Please see Network.postprocess for correct bracketing and BIO formatting of
the output.
"""

import sys
import time

import numpy as np
import tensorflow as tf

# Disable TF warnings
tf.logging.set_verbosity(tf.logging.ERROR)

class Network:
    """NameTag TensorFlow Neural Network class."""

    def __init__(self, threads, seed=42):
        # Create an empty graph and a session
        graph = tf.Graph()
        graph.seed = seed
        self.session = tf.Session(graph = graph, config=tf.ConfigProto(inter_op_parallelism_threads=threads,
                                                                       intra_op_parallelism_threads=threads))

    def construct(self, args, num_forms, num_form_chars, 
                  num_tags, tag_bos, tag_eow, bert_dim, predict_only):
        with self.session.graph.as_default():

            # Inputs
            self.sentence_lens = tf.placeholder(tf.int32, [None], name="sentence_lens")
            self.form_ids = tf.placeholder(tf.int32, [None, None], name="form_ids")
            self.bert_wes = tf.placeholder(tf.float32, [None, None, bert_dim], name="bert_wes")
            self.tags = tf.placeholder(tf.int32, [None, None], name="tags")
            self.is_training = tf.placeholder(tf.bool, [])
            self.learning_rate = tf.placeholder(tf.float32, [])

            if args.including_charseqs:
                self.form_charseqs = tf.placeholder(tf.int32, [None, None], name="form_charseqs")
                self.form_charseq_lens = tf.placeholder(tf.int32, [None], name="form_charseq_lens")
                self.form_charseq_ids = tf.placeholder(tf.int32, [None,None], name="form_charseq_ids")
                
            # RNN Cell
            if args.rnn_cell == "LSTM":
                rnn_cell = tf.nn.rnn_cell.BasicLSTMCell
            elif args.rnn_cell == "GRU":
                rnn_cell = tf.nn.rnn_cell.GRUCell
            else:
                raise ValueError("Unknown rnn_cell {}".format(args.rnn_cell))

            inputs = []

            # Trainable embeddings for forms
            form_embeddings = tf.get_variable("form_embeddings", shape=[num_forms, args.we_dim], dtype=tf.float32)
            inputs.append(tf.nn.embedding_lookup(form_embeddings, self.form_ids))
            
            # BERT
            if bert_dim:
                inputs.append(self.bert_wes)

            # Character-level form embeddings
            if args.including_charseqs:

                # Generate character embeddings for num_form_chars of dimensionality args.cle_dim.
                character_embeddings = tf.get_variable("form_character_embeddings",
                                                        shape=[num_form_chars, args.cle_dim],
                                                        dtype=tf.float32)
                
                # Embed self.form_charseqs (list of unique form in the batch) using the character embeddings.
                characters_embedded = tf.nn.embedding_lookup(character_embeddings, self.form_charseqs)
                
                # Use tf.nn.bidirectional.rnn to process embedded self.form_charseqs
                # using a GRU cell of dimensionality args.cle_dim.
                _, (state_fwd, state_bwd) = tf.nn.bidirectional_dynamic_rnn(
                        tf.nn.rnn_cell.GRUCell(args.cle_dim), tf.nn.rnn_cell.GRUCell(args.cle_dim),
                        characters_embedded, sequence_length=self.form_charseq_lens, dtype=tf.float32, scope="form_cle")
                
                # Sum the resulting fwd and bwd state to generate character-level form embedding (CLE)
                # of unique forms in the batch.
                cle = tf.concat([state_fwd, state_bwd], axis=1)
                
                # Generate CLEs of all form in the batch by indexing the just computed embeddings
                # by self.form_charseq_ids (using tf.nn.embedding_lookup).
                cle_embedded = tf.nn.embedding_lookup(cle, self.form_charseq_ids)
                
                # Concatenate the form embeddings (computed above in inputs) and the CLE (in this order).
                inputs.append(cle_embedded)

            # Concatenate inputs
            inputs = tf.concat(inputs, axis=2)
            
            # Dropout
            inputs_dropout = tf.layers.dropout(inputs, rate=args.dropout, training=self.is_training)
            
            # Computation
            hidden_layer_dropout = inputs_dropout # first layer is input
            for i in range(args.rnn_layers):
                (hidden_layer_fwd, hidden_layer_bwd), _ = tf.nn.bidirectional_dynamic_rnn(
                    rnn_cell(args.rnn_cell_dim), rnn_cell(args.rnn_cell_dim),
                    hidden_layer_dropout, sequence_length=self.sentence_lens, dtype=tf.float32,
                    scope="RNN-{}".format(i))
                hidden_layer = tf.concat([hidden_layer_fwd, hidden_layer_bwd], axis=2)
                if i == 0: hidden_layer_dropout = 0
                hidden_layer_dropout += tf.layers.dropout(hidden_layer, rate=args.dropout, training=self.is_training)

            # Decoders
            if args.decoding == "CRF": # conditional random fields
                output_layer = tf.layers.dense(hidden_layer_dropout, num_tags)
                weights = tf.sequence_mask(self.sentence_lens, dtype=tf.float32)
                log_likelihood, transition_params = tf.contrib.crf.crf_log_likelihood(
                    output_layer, self.tags, self.sentence_lens)
                loss = tf.reduce_mean(-log_likelihood)
                self.predictions, viterbi_score = tf.contrib.crf.crf_decode(
                    output_layer, transition_params, self.sentence_lens)
                self.predictions_training = self.predictions
            elif args.decoding == "ME": # vanilla maximum entropy
                output_layer = tf.layers.dense(hidden_layer_dropout, num_tags)
                weights = tf.sequence_mask(self.sentence_lens, dtype=tf.float32)
                if args.label_smoothing:
                    gold_labels = tf.one_hot(self.tags, num_tags) * (1 - args.label_smoothing) + args.label_smoothing / num_tags
                    loss = tf.losses.softmax_cross_entropy(gold_labels, output_layer, weights=weights)
                else:
                    loss = tf.losses.sparse_softmax_cross_entropy(self.tags, output_layer, weights=weights)
                self.predictions = tf.argmax(output_layer, axis=2)
                self.predictions_training = self.predictions
            elif args.decoding in ["LSTM", "seq2seq"]: # Decoder
                # Generate target embeddings for target chars, of shape [target_chars, args.char_dim].
                tag_embeddings = tf.get_variable("tag_embeddings", shape=[num_tags, args.we_dim], dtype=tf.float32)

                # Embed the target_seqs using the target embeddings. 
                tags_embedded = tf.nn.embedding_lookup(tag_embeddings, self.tags)

                decoder_rnn_cell = rnn_cell(args.rnn_cell_dim)

                # Create a `decoder_layer` -- a fully connected layer with
                # target_chars neurons used in the decoder to classify into target characters.
                decoder_layer = tf.layers.Dense(num_tags)
                
                sentence_lens = self.sentence_lens
                max_sentence_len = tf.reduce_max(sentence_lens)
                tags = self.tags
                # The DecoderTraining will be used during training. It will output logits for each
                # target character.
                class DecoderTraining(tf.contrib.seq2seq.Decoder):
                    @property
                    def batch_size(self): return tf.shape(hidden_layer_dropout)[0]
                    @property
                    def output_dtype(self): return tf.float32 # Type for logits of target characters
                    @property
                    def output_size(self): return num_tags # Length of logits for every output
                    @property
                    def tag_eow(self): return tag_eow

                    def initialize(self, name=None):
                        states = decoder_rnn_cell.zero_state(self.batch_size, tf.float32)
                        inputs = [tf.nn.embedding_lookup(tag_embeddings, tf.fill([self.batch_size], tag_bos)), hidden_layer_dropout[:,0]]
                        inputs = tf.concat(inputs, axis=1)
                        if args.decoding == "seq2seq":
                            predicted_eows = tf.zeros([self.batch_size], dtype=tf.int32)
                            inputs = (inputs, predicted_eows)
                        finished = sentence_lens <= 0
                        return finished, inputs, states

                    def step(self, time, inputs, states, name=None):
                        if args.decoding == "seq2seq":
                            inputs, predicted_eows = inputs
                        outputs, states = decoder_rnn_cell(inputs, states)
                        outputs = decoder_layer(outputs)
                        next_input = [tf.nn.embedding_lookup(tag_embeddings, tags[:,time])]
                        if args.decoding == "seq2seq":
                            predicted_eows += tf.to_int32(tf.equal(tags[:, time], self.tag_eow))
                            indices = tf.where(tf.one_hot(tf.minimum(predicted_eows, max_sentence_len - 1), tf.reduce_max(predicted_eows) + 1))
                            next_input.append(tf.gather_nd(hidden_layer_dropout, indices))
                        else:
                            next_input.append(hidden_layer_dropout[:,tf.minimum(time + 1, max_sentence_len - 1)])
                        next_input = tf.concat(next_input, axis=1)
                        if args.decoding == "seq2seq":
                            next_input = (next_input, predicted_eows)
                            finished = sentence_lens <= predicted_eows
                        else:
                            finished = sentence_lens <= time + 1
                        return outputs, states, next_input, finished
                output_layer, _, prediction_training_lens = tf.contrib.seq2seq.dynamic_decode(DecoderTraining())
                self.predictions_training = tf.argmax(output_layer, axis=2, output_type=tf.int32)
                weights = tf.sequence_mask(prediction_training_lens, dtype=tf.float32)
                if args.label_smoothing:
                    gold_labels = tf.one_hot(self.tags, num_tags) * (1 - args.label_smoothing) + args.label_smoothing / num_tags
                    loss = tf.losses.softmax_cross_entropy(gold_labels, output_layer, weights=weights)
                else:
                    loss = tf.losses.sparse_softmax_cross_entropy(self.tags, output_layer, weights=weights)

                # The DecoderPrediction will be used during prediction. It will
                # directly output the predicted target characters.
                class DecoderPrediction(tf.contrib.seq2seq.Decoder):
                    @property
                    def batch_size(self): return tf.shape(hidden_layer_dropout)[0]
                    @property
                    def output_dtype(self): return tf.int32 # Type for predicted target characters
                    @property
                    def output_size(self): return 1 # Will return just one output
                    @property
                    def tag_eow(self): return tag_eow

                    def initialize(self, name=None):
                        states = decoder_rnn_cell.zero_state(self.batch_size, tf.float32)
                        inputs = [tf.nn.embedding_lookup(tag_embeddings, tf.fill([self.batch_size], tag_bos)), hidden_layer_dropout[:,0]]
                        inputs = tf.concat(inputs, axis=1)
                        if args.decoding == "seq2seq":
                            predicted_eows = tf.zeros([self.batch_size], dtype=tf.int32)
                            labels_per_tokens = tf.zeros([self.batch_size], dtype=tf.int32)
                            inputs = (inputs, predicted_eows, labels_per_tokens) 
                        finished = sentence_lens <= 0
                        return finished, inputs, states
                    
                    def step(self, time, inputs, states, name=None):
                        if args.decoding == "seq2seq":
                            inputs, predicted_eows, labels_per_tokens = inputs
                        outputs, states = decoder_rnn_cell(inputs, states)
                        outputs = decoder_layer(outputs)
                        outputs = tf.argmax(outputs, axis=1, output_type=self.output_dtype)

                        if args.decoding == "seq2seq":
                            # Force eow in tokens with too many generated labels
                            outputs = tf.where(labels_per_tokens >= args.max_labels_per_token, tf.fill([self.batch_size], self.tag_eow), outputs)

                            # Update counts of labels and predicted eows
                            outputs_are_eow = tf.cast(tf.equal(outputs, self.tag_eow), tf.int32)
                            labels_per_tokens = (labels_per_tokens + 1) * (1 - outputs_are_eow)
                            predicted_eows += outputs_are_eow

                        next_input = [tf.nn.embedding_lookup(tag_embeddings, outputs)]

                        if args.decoding == "seq2seq":
                            indices = tf.where(tf.one_hot(tf.minimum(predicted_eows, max_sentence_len - 1), tf.reduce_max(predicted_eows) + 1))
                            next_input.append(tf.gather_nd(hidden_layer_dropout, indices))
                        else:
                            next_input.append(hidden_layer_dropout[:,tf.minimum(time + 1, max_sentence_len - 1)])
                        next_input = tf.concat(next_input, axis=1)
                        if args.decoding == "seq2seq":
                            next_input = (next_input, predicted_eows, labels_per_tokens)
                            finished = sentence_lens <= predicted_eows
                        else:
                            finished = sentence_lens <= time + 1
                        return outputs, states, next_input, finished
                self.predictions, _, _ = tf.contrib.seq2seq.dynamic_decode(
                        DecoderPrediction(), maximum_iterations=3*tf.reduce_max(self.sentence_lens) + 10)
                
            # Saver
            self.saver = tf.train.Saver(max_to_keep=1)
            if predict_only: return

            # Training
            global_step = tf.train.create_global_step()
            self.training = tf.contrib.opt.LazyAdamOptimizer(learning_rate=self.learning_rate, beta2=args.beta_2).minimize(loss, global_step=global_step)

            # Summaries
            self.current_accuracy, self.update_accuracy = tf.metrics.accuracy(self.tags, self.predictions_training, weights=weights)
            self.current_loss, self.update_loss = tf.metrics.mean(loss, weights=tf.reduce_sum(weights))
            self.reset_metrics = tf.variables_initializer(tf.get_collection(tf.GraphKeys.METRIC_VARIABLES))

            summary_writer = tf.contrib.summary.create_file_writer(args.logdir, flush_millis=10 * 1000)
            self.summaries = {}
            with summary_writer.as_default(), tf.contrib.summary.record_summaries_every_n_global_steps(100):
                self.summaries["train"] = [tf.contrib.summary.scalar("train/loss", self.update_loss),
                                           tf.contrib.summary.scalar("train/accuracy", self.update_accuracy)]
            with summary_writer.as_default(), tf.contrib.summary.always_record_summaries():
                for dataset in ["dev", "test"]:
                    self.summaries[dataset] = [tf.contrib.summary.scalar(dataset + "/loss", self.current_loss),
                                               tf.contrib.summary.scalar(dataset + "/accuracy", self.current_accuracy)]

            self.metrics = {}
            self.metrics_summarize = {}
            for metric in ["precision", "recall", "F1"]:
                self.metrics[metric] = tf.placeholder(tf.float32, [], name=metric)
                self.metrics_summarize[metric] = {}
                with summary_writer.as_default(), tf.contrib.summary.always_record_summaries():
                    for dataset in ["dev", "test"]:
                        self.metrics_summarize[metric][dataset] = tf.contrib.summary.scalar(dataset + "/" + metric,
                                                                                            self.metrics[metric])

            # Initialize variables
            self.session.run(tf.global_variables_initializer())
            with summary_writer.as_default():
                tf.contrib.summary.initialize(session=self.session, graph=self.session.graph)


    def train_epoch(self, train, learning_rate, args):
        while not train.epoch_finished():
            seq2seq = args.decoding == "seq2seq"
            batch_dict = train.next_batch(args.batch_size, including_charseqs=args.including_charseqs, seq2seq=seq2seq)
            if args.word_dropout:
                mask = np.random.binomial(n=1, p=args.word_dropout, size=batch_dict["word_ids"][train.FORMS].shape)
                batch_dict["word_ids"][train.FORMS] = (1 - mask) * batch_dict["word_ids"][train.FORMS] + mask * train.factors[train.FORMS].words_map["<unk>"]
                
            self.session.run(self.reset_metrics)
            feeds = {self.sentence_lens: batch_dict["sentence_lens"],
                     self.form_ids: batch_dict["word_ids"][train.FORMS],
                     self.tags: batch_dict["word_ids"][train.TAGS],
                     self.is_training: True,
                     self.learning_rate: learning_rate}
            if args.bert_train: # BERT
                feeds[self.bert_wes] = batch_dict["batch_bert_wes"]
            if args.including_charseqs: # character-level embeddings
                feeds[self.form_charseqs] = batch_dict["batch_charseqs"][train.FORMS]
                feeds[self.form_charseq_lens] = batch_dict["batch_charseq_lens"][train.FORMS]
                feeds[self.form_charseq_ids] = batch_dict["batch_charseq_ids"][train.FORMS]

            self.session.run([self.training, self.summaries["train"]], feeds)


    def evaluate(self, dataset_name, dataset, args):
        with open("{}/{}_system_predictions.conll".format(args.logdir, dataset_name), "w", encoding="utf-8") as prediction_file:
            self.predict(dataset_name, dataset, args, prediction_file, evaluating=True)

        f1 = 0.0

#       # The training graph only computes training and development accuracy.
#       # To get a better idea about the direction of training, NER F1 score
#       # can be computed for development data after each epoch by calling and
#       # external commandline evaluation script and then parsing its output.
#
#        if args.corpus in ["english-conll", "german-conll", "dutch-conll", "spanish-conll"]:
#            os.system("cd {} && ../../run_conlleval.sh {} {} {}_system_predictions.conll".format(args.logdir, dataset_name, args.__dict__[dataset_name + "_data"], dataset_name))
#
#           with open("{}/{}.eval".format(args.logdir,dataset_name), "r", encoding="utf-8") as result_file:
#                for line in result_file:
#                    line = line.strip("\n")
#                    if line.startswith("accuracy:"):
#                        f1 = float(line.split()[-1])
#                        self.session.run(self.metrics_summarize["F1"][dataset_name], {self.metrics["F1"]: f1})
#
#        elif args.corpus in ["czech-cnec2.0"]:
#            os.system("cd {} && ../../run_cnec2.0_eval_nested.sh {} {} {}_system_predictions.conll".format(args.logdir, dataset_name, args.__dict__[dataset_name + "_data"], dataset_name))
#
#            with open("{}/{}.eval".format(args.logdir,dataset_name), "r", encoding="utf-8") as result_file:
#                for line in result_file:
#                    line = line.strip("\n")
#                    if line.startswith("Type:"):
#                        cols = line.split()
#                        precision, recall, f1 = map(float, [cols[1], cols[3], cols[5]])
#                        for metric, value in [["precision", precision], ["recall", recall], ["F1", f1]]:
#                            self.session.run(self.metrics_summarize[metric][dataset_name], {self.metrics[metric]: value})

        return f1


    def predict(self, dataset_name, dataset, args, prediction_file, evaluating=False):
        """Predicts labels for NameTag2Dataset (see nametag2_dataset.py).
        No sanity check of the neural network output is done, which means:
        1. Neither correct nesting of the entities, nor correct entity openings
        and closing (correct bracketing) are guaranteed.
        2. Labels and their encoding (BIO vs. IOB) is the exact same as in the
        model trained from and underlying corpus (i.e., IOB found in English
        CoNLL-2003 dataset).
        Please see Network.postprocess for correct bracketing and
        BIO formatting of the output."""

        if evaluating:
            self.session.run(self.reset_metrics)
        tags = []
        first_batch=True
        while not dataset.epoch_finished():
            seq2seq = args.decoding == "seq2seq"
            batch_dict = dataset.next_batch(args.batch_size, including_charseqs=args.including_charseqs, seq2seq=seq2seq)
            targets = [self.predictions]
            feeds = {self.sentence_lens: batch_dict["sentence_lens"],
                    self.form_ids: batch_dict["word_ids"][dataset.FORMS],
                    self.is_training: False}
            if evaluating:
                targets.extend([self.update_accuracy, self.update_loss])
                feeds[self.tags] = batch_dict["word_ids"][dataset.TAGS]
            if args.bert or args.bert_dev or args.bert_test: # BERT
                feeds[self.bert_wes] = batch_dict["batch_bert_wes"]
            if args.including_charseqs: # character-level embeddings
                feeds[self.form_charseqs] = batch_dict["batch_charseqs"][dataset.FORMS]
                feeds[self.form_charseq_lens] = batch_dict["batch_charseq_lens"][dataset.FORMS]
                feeds[self.form_charseq_ids] = batch_dict["batch_charseq_ids"][dataset.FORMS]
            tags.extend(self.session.run(targets, feeds)[0])

            # Start measuring time after first batch
            if args.time and first_batch:
                start = time.time()
            first_batch=False
        # End measuring time after all batches
        if args.time:
            end = time.time()
            print("Time elapsed: {}".format(end - start), file=sys.stderr, flush=True)

        if evaluating:
            self.session.run([self.current_accuracy, self.summaries[dataset_name]])
     
        forms = dataset.factors[dataset.FORMS].strings
        for s in range(len(forms)):
            j = 0
            for i in range(len(forms[s])):
                if args.decoding == "seq2seq": # collect all tags until <eow>
                    labels = []
                    while j < len(tags[s]) and dataset.factors[dataset.TAGS].words[tags[s][j]] != "<eow>":
                        labels.append(dataset.factors[dataset.TAGS].words[tags[s][j]])
                        j += 1
                    j += 1 # skip the "<eow>"
                    print("{}\t{}".format(forms[s][i], "|".join(labels)), file=prediction_file)
                else:
                    print("{}\t{}".format(forms[s][i], dataset.factors[dataset.TAGS].words[tags[s][i]]), file=prediction_file)
            print("", file=prediction_file)

    def postprocess(self, text):
        
        # Create a set of correctly bracketed, unique entities
        
        forms, previous_labels, starts = [], [], []
        entities = dict()   # (start, end, label)

        for i, line in enumerate(text.split("\n")):
            if not line:    # end of sentence
                forms.append("")
                for j in range(len(previous_labels)): # close entities
                    entities[(starts[j], i, previous_labels[j][2:])] = j
                previous_labels, starts = [], []
            else:
                form, ne = line.split("\t")
                if ne == "O":   # all entities ended
                    forms.append(form)
                    for j in range(len(previous_labels)): # close entities
                        entities[(starts[j], i, previous_labels[j][2:])] = j
                    previous_labels, starts = [], []
                else:
                    labels = ne.split("|")
                    for j in range(len(labels)):
                        if labels[j] == "O": # bad decoder output, "O" should be alone
                            labels = labels[:j]
                            break
                        if j < len(previous_labels):
                            if labels[j].startswith("B-") or previous_labels[j][2:] != labels[j][2:]:
                                # Previous entity was ended by current starting
                                # entity, forcing end of all its nested
                                # entities (following in the previous list):
                                for k in range(j, len(previous_labels)): # close entities
                                    entities[(starts[k], i, previous_labels[k][2:])] = k
                                previous_labels = previous_labels[:j]
                                starts = starts[:j]
                                starts.append(i)
                        else: # new entity starts here
                            starts.append(i)
                    forms.append(form)
                    if len(labels) < len(previous_labels):  # close entities
                        for j in range(len(labels), len(previous_labels)):
                            entities[(starts[j], i, previous_labels[j][2:])] = j
                    previous_labels = labels
                    starts = starts[:len(labels)]
        
        # Sort entities
        entities = sorted(entities.items(), key=lambda x: (x[0][0], -x[0][1], x[1]))

        # Reconstruct the CoNLL output with the entities set,
        # removing duplicates and changing IOB -> BIO
        labels = [ [] for _ in range(len(forms)) ]
        for (start, end, label), _ in entities:
            for i in range(start, end):
                labels[i].append(("B-" if i == start else "I-") + label)

        output = []
        for form, label in zip(forms, labels):
            if form:
                output.append("{}\t{}\n".format(form, "|".join(label) if label else "O"))
            else:
                output.append("\n")
       
        if output and output[-1] == "\n":
            output.pop()

        return "".join(output)
