# NameTag 2

This is a named entity recognition tool NameTag 2.

It is a public release of the following publication:

Jana Straková, Milan Straka, Jan Hajič (2019): Neural Architectures for Nested NER through Linearization. In: Proceedings of the 57th Annual Meeting of the Association for Computational Linguistics, pp. 5326-5331, Association for Computational Linguistics, Stroudsburg, PA, USA, ISBN 978-1-950737-48-2 (https://aclweb.org/anthology/papers/P/P19/P19-1527/)

NameTag 2 can be used either as a commandline tool (this repository) or by requesting NameTag webservice (http://lindat.mff.cuni.cz/services/nametag). You can also run your own NameTag server.

Homepage: https://ufal.mff.cuni.cz/nametag

Web service and demo: http://lindat.mff.cuni.cz/services/nametag/

Contact: strakova@ufal.mff.cuni.cz

## License

Copyright 2021 Institute of Formal and Applied Linguistics, Faculty of Mathematics and Physics, Charles University, Czech Republic.

This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0. If a copy of the MPL was not distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.

## Please cite as

```
@inproceedings{strakova-etal-2019-neural,
    title = {{Neural Architectures for Nested {NER} through Linearization}},
    author = {Jana Strakov{\'a} and Milan Straka and Jan Haji\v{c}},
    booktitle = {Proceedings of the 57th Annual Meeting of the Association for Computational Linguistics},
    month = jul,
    year = {2019},
    address = {Florence, Italy},
    publisher = {Association for Computational Linguistics},
    url = {https://www.aclweb.org/anthology/P19-1527},
    pages = {5326--5331},
}
```

## Requirements

The software has been developed and tested on Linux. You'll need a machine with AVX instructions for serving BERT embeddings (see below).

## Installation

### Clone the NameTag 2 GIT repository

```sh
git clone https://github.com/ufal/nametag -b nametag2
```

The main NameTag 2 tagger is called `nametag2.py`. Running `nametag2.py` for NER prediction is described in Section 5. Training your own custom NER model is described later.

### Get TensorFlow 1.12

You will need Python virtual environment with TensorFlow 1.12 either on CPU or GPU.

#### TensorFlow on CPU

Create a Python virtual environment with TensorFlow 1.12 called `venv-tf-1.12-cpu` in the root of this directory:

```sh
python3 -m venv venv-tf-1.12-cpu
venv-tf-1.12-cpu/bin/pip3 install -r requirements.txt
```

The `nametag2.py` script is then called using the Python installed in your virtual environment, which now contains also the appropriate version of TensorFlow.

#### TensorFlow on GPU

If you plan to scale up for larger experiments or greater speed, you can use the TensorFlow GPU version. You need to install Python virtual environment with GPU TensorFlow version.

### Download the NameTag 2 models

Download the latest version of NameTag 2 models from https://ufal.mff.cuni.cz/nametag/2#models.

## Get WEmbedding service

NameTag 2 is using contextualized BERT embeddings computed by Transformers (https://arxiv.org/abs/1910.03771, https://github.com/huggingface/transformers).

To this end, we issused a webservice serving transformers BERT embeddings called WEmbedding service: `https://github.com/ufal/wembedding_service`. Adding WEmbeddings service as a submodule, a clone command should be:

```sh
git clone --recurse-submodules
```

and/or at later stages (in existing clones):

```sh
git submodule update --init
```

Then install Python `venv` using `requirements.txt` inside the submodule (see also WEmbeddings repository `README`):

```sh
cd wembedding_service
python3 -m venv venv
venv/bin/python3 install -r requirements.txt
```

This will allow you to run the WEmbeddings server which will serve the BERT embeddings to NameTag 2:

```sh
venv/bin/python3 ./start_wembeddings_server.py 8000
```

## Running NER prediction with NameTag 2

Before you run NameTag 2 for NER prediction, please make sure you have:

1. installed Python virtual environment,
2. downloaded the NameTag 2 models,
3. you have a ip:port of a running BERT WEmbeddings service

The main NameTag 2 script is called `nametag2.py`. Example NER prediction usage:

```sh
venv-tf-1.12-cpu/bin/python3 nametag2.py --test_data=examples/en_input.conll --predict=models/english-conll-200831 --bert=localhost:8000
```

Example usage with more documentation on input and output formats can be found in the `nametag2.py` script.

## Training NameTag 2

The main NameTag 2 script `nametag2.py` can be used for training a custom corpus. It will do so when provided the parameters `--train_data`. Optionally, `--dev_data` can be provided and many other training parameters that control the training.

The input data file format is a vertical file, one token and its label per line, separated by a tabulator; sentences delimited by newlines (such as a first and fourth column in a well-known CoNLL-2003 IOB shared task corpus). An example of such input file can be found in `nametag2.py` and in `examples/en_gold.conll`.

Example usage:

```sh
venv-tf-1.12-cpu/bin/python3 nametag2.py --threads=4 --batch_size=4 --train_data=train.conll --dev_data=dev.conll --epochs=10:1e-3,8:1e-4 --save_checkpoint=my_models --bert_train=train_bert.npz --bert_dev=dev_bert.npz
```

While it is technically possible to compute the BERT embeddings on demand for each shuffled batch in each step, we strongly recommend to precompute the BERT embeddings as numpy arrays, one array per each token in your vertical CoNLL-like file and store these as npz files. The argument `--bert_train` and `--bert_dev` will load the precomputed BERT embeddings from the npz file and reuse them during training, speeding up the training process. 

## NameTag 2 server

See `nametag2_server.py`.

The mandatory arguments are given in this order:

- port
- WEmbeddings server (host:port)
- default model name
- each following triple of arguments defines a model, of which
  - first argument is the model name
  - second argument is the model directory
  - third argument are the acknowledgemets to append

Example server usage:

```sh
venv/bin/python nametag2_server.py 8001 localhost:8000 czech-cnec2.0-200831 czech-cnec2.0-200831 models/czech-cnec2.0-200831/ ack-text
```
