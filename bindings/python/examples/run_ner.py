# This file is part of NameTag <http://github.com/ufal/nametag/>.
#
# Copyright 2016 Institute of Formal and Applied Linguistics, Faculty of
# Mathematics and Physics, Charles University in Prague, Czech Republic.
#
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http://mozilla.org/MPL/2.0/.

import sys

from ufal.nametag import *

def encode_entities(text):
  return text.replace('&', '&amp;').replace('<', '&lt;').replace('>', '&gt;').replace('"', '&quot;')

def sort_entities(entities):
  return sorted(entities, key=lambda entity: (entity.start, -entity.length))

# In Python2, wrap sys.stdin and sys.stdout to work with unicode.
if sys.version_info[0] < 3:
  import codecs
  import locale
  encoding = locale.getpreferredencoding()
  sys.stdin = codecs.getreader(encoding)(sys.stdin)
  sys.stdout = codecs.getwriter(encoding)(sys.stdout)

if len(sys.argv) == 1:
  sys.stderr.write('Usage: %s recognizer_model\n' % sys.argv[0])
  sys.exit(1)

sys.stderr.write('Loading ner: ')
ner = Ner.load(sys.argv[1])
if not ner:
  sys.stderr.write("Cannot load recognizer from file '%s'\n" % sys.argv[1])
  sys.exit(1)
sys.stderr.write('done\n')

forms = Forms()
tokens = TokenRanges()
entities = NamedEntities()
sortedEntities = []
openEntities = []
tokenizer = ner.newTokenizer()
if tokenizer is None:
  sys.stderr.write("No tokenizer is defined for the supplied model!")
  sys.exit(1)

not_eof = True
while not_eof:
  text = ''

  # Read block
  while True:
    line = sys.stdin.readline()
    not_eof = bool(line)
    if not not_eof: break
    line = line.rstrip('\r\n')
    text += line
    text += '\n';
    if not line: break

  # Tokenize and recognize
  tokenizer.setText(text)
  t = 0
  while tokenizer.nextSentence(forms, tokens):
    ner.recognize(forms, entities)
    sortedEntities = sort_entities(entities)

    # Write entities
    e = 0
    for i in range(len(tokens)):
      sys.stdout.write(encode_entities(text[t:tokens[i].start]))
      if (i == 0): sys.stdout.write("<sentence>")

      # Open entities starting at current token
      while (e < len(sortedEntities) and sortedEntities[e].start == i):
        sys.stdout.write('<ne type="%s">' % encode_entities(sortedEntities[e].type))
        openEntities.append(sortedEntities[e].start + sortedEntities[e].length - 1)
        e = e + 1

      # The token itself
      sys.stdout.write('<token>%s</token>' % encode_entities(text[tokens[i].start : tokens[i].start + tokens[i].length]))

      # Close entities ending after current token
      while openEntities and openEntities[-1] == i:
        sys.stdout.write('</ne>')
        openEntities.pop()
      if (i + 1 == len(tokens)): sys.stdout.write("</sentence>")
      t = tokens[i].start + tokens[i].length
  # Write rest of the text
  sys.stdout.write(encode_entities(text[t:]))
