#!/usr/bin/env python3

# Copyright 2021 Institute of Formal and Applied Linguistics, Faculty of
# Mathematics and Physics, Charles University in Prague, Czech Republic.
#
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http://mozilla.org/MPL/2.0/.

"""NameTag 2 server.

Before running this script, please see README on instructions how to setup your
Python virtual environment with TensorFlow and where to get the models and BERT
WEmbeddings server.

Starting NameTag 2 server
-------------------------

The mandatory arguments are given in this order:

- port
- WEmbeddings server (host:port)
- default model name
- each following triple of arguments defines a model, of which
    - first argument is the model name
    - second argument is the model directory
    - third argument are the acknowledgemets to append

Example server usage:

$ venv/bin/python nametag2_server.py 8001 localhost:8000 czech-cnec2.0-200831 czech-cnec2.0-200831 models/czech-cnec2.0-200831/ ack-text

Sending requests to the NameTag 2 server
----------------------------------------

Because the models are loaded on demand with their first usage, the very first
request to the server for a particular model takes somewhat longer time.

Example commandline call with curl:

$ curl -F data=@examples/cs_input.conll -F input="vertical" -F output="conll" localhost:8001/recognize| jq -j .result

Expected commandline output:

Jmenuji O
se      O
Jan     B-P|B-pf
Novák  I-P|B-ps
.       O

"""

import argparse
import email.parser
import http.server
import itertools
import io
import json
import os
import pickle
import socketserver
import sys
import threading
import time
import urllib.parse

import nametag2_dataset
import nametag2_network
import ufal.udpipe
import wembedding_service.wembeddings.wembeddings as wembeddings


SEP = "\t"


class UDPipeTokenizer:
    class Token:
        def __init__(self, token, spaces_before, spaces_after):
            self.token = token
            self.spaces_before = spaces_before
            self.spaces_after = spaces_after

    def __init__(self, path):
        self._model = ufal.udpipe.Model.load(path)
        if self._model is None:
            raise RuntimeError("Cannot load tokenizer from {}".format(path))

    def tokenize(self, text, mode="untokenized"):
        if mode == "untokenized":
            tokenizer = self._model.newTokenizer(self._model.DEFAULT)
        elif mode == "vertical":
            tokenizer = ufal.udpipe.InputFormat.newVerticalInputFormat()
        else:
            raise ValueError("Unknown tokenizer mode '{}'".format(model))
        if tokenizer is None:
            raise RuntimeError("Cannot create the tokenizer")

        sentence = ufal.udpipe.Sentence()
        processing_error = ufal.udpipe.ProcessingError()
        tokenizer.setText(text)
        while tokenizer.nextSentence(sentence, processing_error):
            result, word, multiword_token = [], 1, 0
            while word < len(sentence.words):
                if multiword_token < len(sentence.multiwordTokens) and sentence.multiwordTokens[multiword_token].idFirst == word:
                    token = sentence.multiwordTokens[multiword_token]
                    word = sentence.multiwordTokens[multiword_token].idLast + 1
                    multiword_token += 1
                else:
                    token = sentence.words[word]
                    word += 1
                result.append(self.Token(token.form, token.getSpacesBefore(), token.getSpacesAfter()))
            yield result
        if processing_error.occurred():
            raise RuntimeError("Cannot read input data: '{}'".format(processing_error.message))


