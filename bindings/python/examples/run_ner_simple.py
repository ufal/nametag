# This file is part of NameTag <http://github.com/ufal/nametag/>.
#
# Copyright 2019 Institute of Formal and Applied Linguistics, Faculty of
# Mathematics and Physics, Charles University in Prague, Czech Republic.
#
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http://mozilla.org/MPL/2.0/.

# A very simple NameTag Python binding usage example. Does not handle data
# reading and writing. For a complete example including data reading and
# writing, please see run_ner.py.

# To run this script from the commandline, simply type:
# python run_ner_simple.py path_to_nametag_model

import sys

from ufal.nametag import *

# Check required commandline argument (path to NameTag model).
if len(sys.argv) == 1:
  sys.stderr.write('Usage: %s recognizer_model\n' % sys.argv[0])
  sys.exit(1)

# Load NameTag model from given path.
sys.stderr.write('Loading ner: ')
ner = Ner.load(sys.argv[1])
if not ner:
  sys.stderr.write("Cannot load recognizer from file '%s'\n" % sys.argv[1])
  sys.exit(1)
sys.stderr.write('done\n')

# Create structures for keeping recognized forms, tokens and entities.
forms = Forms()
tokens = TokenRanges()
entities = NamedEntities()

# Create tokenizer.
tokenizer = ner.newTokenizer()
if tokenizer is None:
  sys.stderr.write("No tokenizer is defined for the supplied model!")
  sys.exit(1)

# Create a sample text.
text = "John loves Mary. They live in London."

# Set the text in the tokenizer.
tokenizer.setText(text)

# Go through all sentences returned by tokenizer one by one.
# Each cycle, the tokenizer will cut a sentence from the text beginning,
# and fill the forms and tokens.
sentence_number = 0
while tokenizer.nextSentence(forms, tokens):
  sentence_number += 1
  print("Sentence number {}: {}".format(sentence_number, " ".join(forms)))
  
  # Now the forms structure is filled by the tokenizer, so NE recognizer will
  # get them (first argument) and will fill entities (second argument) with
  # recognized entities.
  ner.recognize(forms, entities)

  # Print entities
  # The entity structure contains these fields:
  #   - type: entity type,
  #   - start: form index in the forms structure,
  #   - length: number of forms.
  for entity in entities:
      print("{} {}".format(entity.type, " ".join(forms[entity.start:entity.start+entity.length])))
