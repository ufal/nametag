#!/usr/bin/env python3
# coding=utf-8
#
# Copyright 2021 Institute of Formal and Applied Linguistics, Faculty of
# Mathematics and Physics, Charles University, Czech Republic.
#
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http://mozilla.org/MPL/2.0/.

"""Nested NER training and prediction script.

Before running this script, please see README on instructions how to setup
your Python virtual environment with TensorFlow and where to get the models
and BERT WEmbeddings server.

Using NameTag 2 for NER prediction
----------------------------------

Before you run NameTag 2 for NER prediction, please make sure you have:

  1. installed Python virtual environment,
  2. downloaded the NameTag 2 models,
  3. you have an ip:port of a running BERT WEmbeddings service (e.g.,
     localhost:8000).

NER prediction example usage:

$ venv-tf-1.12-cpu/bin/python ./nametag2.py --test_data=examples/en_input.conll --predict=models/en_CoNLL_en --bert=localhost:8000

test_data: Input file. The expected format is a segmented and tokenized vertical
           format: one token per line, sentences delimited with newline.
predict: Path to model directory.
bert: ip:port of a running WEmbeddings server.

Input example (also please see examples/en_input.conll):

John
loves
Mary
.

Mary
loves
John.

Output and gold example (also please see examples/en_gold.conll):

John  B-PER
loves O
Mary  B-PER
.

Mary  B-PER
loves O
John  B-PER
.     O

Training NameTag 2
------------------

The main NameTag 2 script nametag2.py can be used for training a custom corpus.
It will do so when provided the parameters --train_data. Optionally, --dev_data
can be provided and many other training parameters that control the training.

The input data file format is a vertical file, one token and its label per
line, separated by a tabulator; sentences delimited by newlines (such as
a first and fourth column in a well-known CoNLL-2003 IOB shared task corpus).
An example of such input file can be found in examples/en_gold.conll.

Example usage:

venv-tf-1.12-cpu/bin/python3 nametag2.py --threads=4 --batch_size=4 --train_data=train.conll --dev_data=dev.conll --epochs=10:1e-3,8:1e-4 --save_checkpoint=my_models --bert_train=train_bert.npz --bert_dev=dev_bert.npz

While it is technically possible to compute the BERT embeddings on demand for
each shuffled batch in each step, we strongly recommend to precompute the BERT
embeddings as numpy arrays, one array per each token in your vertical
CoNLL-like file and store these as .npz files. The argument --bert_train and
--bert_dev will load the precomputed BERT embeddings from the npz file and
reuse them during training, speeding up the training process.
"""


import io
import json
import pickle
import sys

import numpy as np

import nametag2_dataset
import nametag2_network