class Models:
    class Model:
        def __init__(self, path, name, acknowledgements, server_args):
            """Initializes NameTag 2 models and UDPipe tokenizer models."""
            self._server_args = server_args
            self.name = name
            self.acknowledgements = acknowledgements

            self.network, self.args, self.train = None, None, None
             
            # Load saved options from the model
            with open("{}/options.json".format(path), mode="r") as options_file:
                self.args = argparse.Namespace(**json.load(options_file))

            # Postprocess args
            self.args.including_charseqs = (self.args.cle_dim > 0)

            if "max_labels_per_token" not in self.args:
                self.args.max_labels_per_token = server_args.max_labels_per_token
 
            # Unpickle word mappings of train data
            with open("{}/mappings.pickle".format(path), mode="rb") as mappings_file:
                self.train = pickle.load(mappings_file)

            # Construct the network
            self.network = nametag2_network.Network(threads=server_args.threads)
            self.network.construct(self.args,
                                   num_forms=len(self.train.factors[self.train.FORMS].words),
                                   num_form_chars=len(self.train.factors[self.train.FORMS].alphabet),
                                   num_tags=len(self.train.factors[self.train.TAGS].words),
                                   tag_bos=self.train.factors[self.train.TAGS].words_map["<bos>"],
                                   tag_eow=self.train.factors[self.train.TAGS].words_map["<eow>"],
                                   bert_dim=self.train.bert_dim(),
                                   predict_only=True)
 
            # Load the checkpoint
            self.network.saver.restore(self.network.session, "{}/model".format(path.rstrip("/")))
            print("Loaded model {}".format(os.path.basename(path)), file=sys.stderr, flush=True)
            
            # Load the tokenizer
            tokenizer_path = os.path.join(path, "udpipe.tokenizer")
            self._tokenizer = UDPipeTokenizer(tokenizer_path)
            if self._tokenizer is None:
                raise RuntimeError("Cannot load tokenizer from {}".format(tokenizer_path))


        def predict(self, text):
            time_start = time.time()
                
            # Create NameTag2Dataset
            dataset = nametag2_dataset.NameTag2Dataset(text=text,
                                                       train=self.train,
                                                       shuffle_batches=False,
                                                       seq2seq=True,
                                                       bert=self._server_args.wembedding_server)

            # Predict
            output = io.StringIO()
            self.network.predict("test", dataset, self.args, output, evaluating=False)

            time_end = time.time()
            print("Request {:.2f}ms,".format(1000 * (time_end - time_start)), file=sys.stderr, flush=True)

            return output.getvalue()


        def postprocess(self, text):
            return self.network.postprocess(text)


        def conll_to_vertical(self, text, sentences, n_tokens_in_batch):
            output = []
            open_ids, open_forms, open_labels = [], [], []  # open entities on i-th line
            
            # indexes to tokenizer's sentences
            n_sentences = 0
            n_tokens = 0
            in_sentence = False

            for i, line in enumerate(text.split("\n")):
                if not line:                                # end of sentence
                    if in_sentence:
                        for j in range(len(open_ids)):          # print all open entities 
                            output.append((open_ids[j], open_labels[j], open_forms[j]))
                        open_ids, open_forms, open_labels = [], [], []
                        n_sentences += 1
                        n_tokens = 0
                        n_tokens_in_batch += 1
                    in_sentence = False
                else:
                    in_sentence = True
                    form, ne = line.split(SEP)
                    n_tokens += 1
                    n_tokens_in_batch += 1
                    if ne == "O":                           # all entities ended
                        for j in range(len(open_ids)):      # print all open entities
                            output.append((open_ids[j], open_labels[j], open_forms[j]))
                        open_ids, open_forms, open_labels = [], [], []
                    else:
                        labels = ne.split("|")
                        for j in range(len(labels)):        # for each label line
                            if j < len(open_ids):           # all open entities
                                # previous open entity ends here, close and replace with new entity instead
                                if labels[j].startswith("B-") or open_labels[j] != labels[j][2:]:
                                    output.append((open_ids[j], open_labels[j], open_forms[j]))
                                    open_ids[j] = [n_tokens_in_batch]
                                    open_forms[j] = form
                                # entity continues, append ids and forms
                                else:
                                    open_ids[j].append(n_tokens_in_batch)
                                    open_forms[j] += " " + form
                                open_labels[j] = labels[j][2:]
                            else: # no running entities, new entity starts here, just append
                                open_ids.append([n_tokens_in_batch])
                                open_forms.append(form)
                                open_labels.append(labels[j][2:])
            output.sort(key=lambda ids_labels_forms: (ids_labels_forms[0][0], -ids_labels_forms[0][-1]))
            output = "".join([",".join(map(str, ids)) + SEP + label + SEP + forms + "\n" for ids, label, forms in output])
            return output, n_tokens_in_batch


        @staticmethod
        def encode_entities(text):
            return text.replace('&', '&amp;').replace('<', '&lt;').replace('>', '&gt;').replace('"', '&quot;')


        def conll_to_xml(self, text, sentences):
            """Converts postprocessed (!) CoNLL output of the network.
            Postprocessing (model.postprocess) is important to enforce balanced
            bracketing and BIO format of the neural network output."""

            output = "" # TODO appending to string is quadratic
            open_labels = []
            in_sentence = False

            # indexes to tokenizer's sentences
            n_sentences = 0
            n_tokens = 0

            for line in text.split("\n"):
                if not line:                            # end of sentence
                    for i in range(len(open_labels)):   # close all open entities 
                        output += "</ne>"
                    open_labels = []
                    if in_sentence:
                        output += "</sentence>"         # close sentence
                        in_sentence = False
                        n_sentences += 1
                        n_tokens = 0
                else:                                   # in sentence
                    if not in_sentence:                 # sentence starts
                        output += "<sentence>"
                        in_sentence = True

                    cols = line.split(SEP)
                    form = cols[0]
                    ne = cols[1] if len(cols) == 2 else "O"

                    # This will work for properly nested entities,
                    # hence model.postprocess is important before conll_to_xml.
                    if ne == "O":                           # all entities ended
                        for i in range(len(open_labels)):   # close all open entities
                            output += "</ne>"
                        open_labels = []
                    else:
                        labels = ne.split("|")
                        for i in range(len(labels)):
                            if i < len(open_labels):
                                if labels[i].startswith("B-") or open_labels[i] != labels[i][2:]:
                                    # previous open entity ends here
                                    # -> close it and all open nested entities
                                    for j in range(i, len(open_labels)):
                                        output += "</ne>"
                                    open_labels = open_labels[:i]
                                    # open new entity
                                    output += "<ne type=\"" + self.encode_entities(labels[i][2:]) + "\">"
                                    open_labels.append(labels[i][2:])
                            else: # no running entities, new entity starts here, just append
                                output += "<ne type=\"" + self.encode_entities(labels[i][2:]) + "\">"
                                open_labels.append(labels[i][2:])

                    output += self.encode_entities(sentences[n_sentences][n_tokens].spaces_before)
                    output += "<token>" + self.encode_entities(form) + "</token>"
                    output += self.encode_entities(sentences[n_sentences][n_tokens].spaces_after)
                    n_tokens += 1

            return output


    def __init__(self, server_args):
        self.default_model = server_args.default_model
        self.models_list = []
        self.models_by_names = {}

        for i in range(0, len(server_args.models), 3):
            names, path, acknowledgements = server_args.models[i:i+3]
            names = names.split(":")
            names = [name.split("-") for name in names]
            names = ["-".join(parts[:None if not i else -i]) for parts in names for i in range(len(parts))]

            self.models_list.append(self.Model(path, names[0], acknowledgements, server_args))
            for name in names:
                self.models_by_names.setdefault(name, self.models_list[-1])

        # Check the default model exists
        assert self.default_model in self.models_by_names

