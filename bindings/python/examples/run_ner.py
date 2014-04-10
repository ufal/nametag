# This file is part of NameTag.
#
# Copyright 2013 by Institute of Formal and Applied Linguistics, Faculty of
# Mathematics and Physics, Charles University in Prague, Czech Republic.
#
# NameTag is free software: you can redistribute it and/or modify
# it under the terms of the GNU Lesser General Public License as
# published by the Free Software Foundation, either version 3 of
# the License, or (at your option) any later version.
#
# NameTag is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU Lesser General Public License for more details.
#
# You should have received a copy of the GNU Lesser General Public License
# along with NameTag.  If not, see <http://www.gnu.org/licenses/>.

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
