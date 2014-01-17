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

# In Python2, wrap sys.stdin and sys.stdout to work with unicode.
if sys.version_info[0] < 3:
  import codecs
  import locale
  encoding = locale.getpreferredencoding()
  sys.stdin = codecs.getreader(encoding)(sys.stdin)
  sys.stdout = codecs.getwriter(encoding)(sys.stdout)

def recognize_vertical(ner):
  forms = Forms()
  entities = NamedEntities()

  lines = 0
  not_eof = True
  while not_eof:
    forms.clear()

    # Read sentence
    while True:
      line = sys.stdin.readline()
      not_eof = bool(line)
      line = line.rstrip('\r\n')
      if not line: break
      forms.append(line)

    # Tag
    if forms:
      ner.recognize(forms, entities)

      for entity in entities:
        sys.stdout.write('%s\t%s\t%s\n' % (
          ','.join(map(str, range(lines + 1 + entity.start, lines + 1 + entity.start + entity.length))),
          entity.type,
          ' '.join(forms[entity.start : entity.start + entity.length])))

    lines += forms.size() + 1

def encode_entities(text):
  return text.replace('&', '&amp;').replace('<', '&lt;').replace('>', '&gt;').replace('"', '&quot;')

def recognize_untokenized(ner):
  entities = NamedEntities()
  openEntities = []

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
    ner.tokenizeAndRecognize(text, entities)

    # Write entities
    t = 0
    for entity in entities:
      # Close entities that end sooner than current one
      while openEntities and openEntities[-1] < entity.start:
        sys.stdout.write('%s</ne>' % encode_entities(text[t : openEntities[-1]]))
        t = openEntities.pop()

      # Print text just before entity, open it and add end to the stack
      sys.stdout.write('%s<ne type="%s">' % (encode_entities(text[t : entity.start]), entity.type))
      t = entity.start
      openEntities.append(entity.start + entity.length)

    # Close unclosed entities
    while openEntities:
      sys.stdout.write('%s</ne>' % encode_entities(text[t : openEntities[-1]]))
      t = openEntities.pop()

    # Write rest of the text
    sys.stdout.write(text[t:])

argi = 1
if argi < len(sys.argv) and sys.argv[argi] == "-v": argi = argi + 1
use_vertical = argi > 1

if not argi < len(sys.argv):
  sys.stderr.write('Usage: %s [-v] ner_model\n' % sys.argv[0])
  sys.exit(1)

sys.stderr.write('Loading ner: ')
ner = Ner.load(sys.argv[argi])
if not ner:
  sys.stderr.write("Cannot load recognizer from file '%s'\n" % sys.argv[argi])
  sys.exit(1)
sys.stderr.write('done\n')

if use_vertical: recognize_vertical(ner)
else: recognize_untokenized(ner)