class NameTag2Server(socketserver.ThreadingTCPServer):
    class NameTag2ServerRequestHandler(http.server.BaseHTTPRequestHandler):
        protocol_version = "HTTP/1.1"

        def respond(request, content_type, code=200):
            request.close_connection = True
            request.send_response(code)
            request.send_header("Connection", "close")
            request.send_header("Content-Type", content_type)
            request.send_header("Access-Control-Allow-Origin", "*")
            request.end_headers()

        def respond_error(request, message, code=400):
            request.respond("text/plain", code)
            request.wfile.write(message.encode("utf-8"))

        def do_GET(request):
            # Parse the URL
            params = {}
            try:
                request.path = request.path.encode("iso-8859-1").decode("utf-8")
                url = urllib.parse.urlparse(request.path)
                for name, value in urllib.parse.parse_qsl(url.query, encoding="utf-8", keep_blank_values=True, errors="strict"):
                    params[name] = value
            except:
                return request.respond_error("Cannot parse request URL.")

            # Parse the body of a POST request
            if request.command == "POST":
                if request.headers.get("Transfer-Encoding", "identity").lower() != "identity":
                    return request.respond_error("Only 'identity' Transfer-Encoding of payload is supported for now.")

                try:
                    content_length = int(request.headers["Content-Length"])
                except:
                    return request.respond_error("The Content-Length of payload is required.")

                if content_length > request.server._server_args.max_request_size:
                    return request.respond_error("The payload size is too large.")

                # multipart/form-data
                if request.headers.get("Content-Type", "").startswith("multipart/form-data"):
                    try:
                        parser = email.parser.BytesFeedParser()
                        parser.feed(b"Content-Type: " + request.headers["Content-Type"].encode("ascii") + b"\r\n\r\n")
                        while content_length:
                            parser.feed(request.rfile.read(min(content_length, 4096)))
                            content_length -= min(content_length, 4096)
                        for part in parser.close().get_payload():
                            name = part.get_param("name", header="Content-Disposition")
                            if name:
                                params[name] = part.get_payload(decode=True).decode("utf-8")
                    except:
                        return request.respond_error("Cannot parse the multipart/form-data payload.")
                elif request.headers.get("Content-Type", "").startswith("application/x-www-form-urlencoded"):
                    try:
                        for name, value in urllib.parse.parse_qsl(
                                request.rfile.read(content_length).decode("utf-8"), encoding="utf-8", keep_blank_values=True, errors="strict"):
                            params[name] = value
                    except:
                        return request.respond_error("Cannot parse the application/x-www-form-urlencoded payload.")
                else:
                    return request.respond_error("Unsupported payload Content-Type '{}'.".format(request.headers.get("Content-Type", "<none>")))

            # Handle /models
            if url.path == "/models":
                response = {
                    "models": {model.name: ["tokenize", "recognize"] for model in request.server._models.models_list},
                    "default_model": request.server._models.default_model,
                }
                request.respond("application/json")
                request.wfile.write(json.dumps(response, indent=1).encode("utf-8"))

            # Handle /tokenize and /recognize
            elif url.path in [ "/recognize", "/tokenize" ]:
                if "data" not in params:
                    return request.respond_error("The parameter 'data' is required.")

                # Model
                model = params.get("model", request.server._models.default_model)
                if model not in request.server._models.models_by_names:
                    return request.respond_error("The requested model '{}' does not exist.".format(model))
                model = request.server._models.models_by_names[model]

                # Input
                input_param = "untokenized" if url.path == "/tokenize" else params.get("input", "untokenized")
                if input_param not in ["untokenized", "vertical"]:
                    return request.respond_error("The requested input '{}' does not exist.".format(input_param))

                # Output
                output_param = params.get("output", "xml")
                if output_param not in ["xml", "vertical"] + (["conll"] if url.path == "/recognize" else []):
                    return request.respond_error("The requested output '{}' does not exist.".format(output_param))

                batch, started_responding, n_tokens_in_batches = [], False, 0
                try:
                    for sentence in itertools.chain(model._tokenizer.tokenize(params["data"], input_param), ["EOF"]):
                        if sentence == "EOF" or len(batch) == request.server._server_args.batch_size:
                            tokens = []
                            for batch_sentence in batch:
                                for token in batch_sentence:
                                    tokens.append(token.token)
                                tokens.append("")
                            tokens.append("")
                            output = "\n".join(tokens)
                            if url.path == "/recognize":
                                output = model.predict(output)
                                output = model.postprocess(output)
                                if output_param == "vertical":
                                    output, n_tokens_in_batches = model.conll_to_vertical(output, batch, n_tokens_in_batches)
                            if output_param == "xml":
                                output = model.conll_to_xml(output, batch)
                            if not started_responding:
                                # The first batch is ready, we commit to generate output.
                                request.respond("application/json")
                                request.wfile.write(json.dumps({
                                    "model": model.name,
                                    "acknowledgements": ["http://ufal.mff.cuni.cz/nametag/2#acknowledgements", model.acknowledgements],
                                    "result": "",
                                }, indent=1)[:-3].encode("utf-8"))
                                started_responding=True
                            request.wfile.write(json.dumps(output, ensure_ascii=False)[1:-1].encode("utf-8"))
                            batch = []
                        batch.append(sentence)
                    request.wfile.write(b'"\n}\n')

                except:
                    import traceback
                    traceback.print_exc(file=sys.stderr)
                    sys.stderr.flush()

                    if not started_responding:
                        request.respond_error("An internal error occurred during processing.")
                    else:
                        request.wfile.write(b'",\n"An internal error occurred during processing, producing incorrect JSON!"')

            else:
                request.respond_error("No handler for the given URL '{}'".format(url.path), code=404)

        def do_POST(request):
            return request.do_GET()

    daemon_threads = False

    def __init__(self, server_args, models):
        super().__init__(("", server_args.port), self.NameTag2ServerRequestHandler)

        self._server_args = server_args
        self._models = models

    def server_bind(self):
        import socket
        self.socket.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
        self.socket.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEPORT, 1)
        super().server_bind()

    def service_actions(self):
        if isinstance(getattr(self, "_threads", None), list):
            if len(self._threads) >= 1024:
                self._threads = [thread for thread in self._threads if thread.is_alive()]