if __name__ == "__main__":
    import argparse
    import datetime
    import os
    import re

    # Fix random seed for network initialization before training
    np.random.seed(42)

    # Parse arguments
    parser = argparse.ArgumentParser()
    parser.add_argument("--batch_size", default=64, type=int, help="Batch size.")

    # Computes BERT embeddings on demand by requesting WEmbeddings server at ip:port.
    parser.add_argument("--bert", default=None, type=str, nargs="?", const="", help="Get BERT embeddings by requesting ip:port.")

    # For training, precompute BERT embeddings to speed things up.
    parser.add_argument("--bert_dev", default=None, type=str, help="Pretrained BERT embeddings for dev data in file.")
    parser.add_argument("--bert_test", default=None, type=str, help="Pretrained BERT embeddings for test data in file.")
    parser.add_argument("--bert_train", default=None, type=str, help="Pretrained BERT embeddings for train data in file.")

    parser.add_argument("--beta_2", default=0.98, type=float, help="Beta 2.")
    parser.add_argument("--corpus", default="", type=str, help="[english-conll|german-conll|dutch-conll|spanish-conll|czech-cnec2.0]")
    parser.add_argument("--cle_dim", default=128, type=int, help="Character-level embedding dimension.")
    parser.add_argument("--decoding", default="seq2seq", type=str, help="Decoding: [CRF|ME|LSTM|seq2seq].")
    parser.add_argument("--dev_data", default=None, type=str, help="Dev data.")
    parser.add_argument("--dropout", default=0.5, type=float, help="Dropout rate.")

    # E.g.: 10:1e-3,8:1e-4
    parser.add_argument("--epochs", default="10:1e-3", type=str, help="Epochs and learning rates.")

    parser.add_argument("--label_smoothing", default=0, type=float, help="Label smoothing.")
    parser.add_argument("--max_sentences", default=None, type=int, help="Limit number of training sentences (for debugging).")
    parser.add_argument("--max_labels_per_token", default=5, type=int, help="Maximum labels per token.")
    parser.add_argument("--name", default=None, type=str, help="Experiment name.")
    parser.add_argument("--predict", default=None, type=str, help="Predict using the saved checkpoint.")
    parser.add_argument("--rnn_cell", default="LSTM", type=str, help="RNN cell type.")
    parser.add_argument("--rnn_cell_dim", default=256, type=int, help="RNN cell dimension.")
    parser.add_argument("--rnn_layers", default=1, type=int, help="Number of hidden layers.")
    parser.add_argument("--save_checkpoint", default=None, type=str, help="Save checkpoint to path.")
    parser.add_argument("--test_data", default=None, type=str, help="Test data.")
    parser.add_argument("--time", default=False, action="store_true", help="Measure prediction time.")
    parser.add_argument("--train_data", default=None, type=str, help="Training data.")
    parser.add_argument("--threads", default=4, type=int, help="Maximum number of threads to use.")
    parser.add_argument("--we_dim", default=256, type=int, help="Word embedding dimension.")
    parser.add_argument("--word_dropout", default=0.2, type=float, help="Word dropout.")
    args = parser.parse_args()

    if args.predict:
        # Load saved options from the model
        with open("{}/options.json".format(args.predict), mode="r") as options_file:
            args = argparse.Namespace(**json.load(options_file))
        parser.parse_args(namespace=args)
    else:
        # Create TensorFlow logdir name
        logargs = dict(vars(args).items())
        del logargs["bert_dev"]
        del logargs["bert_test"]
        del logargs["bert_train"]
        del logargs["beta_2"]
        del logargs["cle_dim"]
        del logargs["dev_data"]
        del logargs["dropout"]
        del logargs["label_smoothing"]
        del logargs["max_sentences"]
        del logargs["rnn_cell_dim"]
        del logargs["test_data"]
        del logargs["threads"]
        del logargs["train_data"]
        del logargs["we_dim"]
        del logargs["word_dropout"]
        logargs["bert"] = 1 if args.bert_train else 0

        args.logdir = "logs/{}-{}-{}".format(
            os.path.basename(__file__),
            datetime.datetime.now().strftime("%Y-%m-%d_%H%M%S"),
            ",".join(("{}={}".format(re.sub("(.)[^_]*_?", r"\1", key), re.sub("^.*/", "", value) if type(value) == str else value)
                      for key, value in sorted(logargs.items())))
        )
        os.makedirs(args.logdir, exist_ok=True)

        # Check if directory exists if saving checkpoint
        if args.save_checkpoint:
            if os.path.isdir(args.save_checkpoint):
                print("Checkpoint will be saved to existing directory: {}".format(args.save_checkpoint), file=sys.stderr, flush=True)
            else:
                print("Path to checkpoint does not exist, creating directory {}".format(args.save_checkpoint), file=sys.stderr, flush=True)
                os.mkdir(args.save_checkpoint)

        # Dump passed options to allow future prediction.
        if args.save_checkpoint:
            with open("{}/options.json".format(args.save_checkpoint), mode="w") as options_file:
                json.dump(vars(args), options_file, sort_keys=True)

    # Postprocess args
    args.epochs = [(int(epochs), float(lr)) for epochs, lr in (epochs_lr.split(":") for epochs_lr in args.epochs.split(","))]

    # Print debug outputs
    print("Number of threads: {}".format(args.threads), file=sys.stderr, flush=True)
    print("Batch size: {}".format(args.batch_size), file=sys.stderr, flush=True)

    # Setup BERT embeddings server in subprocess if empty --bert given
    # (otherwise read from file or use --bert=ip:port for requests).
    if args.bert is not None and not args.bert: # --bert
        print("Please provide ip:port WEmbeddings server for --bert. Empty --bert is not supported.", file=sys.stderr, flush=True)
        sys.exit(1)

    # Load the data
    print("Loading data...", file=sys.stderr, flush=True)
    seq2seq = args.decoding == "seq2seq"
    if not args.predict:
        train = nametag2_dataset.NameTag2Dataset(filename=args.train_data,
                                                 max_sentences=args.max_sentences,
                                                 seq2seq=seq2seq,
                                                 bert_filename=args.bert_train if not args.predict else None,
                                                 bert=args.bert)
        train.save_mappings("{}/mappings.pickle".format(args.save_checkpoint))
        if args.dev_data:
            dev = nametag2_dataset.NameTag2Dataset(filename=args.dev_data,
                                                   train=train,
                                                   shuffle_batches=False,
                                                   seq2seq=seq2seq,
                                                   bert_filename=args.bert_dev,
                                                   bert=args.bert)
    else:
        with open("{}/mappings.pickle".format(args.predict), mode="rb") as mappings_file:
            train = pickle.load(mappings_file)

    if args.test_data:
        test = nametag2_dataset.NameTag2Dataset(filename=args.test_data,
                                                train=train,
                                                shuffle_batches=False,
                                                seq2seq=seq2seq,
                                                bert_filename=args.bert_test,
                                                bert=args.bert)

    # Character-level embeddings
    args.including_charseqs = (args.cle_dim > 0)

    # Construct the network
    network = nametag2_network.Network(threads=args.threads)
    network.construct(args,
                      num_forms=len(train.factors[train.FORMS].words),
                      num_form_chars=len(train.factors[train.FORMS].alphabet),
                      num_tags=len(train.factors[train.TAGS].words),
                      tag_bos=train.factors[train.TAGS].words_map["<bos>"],
                      tag_eow=train.factors[train.TAGS].words_map["<eow>"],
                      bert_dim=train.bert_dim(),
                      predict_only=args.predict)

    # Predict only
    if args.predict:
        network.saver.restore(network.session, "{}/model".format(args.predict.rstrip("/")))
        print("Predicting test data using previously saved checkpoint.", file=sys.stderr)
        output = io.StringIO()
        network.predict("test", test, args, output, evaluating=False)
        output = output.getvalue()
        output = network.postprocess(output)
        sys.stdout.write(output)

    # Train and predict
    else:
        # Train
        for epochs, learning_rate in args.epochs:
            for epoch in range(epochs):
                print("Epoch: {}, learning rate: {}".format(epoch, learning_rate), file=sys.stderr, flush=True)
                network.train_epoch(train, learning_rate, args)
                if args.dev_data:
                    dev_score = network.evaluate("dev", dev, args)
                    print("Dev score: {}".format(dev_score), file=sys.stderr, flush=True)
        # Save network
        if args.save_checkpoint:
            network.saver.save(network.session, "{}/model".format(args.save_checkpoint), write_meta_graph=False)
        # Test
        if args.test_data:
            test_score = network.evaluate("test", test, args)
            print("Test score: {}".format(test_score), file=sys.stderr, flush=True)