if __name__ == "__main__":
    import signal
    import threading

    # Parse server arguments
    parser = argparse.ArgumentParser()
    parser.add_argument("port", type=int, help="Port to use")
    parser.add_argument("wembedding_server", type=str, help="Address of an WEmbedding server")
    parser.add_argument("default_model", type=str, help="Default model")
    parser.add_argument("models", type=str, nargs="+", help="Models to serve")
    parser.add_argument("--batch_size", default=64, type=int, help="Batch size")
    parser.add_argument("--logfile", default=None, type=str, help="Log path")
    parser.add_argument("--max_labels_per_token", default=5, type=int, help="Maximum labels per token.")
    parser.add_argument("--max_request_size", default=4096*1024, type=int, help="Maximum request size")
    parser.add_argument("--threads", default=4, type=int, help="Threads to use")
    args = parser.parse_args()

    # Log stderr to logfile if given
    if args.logfile is not None:
        sys.stderr = open(args.logfile, "a", encoding="utf-8")

    # Load the models
    models = Models(args)

    # Create the server
    server = NameTag2Server(args, models)
    server_thread = threading.Thread(target=server.serve_forever, daemon=True)
    server_thread.start()

    print("Started NameTag 2 server on port {}.".format(args.port), file=sys.stderr)
    print("To stop it gracefully, either send SIGINT (Ctrl+C) or SIGUSR1.", file=sys.stderr, flush=True)

    # Wait until the server should be closed
    signal.pthread_sigmask(signal.SIG_BLOCK, [signal.SIGINT, signal.SIGUSR1])
    signal.sigwait([signal.SIGINT, signal.SIGUSR1])
    print("Initiating shutdown of the NameTag 2 server.", file=sys.stderr, flush=True)
    server.shutdown()
    print("Stopped handling new requests, processing all current ones.", file=sys.stderr, flush=True)
    server.server_close()
    print("Finished shutdown of the NameTag 2 server.", file=sys.stderr, flush=True)
